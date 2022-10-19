#include "MainComponent.hpp"
#include "ErrorMessage.hpp"
#include "EnergyPlus.hpp"

#include <ftxui/component/captured_mouse.hpp>      // for ftxui
#include <ftxui/component/component.hpp>           // for Input, Renderer, Vertical
#include <ftxui/component/component_base.hpp>      // for ComponentBase
#include <ftxui/component/component_options.hpp>   //
#include <ftxui/component/screen_interactive.hpp>  // for Component, ScreenInteractive
#include <ftxui/dom/elements.hpp>                  // for operator|, Element, size, border, frame, vscroll_indicator, HEIGHT, LESS_THAN
#include <ftxui/dom/table.hpp>                     // for Table, TableSelection
#include <ftxui/screen/screen.hpp>                 // for Screen
#include <ftxui/screen/color.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/component/receiver.hpp>

#include <algorithm>   // for min, max
#include <atomic>      // for atomic
#include <chrono>      // for operator""s, chrono_literals
#include <filesystem>  // for path

#include <fmt/format.h>
#include <fmt/chrono.h>
#include <fmt/std.h>  // for formatting std::filesystem::path

#ifdef _WIN32
#  define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

namespace fs = std::filesystem;

template <typename TP>
std::chrono::time_point<std::chrono::system_clock> to_system_clock(TP tp) {
  return std::chrono::time_point_cast<std::chrono::system_clock::duration>(tp - TP::clock::now() + std::chrono::system_clock::now());
}

int main(int argc, const char* argv[]) {

  fs::path filePath;

  if (argc > 1) {
    filePath = fs::path(argv[argc - 1]);
    if (!epcli::validateFileType(filePath)) {
      filePath = fs::path("in.idf");
    }
    if (!fs::is_regular_file(filePath)) {
      fmt::print("File does not exist at '{}'\n", filePath);
      exit(1);
    }
  }
  fs::path outputDirectory(".");
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    if (arg == "-d" || arg == "--output-directory") {
      outputDirectory = fs::path(argv[i + 1]);
      break;
    }
  }

  /* std::chrono::time_point<std::chrono::file_clock> */ auto lastWriteTime = fs::last_write_time(filePath);

  auto screen = ftxui::ScreenInteractive::Fullscreen();

  auto receiverRunOutput = ftxui::MakeReceiver<std::string>();
  auto receiverErrorOutput = ftxui::MakeReceiver<ErrorMessage>();
  auto senderRunOutput = receiverRunOutput->MakeSender();
  auto senderErrorOutput = receiverErrorOutput->MakeSender();

  bool hasBeenRunBefore = false;

  std::atomic<int> progress = 0;

  std::shared_ptr<MainComponent> component;

  std::string run_text = "Run " + filePath.string();
  std::thread runThread;
  auto run_button = ftxui::Button(
    &run_text,
    [&]() {
      if (runThread.joinable()) {
        runThread.join();
      }
      if (hasBeenRunBefore) {
        fs::file_time_type newWriteTime = fs::last_write_time(filePath);
        if (newWriteTime <= lastWriteTime) {
          senderRunOutput->Send("--------------------------------------------------------------------------");
          senderRunOutput->Send(fmt::format("Refusing to rerun file at {}, it was not modified since last run, Last modified time: {}", filePath,
                                            to_system_clock(lastWriteTime)));
          return;
        }
        component->clear_state();
      }
      hasBeenRunBefore = true;
      runThread = std::thread(epcli::runEnergyPlus, argc, argv, &senderRunOutput, &senderErrorOutput, &progress, &screen);
    },
    ftxui::ButtonOption::Simple());

  std::string quit_text = "Quit";
  auto quit_button = ftxui::Button(&quit_text, screen.ExitLoopClosure(), ftxui::ButtonOption::Ascii());

  component = std::make_shared<MainComponent>(std::move(receiverRunOutput), std::move(receiverErrorOutput), std::move(run_button),
                                              std::move(quit_button), &progress, std::move(outputDirectory));

  screen.Loop(component);

  if (runThread.joinable()) {
    runThread.join();
  }

  // screen.Loop(ftxui::Container::Vertical({
  //   renderer,
  //   renderer_runOutput,
  // }));

  std::cout << std::endl;

  // This is a compile definition
  // fmt::print("{}\n", ENERGYPLUS_ROOT);

  return 0;
}

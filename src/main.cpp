#include "MainComponent.hpp"
#include "ErrorMessage.hpp"
#include "EnergyPlus.hpp"

#include "ftxui/component/component.hpp"           // for Button
#include <ftxui/component/component_base.hpp>      // for ComponentBase
#include <ftxui/component/component_options.hpp>   // for ButtonOption
#include <ftxui/component/receiver.hpp>            // for MakeReceiver, Sender
#include <ftxui/component/screen_interactive.hpp>  // for ScreenInteractive
#include <ftxui/dom/elements.hpp>                  // for Element, text, operator|

// TODO: temp, FTXUI 3.0.0 doesn't include this component yet, it's only on master.
#include "ftxui/modal.hpp"

#include <atomic>      // for atomic
#include <chrono>      // for operator""s, chrono_literals
#include <cstdlib>     // for exit
#include <filesystem>  // for path, absolute
#include <functional>  // for function
#include <memory>      // for allocator, shared_ptr
#include <string>      // for string, basic_string
#include <thread>      // for thread
#include <utility>     // for move

#include <fmt/format.h>  // for formatting
#include <fmt/chrono.h>  // for formatting std::chrono::time_point<std::chrono::system_clock>
#include <fmt/std.h>     // for formatting std::filesystem::path

#ifdef _WIN32
#  define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

namespace fs = std::filesystem;

template <typename TP>
std::chrono::time_point<std::chrono::system_clock> to_system_clock(TP tp) {
  return std::chrono::time_point_cast<std::chrono::system_clock::duration>(tp - TP::clock::now() + std::chrono::system_clock::now());
}

// Definition of the modal component. The details are not important.
ftxui::Component ReloadModalComponent(std::function<void()> reload_results, std::function<void()> hide_modal, const fs::path& outputDirectory) {
  auto component = Container::Vertical({
    ftxui::Button("Reload Results", std::move(reload_results), ftxui::ButtonOption::Animated()),
    ftxui::Button("Start Fresh", std::move(hide_modal), ftxui::ButtonOption::Animated()),
  });
  // Polish how the two buttons are rendered:
  component |= ftxui::Renderer([&](ftxui::Element inner) {
    return vbox({
             ftxui::text("Previous Results Found on Disk"),
             ftxui::text(fs::absolute(outputDirectory).string()),
             separator(),
             std::move(inner),
           })                                                    //
           | ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 30)  //
           | ftxui::border;                                      //
  });
  return component;
}

int main(int argc, const char* argv[]) {

  // State of the application:
  bool modal_reload_shown = false;

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
  if (fs::is_regular_file(outputDirectory / "eplusout.err")) {
    modal_reload_shown = true;
  }

  /* std::chrono::time_point<std::chrono::file_clock> */ auto lastWriteTime = fs::last_write_time(filePath);

  auto screen = ftxui::ScreenInteractive::Fullscreen();

  auto receiverRunOutput = ftxui::MakeReceiver<std::string>();
  auto receiverErrorOutput = ftxui::MakeReceiver<ErrorMessage>();
  auto senderRunOutput = receiverRunOutput->MakeSender();
  auto senderErrorOutput = receiverErrorOutput->MakeSender();

  std::atomic<int> progress = 0;

  std::shared_ptr<MainComponent> main_component;

  std::string run_text = "Run " + filePath.string();
  std::thread runThread;
  auto run_button = ftxui::Button(
    &run_text,
    [&]() {
      if (runThread.joinable()) {
        runThread.join();
      }
      if (main_component != nullptr && main_component->hasAlreadyRun()) {
        fs::file_time_type newWriteTime = fs::last_write_time(filePath);
        if (newWriteTime <= lastWriteTime) {
          senderRunOutput->Send("--------------------------------------------------------------------------");
          senderRunOutput->Send(fmt::format("Refusing to rerun file at {}, it was not modified since last run, Last modified time: {}", filePath,
                                            to_system_clock(lastWriteTime)));
          return;
        }
        main_component->clear_state();
      }
      runThread = std::thread(epcli::runEnergyPlus, argc, argv, &senderRunOutput, &senderErrorOutput, &progress, &screen);
    },
    ftxui::ButtonOption::Simple());

  std::string quit_text = "Quit";
  auto quit_button = ftxui::Button(&quit_text, screen.ExitLoopClosure(), ftxui::ButtonOption::Ascii());

  main_component = std::make_shared<MainComponent>(std::move(receiverRunOutput), std::move(receiverErrorOutput), std::move(run_button),
                                                   std::move(quit_button), &progress, std::move(outputDirectory));

  auto hide_modal = [&modal_reload_shown] { modal_reload_shown = false; };
  auto reload_results = [&main_component, &modal_reload_shown]() {
    modal_reload_shown = false;
    main_component->reload_results();
  };

  auto modal_component = ReloadModalComponent(reload_results, hide_modal, outputDirectory);

  // Use the `Modal` function to use together the main component and its modal window.
  // The |modal_reload_shown| boolean controls whether the modal is shown or not.
  auto composite = ftxui::Modal(main_component, modal_component, &modal_reload_shown);

  screen.Loop(composite);

  if (runThread.joinable()) {
    runThread.join();
  }

  // screen.Loop(ftxui::Container::Vertical({
  //   renderer,
  //   renderer_runOutput,
  // }));

  fmt::print("\n");

  // This is a compile definition
  // fmt::print("{}\n", ENERGYPLUS_ROOT);

  return 0;
}

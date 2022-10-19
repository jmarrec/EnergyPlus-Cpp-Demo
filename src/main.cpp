#include <EnergyPlus/api/runtime.h>
#include <EnergyPlus/api/state.h>
#include <EnergyPlus/api/func.h>
#include <EnergyPlus/api/TypeDefs.h>

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

#include <algorithm>  // for min, max
#include <atomic>     // for atomic
#include <chrono>     // for operator""s, chrono_literals

#include <fmt/format.h>

#ifdef _WIN32
#  define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

int main(int argc, const char* argv[]) {

  auto screen = ftxui::ScreenInteractive::Fullscreen();

  auto receiverRunOutput = ftxui::MakeReceiver<std::string>();
  auto senderRunOutput = receiverRunOutput->MakeSender();

  auto receiverErrorOutput = ftxui::MakeReceiver<ErrorMessage>();
  auto senderErrorOutput = receiverErrorOutput->MakeSender();

  std::atomic<int> progress = 0;

  std::string run_text = "Run";
  std::thread runThread;
  auto run_button = ftxui::Button(
    &run_text,
    [&]() {
      runThread = std::thread(epcli::runEnergyPlus, argc, argv, std::move(senderRunOutput), std::move(senderErrorOutput), &progress, &screen);
    },
    ftxui::ButtonOption::Ascii());

  std::string quit_text = "Quit";
  auto quit_button = ftxui::Button(&quit_text, screen.ExitLoopClosure(), ftxui::ButtonOption::Ascii());

  auto component = std::make_shared<MainComponent>(std::move(receiverRunOutput), std::move(receiverErrorOutput), std::move(run_button),
                                                   std::move(quit_button), &progress);

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

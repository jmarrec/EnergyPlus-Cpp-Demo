#include <EnergyPlus/api/runtime.h>
#include <EnergyPlus/api/state.h>
#include <EnergyPlus/api/func.h>
#include <EnergyPlus/api/TypeDefs.h>

#include "MainComponent.hpp"
#include "ErrorMessage.hpp"

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

void runEnergyPlus(int argc, const char* argv[], ftxui::Sender<std::string> senderRunOutput, ftxui::Sender<ErrorMessage> senderErrorOutput,
                   std::atomic<int>* progress, ftxui::ScreenInteractive* screen) {

  EnergyPlusState state = stateNew();
  setEnergyPlusRootDirectory(state, ENERGYPLUS_ROOT);

  // callbackBeginNewEnvironment(state, BeginNewEnvironmentHandler);
  registerProgressCallback(state, [&progress, &screen](int const t_progress) {
    // The |progress| variable belong to the main thread. `screen.Post(task)`
    // will execute the update on the thread where |screen| lives (e.g. the
    // main thread). Using `screen.Post(task)` is threadsafe.
    *progress = t_progress;

    // After updating the state, request a new frame to be drawn. This is done
    // by simulating a new "custom" event to be handled.
    screen->PostEvent(ftxui::Event::Custom);
  });

  setConsoleOutputState(state, 0);
  registerStdOutCallback(state, [&senderRunOutput, &screen](const std::string& message) {
    senderRunOutput->Send(message);
    screen->PostEvent(ftxui::Event::Custom);
  });

  registerErrorCallback(state, [&senderErrorOutput, &screen](EnergyPlus::Error error, const std::string& message) {
    // fmt::print("[{}%] {}\n", progress, msg);

    senderErrorOutput->Send(ErrorMessage{error, message});
    screen->PostEvent(ftxui::Event::Custom);
  });

  energyplus(state, argc, argv);

  *progress = 100;
  stateDelete(state);

  screen->PostEvent(ftxui::Event::Custom);
}

int main(int argc, const char* argv[]) {

  auto screen = ftxui::ScreenInteractive::Fullscreen();

  std::atomic<int> progress = 0;

  std::string gauge_label = "Pending";
  auto render_gauge = [&progress, &gauge_label]() {
    return ftxui::hbox({
      ftxui::text(gauge_label),
      ftxui::gauge(progress / 100.f) | ftxui::flex,
      ftxui::text(fmt::format("{} %", progress)),
    });
  };

  auto receiverRunOutput = ftxui::MakeReceiver<std::string>();
  auto senderRunOutput = receiverRunOutput->MakeSender();

  auto receiverErrorOutput = ftxui::MakeReceiver<ErrorMessage>();
  auto senderErrorOutput = receiverErrorOutput->MakeSender();

  auto component = std::make_shared<MainComponent>(std::move(receiverRunOutput), std::move(receiverErrorOutput));

  auto gauge_component = ftxui::Renderer([render_gauge] { return render_gauge(); });

  std::string run_text = "Run";

  std::thread runThread;
  auto run_button = ftxui::Button(
    &run_text,
    [&]() { runThread = std::thread(runEnergyPlus, argc, argv, std::move(senderRunOutput), std::move(senderErrorOutput), &progress, &screen); },
    ftxui::ButtonOption::Ascii());

  std::string quit_text = "Quit";
  auto quit_button = ftxui::Button(&quit_text, screen.ExitLoopClosure(), ftxui::ButtonOption::Ascii());

  auto firstrow = ftxui::Container::Horizontal({run_button, quit_button});

  auto container = ftxui::Container::Vertical({
    firstrow,
    gauge_component,
    component,
  });

  auto renderer = ftxui::Renderer(container, [&] {
    return ftxui::vbox({
             ftxui::hbox({
               run_button->Render() | ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 20),
               ftxui::filler(),
               quit_button->Render() | ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 20),
             }),
             ftxui::separator(),
             render_gauge(),
             ftxui::separator(),
             component->Render(),
           })
           | ftxui::border;
  });

  screen.Loop(renderer);

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

#include <EnergyPlus/api/runtime.h>
#include <EnergyPlus/api/state.h>
#include <EnergyPlus/api/func.h>
#include <EnergyPlus/api/TypeDefs.h>

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

#include <algorithm>  // for min, max
#include <atomic>     // for atomic
#include <chrono>     // for operator""s, chrono_literals

#include <fmt/format.h>

#ifdef _WIN32
#  define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

int main(int argc, const char* argv[]) {

  auto screen = ftxui::ScreenInteractive::TerminalOutput();

  int progress = 0;

  std::string gauge_label = "Pending";
  auto render_gauge = [&progress, &gauge_label]() {
    return ftxui::hbox({
      ftxui::text(gauge_label),
      ftxui::gauge(progress / 100.f) | ftxui::flex,
      ftxui::text(fmt::format("{} %", progress)),
    });
  };

  auto gauge_component = ftxui::Renderer([render_gauge] { return render_gauge(); });

  std::atomic<bool> keep_running = true;
  std::thread runThread;

  std::vector<std::string> runOutputText{"Not running yet"};

  int selected_runOutputLine = 0;

  std::string run_text = "Run";
  auto runIt = [&]() {
    int thisProgress = 0;
    int thisSelected_runOutputLine = 0;

    std::string thisMessage;

    EnergyPlusState state = stateNew();
    setEnergyPlusRootDirectory(state, ENERGYPLUS_ROOT);

    // callbackBeginNewEnvironment(state, BeginNewEnvironmentHandler);
    registerProgressCallback(state, [&thisProgress, &screen, &progress](int const t_progress) {
      // The |progress| variable belong to the main thread. `screen.Post(task)`
      // will execute the update on the thread where |screen| lives (e.g. the
      // main thread). Using `screen.Post(task)` is threadsafe.
      thisProgress = t_progress;
      screen.Post([&] { progress = thisProgress; });
      // After updating the state, request a new frame to be drawn. This is done
      // by simulating a new "custom" event to be handled.
      screen.Post(ftxui::Event::Custom);
    });

    setConsoleOutputState(state, 0);
    registerStdOutCallback(state,
                           [&thisMessage, &thisSelected_runOutputLine, &selected_runOutputLine, &screen, &runOutputText](const std::string& msg) {
                             // fmt::print("[{}%] {}\n", progress, msg);
                             thisMessage = msg;
                             if (thisMessage.empty()) {
                               return;
                             }
                             ++thisSelected_runOutputLine;
                             screen.Post([&] {
                               runOutputText.push_back(thisMessage);
                               selected_runOutputLine = thisSelected_runOutputLine;
                             });
                             screen.PostEvent(ftxui::Event::Custom);
                           });
    //  registerErrorCallback(state, [&displayGauge](EnergyPlus::Error error, const std::string& message) {
    //    fmt::print("{} - {}\n", formatError(error), message);
    //  });

    gauge_label = "Running E+:";
    energyplus(state, argc, argv);

    thisProgress = 100;
    gauge_label = "Done Running E+";

    screen.Post([&] { progress = thisProgress; });
    screen.Post(ftxui::Event::Custom);
    //
    // if (numWarnings > 0) {
    //   fmt::print("There were {} warnings!\n", numWarnings);
    // }

    // using namespace std::chrono_literals;
    // std::this_thread::sleep_for(0.05s);
    // thisProgress++;
    // // The |shift| variable belong to the main thread. `screen.Post(task)`
    // // will execute the update on the thread where |screen| lives (e.g. the
    // // main thread). Using `screen.Post(task)` is threadsafe.
    // screen.Post([&] {
    //   progress = thisProgress;
    //   run_text = "Running";
    // });
    // // After updating the state, request a new frame to be drawn. This is done
    // // by simulating a new "custom" event to be handled.
    // screen.Post(ftxui::Event::Custom);
  };

  auto run_button = ftxui::Button(
    &run_text, [&]() { runThread = std::thread{runIt}; }, ftxui::ButtonOption::Ascii());

  std::string quit_text = "Quit";
  auto quit_button = ftxui::Button(&quit_text, screen.ExitLoopClosure(), ftxui::ButtonOption::Ascii());

  auto firstrow = ftxui::Container::Horizontal({run_button, quit_button});

  /*
  auto renderer_runOutput = [&runOutputText]() {
    if (runOutputText.empty()) {
      return ftxui::text("");
    }

    return ftxui::text(runOutputText.back());
  };

  auto runOutput_component = ftxui::Renderer([renderer_runOutput] { return renderer_runOutput(); });
*/

  auto runOutputMenu = ftxui::Menu(&runOutputText, &selected_runOutputLine);

  auto render_runOutputMenu = ftxui::Renderer(runOutputMenu, [&] {
    int begin = std::max(0, selected_runOutputLine - 10);
    int end = std::min(static_cast<int>(runOutputText.size()), selected_runOutputLine + 10);
    ftxui::Elements elements;
    for (int i = begin; i < end; ++i) {
      ftxui::Element element = ftxui::text(runOutputText[i]);
      if (i == selected_runOutputLine) {
        element = element | ftxui::inverted | ftxui::select;
      }
      elements.push_back(element);
    }
    return vbox(std::move(elements)) | ftxui::vscroll_indicator | ftxui::frame | ftxui::border;
  });

  //int log_index = 0;
  // auto log_menu = ftxui::Menu(&runOutputText, &log_index);

  auto container = ftxui::Container::Vertical({
    firstrow,
    gauge_component,
    runOutputMenu,
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
             ftxui::text("Stdout"),
             render_runOutputMenu->Render(),
           })
           | ftxui::border;
  });

  screen.Loop(renderer);

  keep_running = false;
  if (runThread.joinable()) {
    runThread.join();
  }

  // screen.Loop(ftxui::Container::Vertical({
  //   renderer,
  //   renderer_runOutput,
  // }));

  std::cout << std::endl;

  // This is a compile definition
  fmt::print("{}\n", ENERGYPLUS_ROOT);

  return 0;
}

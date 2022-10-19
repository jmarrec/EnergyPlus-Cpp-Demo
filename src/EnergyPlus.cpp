#include "EnergyPlus.hpp"

#include <EnergyPlus/api/runtime.h>
#include <EnergyPlus/api/state.h>
#include <EnergyPlus/api/func.h>
#include <EnergyPlus/api/TypeDefs.h>

namespace epcli {

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

}  // namespace epcli

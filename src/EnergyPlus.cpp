#include "EnergyPlus.hpp"

#include "ErrorMessage.hpp"            // for ErrorMessage
#include "utilities/ASCIIStrings.hpp"  // for ascii_to_lower_copy

#include <EnergyPlus/api/TypeDefs.h>  // for Error
#include <EnergyPlus/api/func.h>      // for registerErrorCallback
#include <EnergyPlus/api/runtime.h>   // for energyplus, registerStdOut/ProgressCallback, setConsoleOutputState, setEnergyPlusRootDirectory
#include <EnergyPlus/api/state.h>     // for stateDelete, stateNew, EnergyPlusState

#include <ftxui/component/event.hpp>               // for Event, Event::Custom
#include <ftxui/component/screen_interactive.hpp>  // for ScreenInteractive

#include <algorithm>    // for find
#include <atomic>       // for atomic
#include <array>        // for array
#include <memory>       // for unique_ptr
#include <string_view>  // for string_view

namespace epcli {

void runEnergyPlus(int argc, const char* argv[],  // NOLINT(modernize-avoid-c-arrays)
                   ftxui::Sender<std::string>* senderRunOutput, ftxui::Sender<ErrorMessage>* senderErrorOutput, std::atomic<int>* progress,
                   ftxui::ScreenInteractive* screen) {

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
    (*senderRunOutput)->Send(message);
    screen->PostEvent(ftxui::Event::Custom);
  });

  registerErrorCallback(state, [&senderErrorOutput, &screen](EnergyPlus::Error error, const std::string& message) {
    // fmt::print("[{}%] {}\n", progress, msg);

    (*senderErrorOutput)->Send(ErrorMessage{error, message});
    screen->PostEvent(ftxui::Event::Custom);
  });

  const int success = energyplus(state, argc, argv);
  if (success == 0) {
    *progress = 100;
  } else {
    *progress = -1;
  }
  stateDelete(state);

  screen->PostEvent(ftxui::Event::Custom);
}

static constexpr std::array<std::string_view, 4> acceptedExtensions{".epjson", ".json", ".idf", ".imf"};

bool validateFileType(const std::filesystem::path& filePath) {

  auto const filePathStr = filePath.extension().string();

  auto ext = utilities::ascii_to_lower_copy(filePathStr);
  return (std::find(acceptedExtensions.cbegin(), acceptedExtensions.cend(), ext) != acceptedExtensions.cend());
}

}  // namespace epcli

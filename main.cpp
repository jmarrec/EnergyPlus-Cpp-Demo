#include <EnergyPlus/api/runtime.h>
#include <EnergyPlus/api/state.h>
#include <EnergyPlus/api/func.h>
#include <EnergyPlus/api/TypeDefs.h>

#include <fmt/format.h>

#ifdef _WIN32
#  define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

int numWarnings = 0;
int oneTimeHalfway = 0;
static int progress = 0;

void BeginNewEnvironmentHandler(EnergyPlusState state) {
  fmt::print("CALLBACK: {}\n", __PRETTY_FUNCTION__);
  issueWarning(state, "Fake Warning at new environment");
  issueSevere(state, "Fake Severe at new environment");
  issueText(state, "Just some text at the new environment");
}

void stdOutHandler(const char* message) {
  fmt::print("STANDARD OUTPUT CALLBACK: {}\n", message);
}

void progressHandler(int const t_progress) {
  progress = t_progress;
  if (oneTimeHalfway == 0 && progress > 50) {
    fmt::print("Were halfway there!\n");
    oneTimeHalfway = 1;
  }
}

std::string formatError(EnergyPlus::Error error) {

  if (error == EnergyPlus::Error::Fatal) {
    return "Fatal";
  } else if (error == EnergyPlus::Error::Severe) {
    return "Severe";
  } else if (error == EnergyPlus::Error::Warning) {
    return "Warning";
  }

  return "";
}

int main(int argc, const char* argv[]) {

  // This is a compile definition
  fmt::print("{}\n", ENERGYPLUS_ROOT);

  EnergyPlusState state = stateNew();
  setEnergyPlusRootDirectory(state, ENERGYPLUS_ROOT);

  callbackBeginNewEnvironment(state, BeginNewEnvironmentHandler);
  registerProgressCallback(state, progressHandler);
  registerStdOutCallback(state, [](const auto& msg) { fmt::print("[{}%]: {}\n", progress, msg); });
  registerErrorCallback(state, [](EnergyPlus::Error error, const std::string& message) { fmt::print("{} - {}\n", formatError(error), message); });
  energyplus(state, argc, argv);
  if (numWarnings > 0) {
    fmt::print("There were {} warnings!\n", numWarnings);
  }

  return 0;
}

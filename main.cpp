#include <EnergyPlus/api/runtime.h>
#include <EnergyPlus/api/state.h>
#include <cstdio>
#include <cstring>

#include <fmt/format.h>

#ifdef _WIN32
#  define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

int numWarnings = 0;
int oneTimeHalfway = 0;

void BeginNewEnvironmentHandler(EnergyPlusState state) {
  fmt::print("CALLBACK: {}\n", __PRETTY_FUNCTION__);
  issueWarning(state, "Fake Warning at new environment");
  issueSevere(state, "Fake Severe at new environment");
  issueText(state, "Just some text at the new environment");
}

void stdOutHandler(const char* message) {
  fmt::print("STANDARD OUTPUT CALLBACK: {}\n", message);
}

void progressHandler(int const progress) {
  if (oneTimeHalfway == 0 && progress > 50) {
    fmt::print("Were halfway there!\n");
    oneTimeHalfway = 1;
  }
}

int main(int argc, const char* argv[]) {
  EnergyPlusState state = stateNew();
  callbackBeginNewEnvironment(state, BeginNewEnvironmentHandler);
  registerProgressCallback(state, progressHandler);
  registerStdOutCallback(state, stdOutHandler);
  // registerErrorCallback(errorHandler);
  energyplus(state, argc, argv);
  if (numWarnings > 0) {
    fmt::print("There were {} warnings!\n", numWarnings);
  }

  return 0;
}

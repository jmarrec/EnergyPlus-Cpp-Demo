#include "ErrorMessage.hpp"

std::string ErrorMessage::formatError(EnergyPlus::Error error) {
  if (error == EnergyPlus::Error::Fatal) {
    return "Fatal";
  } else if (error == EnergyPlus::Error::Severe) {
    return "Severe";
  } else if (error == EnergyPlus::Error::Warning) {
    return "Warning";
  } else if (error == EnergyPlus::Error::Info) {
    return "Info";
  } else if (error == EnergyPlus::Error::Continue) {
    return "Continue";
  }

  return "";
}

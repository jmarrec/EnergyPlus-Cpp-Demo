#include "ErrorMessage.hpp"

#include <EnergyPlus/api/TypeDefs.h>  // for Error, Error::Continue, Error::Fatal, Error::Info

#include <utility>  // for move

ErrorMessage::ErrorMessage(EnergyPlus::Error t_error, std::string t_message) : error(t_error), message(std::move(t_message)) {}

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

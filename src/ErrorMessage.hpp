#ifndef ERRORMESSAGE_HPP
#define ERRORMESSAGE_HPP

#include <EnergyPlus/api/TypeDefs.h>  // for Error

#include <string>  // for string

struct ErrorMessage
{
  ErrorMessage() = default;
  ErrorMessage(EnergyPlus::Error t_error, std::string t_message);
  EnergyPlus::Error error;
  std::string message;

  static std::string formatError(EnergyPlus::Error error);
};

#endif  // ERRORMESSAGE_HPP

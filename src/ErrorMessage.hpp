#ifndef ERRORMESSAGE_HPP
#define ERRORMESSAGE_HPP

#include <EnergyPlus/api/TypeDefs.h>

#include <string>

struct ErrorMessage
{
  EnergyPlus::Error error;
  std::string message;

  static std::string formatError(EnergyPlus::Error error);
};

#endif  // ERRORMESSAGE_HPP

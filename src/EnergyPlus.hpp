#ifndef ENERGYPLUS_HPP
#define ENERGYPLUS_HPP

#include "ErrorMessage.hpp"

#include <ftxui/component/receiver.hpp>            // For Sender
#include <ftxui/component/screen_interactive.hpp>  // For ScreenInteractive

#include <filesystem>   // For path
#include <string_view>  // For string_view

namespace epcli {
void runEnergyPlus(int argc, const char* argv[], ftxui::Sender<std::string> senderRunOutput, ftxui::Sender<ErrorMessage> senderErrorOutput,
                   std::atomic<int>* progress, ftxui::ScreenInteractive* screen);

bool validateFileType(const std::filesystem::path& filePath);

inline std::string ascii_to_lower_copy(std::string_view input) {
  std::string result{input};
  constexpr auto to_lower_diff = 'a' - 'A';
  for (auto& c : result) {
    if (c >= 'A' && c <= 'Z') {
      c += to_lower_diff;
    }
  }
  return result;
}
}  // namespace epcli
#endif  // ENERGYPLUS_HPP

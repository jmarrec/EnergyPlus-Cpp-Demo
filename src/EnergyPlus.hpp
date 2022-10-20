#ifndef ENERGYPLUS_HPP
#define ENERGYPLUS_HPP

#include "ErrorMessage.hpp"

#include <ftxui/component/receiver.hpp>            // For Sender
#include <ftxui/component/screen_interactive.hpp>  // For ScreenInteractive

#include <filesystem>   // For path
#include <string_view>  // For string_view

namespace epcli {
void runEnergyPlus(int argc, const char* argv[], ftxui::Sender<std::string>* senderRunOutput, ftxui::Sender<ErrorMessage>* senderErrorOutput,
                   std::atomic<int>* progress, ftxui::ScreenInteractive* screen);

bool validateFileType(const std::filesystem::path& filePath);

}  // namespace epcli
#endif  // ENERGYPLUS_HPP

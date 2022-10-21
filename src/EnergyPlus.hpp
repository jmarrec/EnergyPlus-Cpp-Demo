#ifndef ENERGYPLUS_HPP
#define ENERGYPLUS_HPP

#include <ftxui/component/receiver.hpp>  // For Sender

#include <atomic>      // for atomic
#include <filesystem>  // for path
#include <string>      // for string

struct ErrorMessage;

namespace ftxui {
class ScreenInteractive;
}

namespace epcli {
void runEnergyPlus(int argc, const char* argv[], ftxui::Sender<std::string>* senderRunOutput, ftxui::Sender<ErrorMessage>* senderErrorOutput,
                   std::atomic<int>* progress, ftxui::ScreenInteractive* screen);

bool validateFileType(const std::filesystem::path& filePath);

}  // namespace epcli
#endif  // ENERGYPLUS_HPP

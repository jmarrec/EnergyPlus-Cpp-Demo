#include "MainComponent.hpp"

#include "sqlite/SQLiteReports.hpp"       // for SQLiteComponent
#include "utilities/ASCIIStrings.hpp"     // for ascii_trim
                                          //
#include <EnergyPlus/api/TypeDefs.h>      // for Error
                                          //
#include <ftxui/component/component.hpp>  // for Horizontal, Vertical, Tab, Button, Checkbox
#include <ftxui/component/event.hpp>      // for Event
#include <ftxui/dom/elements.hpp>         // for text, separator, operator|, color, filler, Element, hcenter, gauge, hbox, spinner, vbox, notflex
#include <ftxui/screen/color.hpp>         // for Color
                                          //
#include <ctre.hpp>                       // for CTRE
                                          //
#include <fmt/format.h>                   // for formatting
#include <fmt/std.h>                      // for formatting std::filesystem::path
                                          //
#include <algorithm>                      // for max, min
#include <chrono>                         // for filesystem
#include <cstdlib>                        // for system
#include <filesystem>                     // path, operator/, is_regular_file, weakly_canonical
#include <fstream>                        // for ifstream
#include <set>                            // for set
#include <string>                         // for string, to_string, char_traits, getline
#include <string_view>                    // for operator==, basic_string_view

using namespace ftxui;
namespace fs = std::filesystem;

static constexpr auto programName = "EnergyPlus-Cpp-Demo";

MainComponent::MainComponent(Receiver<std::string> receiverRunOutput, Receiver<ErrorMessage> receiverErrorOutput, Component runButton,
                             Component quitButton, std::atomic<int>* progress, std::filesystem::path outputDirectory)
  : m_receiverRunOutput(std::move(receiverRunOutput)),
    m_receiverErrorOutput(std::move(receiverErrorOutput)),
    m_runButton(std::move(runButton)),
    m_quitButton(std::move(quitButton)),
    m_progress(progress),
    m_outputDirectory(std::move(outputDirectory)) {

  m_openHTMLButton = Button(
    &m_outputHTMLButtonText,
    [this]() {
      fs::path outputPath = m_outputDirectory / "eplustbl.htm";
      if (fs::is_regular_file(outputPath)) {
#if __linux__
        std::string cmd = fmt::format("xdg-open {} &", outputPath);
#elif __APPLE__
        std::string cmd = fmt::format("open {} &", outputPath);
#elif _WIN32
        std::string cmd = fmt::format("start {} &", outputPath);
#endif
        [[maybe_unused]] auto result = std::system(cmd.c_str());
      }
    },
    ButtonOption::Simple());

  Add(  //
    Container::Vertical({
      Container::Horizontal({
        toggle_,
        m_quitButton,
      }),
      Container::Tab(
        {
          // Stdout / Run
          Container::Vertical({
            Container::Horizontal({
              m_runButton,
              m_openHTMLButton,
              m_clearResultsButton,
            }),
            m_stdout_displayer,
          }),
          // eplusout.err
          Container::Vertical({
            container_level_filter_,
            m_error_displayer,
          }),
          // Sqlite reports
          m_sqlite_component,
          // About
          info_component_,
        },
        &tab_selected_),
    }));
}

void MainComponent::clear_state() {
  m_stdout_lines.clear();
  m_errors.clear();
  *m_progress = 0;
  m_numWarnings = 0;
  m_numSeveres = 0;
  m_hasAlreadyRun = false;
}

void MainComponent::reload_results() {
  clear_state();

  m_stdout_lines.emplace_back("=========================================");
  m_stdout_lines.emplace_back("   Results have been reloaded from disk");
  m_stdout_lines.emplace_back("=========================================");
  m_stdout_lines.emplace_back("");

  std::ifstream ifs(m_outputDirectory / "eplusout.err");

  // matches[1], warning/error type
  // matches[2], rest of line
  auto warningOrErrorMatcher = ctre::match<R"(^\s*\**\s+\*\*\s*([[:alpha:]]+)\s*\*\*(.*)$)">;

  // matches[1], rest of line
  auto warningOrErrorContinueMatcher = ctre::match<R"(^\s*\**\s+\*\*\s*~~~\s*\*\*(.*)$)">;

  // completed successfully
  auto completedSuccessfulMatcher = ctre::match<R"(^\s*\*+ EnergyPlus Completed Successfully.*)">;

  // ground temp completed successfully
  auto groundTempCompletedSuccessfulMatcher = ctre::match<R"(^\s*\*+ GroundTempCalc\S* Completed Successfully.*)">;

  // completed unsuccessfully
  auto completedUnsuccessfulMatcher = ctre::match<R"(^\s*\*+ EnergyPlus Terminated.*)">;

  // repeat count

  // read the file line by line using regexes
  std::string line;

  while (std::getline(ifs, line)) {

    EnergyPlus::Error errorType = EnergyPlus::Error::Info;

    std::string message;

    if (completedSuccessfulMatcher(line) || groundTempCompletedSuccessfulMatcher(line)) {
      *m_progress = 100;
      m_stdout_lines.emplace_back(std::move(line));
      m_hasAlreadyRun = true;
      continue;
    } else if (completedUnsuccessfulMatcher(line)) {
      *m_progress = -1;
      m_stdout_lines.emplace_back(std::move(line));
      m_hasAlreadyRun = false;
      continue;
    }

    // parse the file
    if (auto [whole, warningOrErrorType, msg] = warningOrErrorMatcher(line); whole) {

      std::string_view warningOrErrorTypeTrim = utilities::ascii_trim(warningOrErrorType);
      message = msg;

      if (warningOrErrorTypeTrim == "Fatal") {
        errorType = EnergyPlus::Error::Fatal;
      } else if (warningOrErrorTypeTrim == "Severe") {
        errorType = EnergyPlus::Error::Severe;
        ++m_numSeveres;
      } else if (warningOrErrorTypeTrim == "Warning") {
        errorType = EnergyPlus::Error::Warning;
        ++m_numWarnings;
      }
    } else if (auto [whole, msg] = warningOrErrorContinueMatcher(line); whole) {  // cppcheck-suppress shadowVariable
      errorType = EnergyPlus::Error::Continue;
      message = msg;
    } else {
      errorType = EnergyPlus::Error::Info;
      message = line;
    }

    utilities::ascii_trim(message);
    RegisterLogLevel(errorType);
    m_errors.emplace_back(errorType, std::move(message));
  }
}

bool MainComponent::hasAlreadyRun() const {
  return m_hasAlreadyRun;
}

bool MainComponent::OnEvent(Event event) {
  while (m_receiverRunOutput->HasPending()) {
    std::string line;
    m_receiverRunOutput->Receive(&line);
    m_stdout_lines.emplace_back(std::move(line));
    m_stdout_displayer->setSelected(m_stdout_lines.size());
    m_stdout_displayer->TakeFocus();
  }

  while (m_receiverErrorOutput->HasPending()) {
    ErrorMessage errorMsg;
    m_receiverErrorOutput->Receive(&errorMsg);
    ProcessErrorMessage(std::move(errorMsg));
  }
  return ComponentBase::OnEvent(event);
}

void MainComponent::ProcessErrorMessage(ErrorMessage&& errorMsg) {
  if (errorMsg.error == EnergyPlus::Error::Warning) {
    ++m_numWarnings;
  }
  if (errorMsg.error == EnergyPlus::Error::Severe) {
    ++m_numWarnings;
  }
  RegisterLogLevel(errorMsg.error);
  m_errors.emplace_back(errorMsg);
}

void MainComponent::RegisterLogLevel(EnergyPlus::Error log_level) {
  if (log_level == EnergyPlus::Error::Continue || level_checkbox.contains(log_level)) {
    return;
  }

  level_checkbox[log_level] = true;
  container_level_filter_->Add(Checkbox(ErrorMessage::formatError(log_level), &level_checkbox[log_level]));
}

Element MainComponent::Render() {
  static int i = 0;

  int current_line = (std::min(tab_selected_, 1) == 0 ? m_stdout_displayer : m_error_displayer)->selected();

  if (tab_selected_ == 0) {

    auto header = hbox({
      text(programName),
      filler(),
      separator(),
      hcenter(toggle_->Render()) | color(Color::Yellow),
      separator(),
      text(std::to_string(current_line + 1)),
      text("/"),
      text(std::to_string(m_stdout_lines.size())),
      separator(),
      gauge(float(current_line) / float(std::max(1, (int)m_stdout_lines.size() - 1))) | color(Color::GrayDark),
      separator(),
      spinner(5, i++),
      m_quitButton->Render(),
    });

    auto runRow = ftxui::hbox({
      filler(),
      m_runButton->Render() |                                                                    //
        ((*m_progress > 0 && *m_progress < 100) ? color(Color::GrayDark) : color(Color::Green))  //
        | ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 20),
      filler(),
      (*m_progress == 100) ? m_openHTMLButton->Render() : text(""),
      (*m_progress == 100) ? m_clearResultsButton->Render() : text(""),
    });

    auto run_gaugeLabel = [this]() {
      if (*m_progress == 100) {
        m_hasAlreadyRun = true;
        return ftxui::text("Done") | color(Color::Green) | bold;
      } else if (*m_progress > 0) {
        return ftxui::text("Running") | color(Color::Yellow);
      } else if (*m_progress < 0) {
        return ftxui::text("Failed") | color(Color::Red) | bold;
      } else {
        return ftxui::text("Pending") | color(Color::Blue);
      }
    };

    auto runGaugeRow = ftxui::hbox({
      text("Status"),
      separator(),
      hcenter(run_gaugeLabel()) | ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 20),
      separator(),
      ftxui::gauge(*m_progress / 100.f) | ftxui::flex,
      ftxui::text(fmt::format("{} %", *m_progress)),
      separator(),
      text(fmt::format("{} warnings", m_numWarnings)) | ((m_numWarnings > 0) ? color(Color::Yellow) : color(Color::GrayLight)),
      separator(),
      text(fmt::format("{} severes", m_numSeveres)) | ((m_numWarnings > 0) ? color(Color::Red) : color(Color::GrayLight)),

    });

    // Stdout
    return  //
      vbox({
        header,
        separator(),
        runRow,
        separator(),
        runGaugeRow,
        separator(),
        m_stdout_displayer->RenderLines(m_stdout_lines) | flex_shrink,
        filler(),
      });
  }

  if (tab_selected_ == 1) {
    // eplusout.err

    std::set<EnergyPlus::Error> allowed_level;
    for (auto& [level, state] : level_checkbox) {
      if (state) {
        allowed_level.insert(level);
      }
    }

    std::vector<ErrorMessage*> filtered_errorMsgs;
    filtered_errorMsgs.reserve(m_errors.size());

    bool prevAllowed = false;

    for (auto& m_error : m_errors) {
      if (allowed_level.contains(m_error.error)) {
        filtered_errorMsgs.push_back(&m_error);
        prevAllowed = true;
      } else if (m_error.error == EnergyPlus::Error::Continue && prevAllowed) {
        filtered_errorMsgs.push_back(&m_error);
      }
    }

    auto headerError = hbox({
      text(programName),
      filler(),
      separator(),
      hcenter(toggle_->Render()) | color(Color::Red),
      separator(),
      text(std::to_string(current_line)),
      text("/"),
      text(std::to_string(filtered_errorMsgs.size())),
      text("  ["),
      text(std::to_string(m_errors.size())),
      text("]"),
      separator(),
      gauge(float(current_line) / float(std::max(1, (int)filtered_errorMsgs.size() - 1))) | color(Color::GrayDark),
      separator(),
      spinner(5, i++),
      m_quitButton->Render(),
    });

    return  //
      vbox({
        headerError,
        separator(),
        hbox({
          window(text("Type"), container_level_filter_->Render()) | notflex,
          filler(),
        }) | notflex,
        m_error_displayer->RenderLines(filtered_errorMsgs) | flex_shrink,
      });
  }

  if (tab_selected_ == 2) {

    auto header = hbox({
      text(programName),
      filler(),
      separator(),
      hcenter(toggle_->Render()) | color(Color::Yellow),
      separator(),
      text(std::to_string(current_line + 1)),
      text("/"),
      text(std::to_string(m_stdout_lines.size())),
      separator(),
      gauge(float(current_line) / float(std::max(1, (int)m_stdout_lines.size() - 1))) | color(Color::GrayDark),
      separator(),
      spinner(5, i++),
      m_quitButton->Render(),
    });

    Element content = text("NOTHING TO SHOW");

    if (*m_progress == 100) {
      fs::path databasePath = m_outputDirectory / "eplusout.sql";
      if (fs::is_regular_file(databasePath)) {
        content = m_sqlite_component->RenderDatabase(std::move(databasePath));
      } else {
        content = vbox({
          text(fmt::format("The Run appears to have been successful but I cannot find the SQLFile at {}", fs::weakly_canonical(databasePath))),
          text("Try adding the following to your IDF:"),
          separator(),
          text("  Output:SQLite,"),
          text("     SimpleAndTabular;"),
          separator(),
        });
      }
    }

    return  //
      vbox({
        header,
        separator(),
        filler(),
        content | center,
        filler(),
      });
  }

  // About

  auto header = hbox({
    text(programName),
    filler(),
    separator(),
    hcenter(toggle_->Render()) | color(Color::Blue),
    separator(),
    filler(),
    spinner(5, i++),
    m_quitButton->Render(),
  });

  return  //
    vbox({
      header,
      separator(),
      filler(),
      info_component_->Render() | center,
      filler(),
    });
}

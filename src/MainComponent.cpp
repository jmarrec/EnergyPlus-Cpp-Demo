#include "MainComponent.hpp"
#include "sqlite/SQLiteReports.hpp"
#include "utilities/ASCIIStrings.hpp"

#include <EnergyPlus/api/TypeDefs.h>

#include <ftxui/dom/elements.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/screen/string.hpp>

#include <fmt/format.h>
#include <fmt/std.h>

#include <filesystem>
#include <fstream>
#include <utility>
#include <set>
#include <string>
#include <regex>

using namespace ftxui;
namespace fs = std::filesystem;

static constexpr auto programName = L"EnergyPlus-Cpp-Demo";

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

  std::ifstream ifs(m_outputDirectory / "eplusout.err");

  // matches[1], warning/error type
  // matches[2], rest of line
  std::regex warningOrError(R"(^\s*\**\s+\*\*\s*([[:alpha:]]+)\s*\*\*(.*)$)");

  // matches[1], rest of line
  std::regex warningOrErrorContinue(R"(^\s*\**\s+\*\*\s*~~~\s*\*\*(.*)$)");

  // completed successfully
  std::regex completedSuccessful("^\\s*\\*+ EnergyPlus Completed Successfully.*");

  // ground temp completed successfully
  std::regex groundTempCompletedSuccessful(R"(^\s*\*+ GroundTempCalc\S* Completed Successfully.*)");

  // completed unsuccessfully
  std::regex completedUnsuccessful("^\\s*\\*+ EnergyPlus Terminated.*");

  // repeat count

  // read the file line by line using regexes
  std::string line;

  while (std::getline(ifs, line)) {

    std::smatch matches;

    EnergyPlus::Error errorType = EnergyPlus::Error::Info;

    std::string message;

    if (std::regex_match(line, completedSuccessful) || std::regex_match(line, groundTempCompletedSuccessful)) {
      *m_progress = 100;
      m_stdout_lines.emplace_back(std::move(line));
      m_hasAlreadyRun = true;
      continue;
    } else if (std::regex_match(line, completedUnsuccessful)) {
      *m_progress = -1;
      m_stdout_lines.emplace_back(std::move(line));
      m_hasAlreadyRun = false;
      continue;
    }

    // parse the file
    if (std::regex_search(line, matches, warningOrError)) {

      std::string warningOrErrorType = std::string(matches[1].first, matches[1].second);
      utilities::ascii_trim(warningOrErrorType);
      message = std::string(matches[2].first, matches[2].second);

      if (warningOrErrorType == "Fatal") {
        errorType = EnergyPlus::Error::Fatal;
      } else if (warningOrErrorType == "Severe") {
        errorType = EnergyPlus::Error::Severe;
        ++m_numSeveres;
      } else if (warningOrErrorType == "Warning") {
        errorType = EnergyPlus::Error::Warning;
        ++m_numWarnings;
      }
    } else if (std::regex_search(line, matches, warningOrErrorContinue)) {
      errorType = EnergyPlus::Error::Continue;
      message = std::string(matches[1].first, matches[1].second);
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
      text(to_wstring(current_line + 1)),
      text(L"/"),
      text(to_wstring(m_stdout_lines.size())),
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
      text(to_wstring(current_line)),
      text(L"/"),
      text(to_wstring(filtered_errorMsgs.size())),
      text(L"  ["),
      text(to_wstring(m_errors.size())),
      text(L"]"),
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
          window(text(L"Type"), container_level_filter_->Render()) | notflex,
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
      text(to_wstring(current_line + 1)),
      text(L"/"),
      text(to_wstring(m_stdout_lines.size())),
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

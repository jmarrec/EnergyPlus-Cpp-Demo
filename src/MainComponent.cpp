#include "MainComponent.hpp"

#include <EnergyPlus/api/TypeDefs.h>

#include <ftxui/dom/elements.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/screen/string.hpp>

#include <fmt/format.h>
#include <fmt/std.h>

#include <filesystem>
#include <utility>
#include <set>

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
            }),
            m_stdout_displayer,
          }),
          // eplusout.err
          Container::Vertical({
            container_level_filter_,
            m_error_displayer,
          }),
          // About
          info_component_,
        },
        &tab_selected_),
    }));
}

void MainComponent::clear_state() {
  m_stdout_lines.clear();
  m_errors.clear();
  m_numWarnings = 0;
  m_numSeveres = 0;
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
    });

    auto run_gaugeLabel = [this]() {
      if (*m_progress == 100) {
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

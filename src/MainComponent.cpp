#include "MainComponent.hpp"

#include <TypeDefs.h>
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/screen/string.hpp>
#include <set>
#include <fmt/format.h>

using namespace ftxui;

MainComponent::MainComponent(Receiver<std::string> receiverRunOutput, Receiver<ErrorMessage> receiverErrorOutput, Component runButton,
                             Component quitButton, std::atomic<int>* progress)
  : m_receiverRunOutput(std::move(receiverRunOutput)),
    m_receiverErrorOutput(std::move(receiverErrorOutput)),
    m_runButton(std::move(runButton)),
    m_quitButton(std::move(quitButton)),
    m_progress(progress) {

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
            m_runButton,
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
      text(L"EnergyPlus-Cpp-Demo"),
      filler(),
      separator(),
      hcenter(toggle_->Render()),
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
      m_runButton->Render() | ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 20),
    });

    auto runGaugeRow = ftxui::hbox({
      ftxui::text(m_runGaugeText),
      ftxui::gauge(*m_progress / 100.f) | ftxui::flex,
      ftxui::text(fmt::format("{} %", *m_progress)),
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
      text(L"EnergyPlus-Cpp-Demo"),
      filler(),
      separator(),
      hcenter(toggle_->Render()),
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
    text(L"EnergyPlus-Cpp-Demo"),
    filler(),
    separator(),
    hcenter(toggle_->Render()),
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

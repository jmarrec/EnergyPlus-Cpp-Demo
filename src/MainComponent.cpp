#include "MainComponent.hpp"

#include <TypeDefs.h>
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/screen/string.hpp>
#include <set>

using namespace ftxui;

MainComponent::MainComponent(Receiver<std::string> receiverRunOutput, Receiver<ErrorMessage> receiverErrorOutput)
  : m_receiverRunOutput(std::move(receiverRunOutput)), m_receiverErrorOutput(std::move(receiverErrorOutput)) {
  Add(Container::Vertical({
    toggle_,
    Container::Tab(
      {
        m_stdout_displayer,
        Container::Vertical({
          Container::Horizontal({
            container_level_filter_,
          }),
          m_error_displayer,
        }),
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
  }

  while (m_receiverErrorOutput->HasPending()) {
    ErrorMessage errorMsg;
    m_receiverErrorOutput->Receive(&errorMsg);
    ProcessErrorMessage(std::move(errorMsg));
  }
  return ComponentBase::OnEvent(event);
}

void MainComponent::ProcessErrorMessage(ErrorMessage&& errorMsg) {
  // TODO: re-enable after testing
  // if (errorMsg.error == EnergyPlus::Error::Continue) {
  //   m_errors.back().message += errorMsg.message;
  // } else {
  RegisterLogLevel(errorMsg.error);
  m_errors.emplace_back(errorMsg);
  // }
}

void MainComponent::RegisterLogLevel(EnergyPlus::Error log_level) {
  if (level_checkbox.contains(log_level)) {
    return;
  }

  level_checkbox[log_level] = true;
  container_level_filter_->Add(Checkbox(ErrorMessage::formatError(log_level), &level_checkbox[log_level]));
}

Element MainComponent::Render() {
  static int i = 0;
  std::set<EnergyPlus::Error> allowed_level;
  for (auto& [level, state] : level_checkbox) {
    if (state) {
      allowed_level.insert(level);
    }
  }

  std::vector<ErrorMessage*> filtered_errorMsgs;
  filtered_errorMsgs.reserve(m_errors.size());

  for (auto& m_error : m_errors) {
    if (allowed_level.contains(m_error.error)) {
      filtered_errorMsgs.push_back(&m_error);
    }
  }

  int current_line = (std::min(tab_selected_, 1) == 0 ? m_stdout_displayer : m_error_displayer)->selected();

  auto header = hbox({
    text(L"EnergyPlus-Cpp-Demo"),
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
  });

  Element tab_menu;

  if (tab_selected_ == 0) {
    return  //
      vbox({
        header,
        m_stdout_displayer->RenderLines(m_stdout_lines) | flex_shrink,
        filler(),
      });
  }

  if (tab_selected_ == 1) {
    return  //
      vbox({
        header,
        separator(),
        hbox({
          window(text(L"Type"), container_level_filter_->Render()) | notflex,
          filler(),
        }) | notflex,
        m_error_displayer->RenderLines(filtered_errorMsgs) | flex_shrink,
      });
  }

  return  //
    vbox({
      header,
      separator(),
      filler(),
      info_component_->Render() | center,
      filler(),
    });
}

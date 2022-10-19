#include "LogDisplayer.hpp"
#include "ErrorMessage.hpp"

#include <TypeDefs.h>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/string.hpp>
#include <ftxui/component/event.hpp>
#include <map>

namespace {
struct LogStyle
{
  Decorator level_decorator;
  Decorator line_decorator;
};

std::map<EnergyPlus::Error, LogStyle> log_style = {
  {EnergyPlus::Error::Continue, {nothing, dim}},
  {EnergyPlus::Error::Fatal, {color(Color::Red), bold}},
  {EnergyPlus::Error::Severe, {color(Color::RedLight), bold}},
  {EnergyPlus::Error::Warning, {color(Color::Yellow), nothing}},
  {EnergyPlus::Error::Info, {color(Color::Blue), dim}},
};
}  // namespace

Element LogDisplayer::RenderLines(std::vector<ErrorMessage*> lines) {
  m_size = lines.size();

  Elements list;
  int size_level = 15;
  int size_message = 10;

  for (auto& it : lines) {
    size_message = std::max(size_message, (int)it->message.size());
  }
  size_message += 5;

  auto header = hbox({
    text(L"Type") | ftxui::size(WIDTH, EQUAL, size_level), separator(),
    text(L"Message") | flex,  // Or instead of flex: | ftxui::size(WIDTH, EQUAL, size_message),
  });

  int previous_type = !lines.empty() ? static_cast<int>(lines[0]->error) : -1;

  int index = 0;
  for (auto& it : lines) {
    bool is_focus = (index++ == m_selected);
    if (it->error != EnergyPlus::Error::Continue && previous_type != static_cast<int>(it->error)) {
      list.push_back(separator());
    }
    previous_type = static_cast<int>(it->error);

    Decorator line_decorator = log_style[it->error].line_decorator;
    Decorator level_decorator = log_style[it->error].level_decorator;

    if (is_focus) {
      line_decorator = line_decorator | focus;
      if (Focused()) {
        line_decorator = line_decorator | focus | inverted;
      }
    }

    Element document =  //
      hbox({
        text(/*it->error == EnergyPlus::Error::Continue ? "" : */ ErrorMessage::formatError(it->error))  //
          | ftxui::size(WIDTH, EQUAL, size_level)                                                        //
          | level_decorator                                                                              //
        ,
        separator(),
        text(it->message)  //
          | flex,
      })
      | flex | line_decorator;
    list.push_back(document);
  }

  if (list.empty()) {
    list.push_back(text(L"(empty)"));
  }

  return window(text(L"Log"), vbox({
                                header,
                                separator(),
                                vbox(list) | vscroll_indicator | yframe | reflect(box_),
                              }));
}

Element LogDisplayer::RenderLines(const std::vector<std::string>& lines) {
  m_size = lines.size();

  Elements list;
  int size_message = 10;

  for (const auto& l : lines) {
    size_message = std::max(size_message, static_cast<int>(l.size()));
  }
  size_message += 5;

  auto header = hbox({
    text(L"Message") | flex,  // Or instead of flex: | ftxui::size(WIDTH, EQUAL, size_message),
  });

  int index = 0;
  for (const auto& line : lines) {
    bool is_focus = (index++ == m_selected);

    Decorator line_decorator = nothing;

    if (is_focus) {
      line_decorator = line_decorator | focus;
      if (Focused()) {
        line_decorator = line_decorator | focus | inverted;
      }
    }

    Element document = text(line) | flex | line_decorator;
    list.push_back(document);
  }

  if (list.empty()) {
    list.push_back(text(L"(empty)"));
  }

  return window(text(L"Log"), vbox({
                                header,
                                separator(),
                                vbox(list) | vscroll_indicator | yframe | reflect(box_),
                              }));
}

int LogDisplayer::selected() const {
  return m_selected;
}

void LogDisplayer::incrementSelected() {
  ++m_selected;
}

void LogDisplayer::setSelected(int index) {
  m_selected = index;
}

bool LogDisplayer::OnEvent(Event event) {
  // if (!Focused()) {
  //   return false;
  // }
  if (event.is_mouse() && box_.Contain(event.mouse().x, event.mouse().y)) {
    TakeFocus();
  }

  int old_selected = m_selected;
  if (event == Event::ArrowUp || event == Event::Character('k') || (event.is_mouse() && event.mouse().button == Mouse::WheelUp)) {
    --m_selected;
  }
  if (event == Event::ArrowDown || event == Event::Character('j') || (event.is_mouse() && event.mouse().button == Mouse::WheelDown)) {
    ++m_selected;
  }
  if (event == Event::Tab && (m_size != 0)) {
    m_selected += m_size / 10;
  }
  if (event == Event::TabReverse && (m_size != 0)) {
    m_selected -= m_size / 10;
  }
  if (event == Event::Home) {
    m_selected = 0;
  }
  if (event == Event::End) {
    m_selected = m_size;
  }

  m_selected = std::max(0, std::min(m_size - 1, m_selected));

  return m_selected != old_selected;
}

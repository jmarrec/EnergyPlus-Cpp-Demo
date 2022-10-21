#include "LogDisplayer.hpp"
#include "ErrorMessage.hpp"  // for ErrorMessage

#include <EnergyPlus/api/TypeDefs.h>  // for Error, Error::Continue, Error::Fatal, Error::Info

#include <ftxui/component/event.hpp>  // for Event, Event::ArrowDown, Event:...
#include <ftxui/component/mouse.hpp>  // for Mouse, Mouse::WheelDown, Mouse:...
#include <ftxui/dom/elements.hpp>     // for text, operator|, color, Element, Decorator
#include <ftxui/screen/box.hpp>       // for Box
#include <ftxui/screen/color.hpp>     // for Color, Color::Blue, Color::Red

#include <algorithm>  // for max, min
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

  Elements elementList;
  const int size_level = 15;
  [[maybe_unused]] int size_message = 10;

  for (auto& it : lines) {
    size_message = std::max(size_message, (int)it->message.size());
  }
  size_message += 5;

  auto header = hbox({
    text("Type") | ftxui::size(WIDTH, EQUAL, size_level), separator(),
    text("Message") | flex,  // Or instead of flex: | ftxui::size(WIDTH, EQUAL, size_message),
  });

  int previous_type = !lines.empty() ? static_cast<int>(lines[0]->error) : -1;

  int index = 0;
  for (auto& it : lines) {
    const bool is_focus = (index++ == m_selected);
    if (it->error != EnergyPlus::Error::Continue && previous_type != static_cast<int>(it->error)) {
      elementList.push_back(separator());
    }
    previous_type = static_cast<int>(it->error);

    Decorator line_decorator = log_style[it->error].line_decorator;
    const Decorator level_decorator = log_style[it->error].level_decorator;

    if (is_focus) {
      line_decorator = line_decorator | focus;
      if (Focused()) {
        line_decorator = line_decorator | focus | inverted;
      }
    }

    elementList.emplace_back(  //
      hbox({
        text(/*it->error == EnergyPlus::Error::Continue ? "" : */ ErrorMessage::formatError(it->error))  //
          | ftxui::size(WIDTH, EQUAL, size_level)                                                        //
          | level_decorator                                                                              //
        ,
        separator(),
        text(it->message)  //
          | flex,
      })
      | flex | line_decorator);
  }

  if (elementList.empty()) {
    elementList.push_back(text("(empty)"));
  }

  return window(text("Log"), vbox({
                               header,
                               separator(),
                               vbox(elementList) | vscroll_indicator | yframe | reflect(box_),
                             }));
}

Element LogDisplayer::RenderLines(const std::vector<std::string>& lines) {
  m_size = lines.size();

  Elements elementList;
  [[maybe_unused]] int size_message = 10;

  for (const auto& l : lines) {
    size_message = std::max(size_message, static_cast<int>(l.size()));
  }
  size_message += 5;

  auto header = hbox({
    text("Message") | flex,  // Or instead of flex: | ftxui::size(WIDTH, EQUAL, size_message),
  });

  int index = 0;
  for (const auto& line : lines) {
    const bool is_focus = (index++ == m_selected);

    Decorator line_decorator = nothing;

    if (is_focus) {
      line_decorator = line_decorator | focus;
      if (Focused()) {
        line_decorator = line_decorator | focus | inverted;
      }
    }

    elementList.emplace_back(text(line) | flex | line_decorator);
  }

  if (elementList.empty()) {
    elementList.push_back(text("(empty)"));
  }

  return window(text("Log"), vbox({
                               header,
                               separator(),
                               vbox(elementList) | vscroll_indicator | yframe | reflect(box_),
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

  const int old_selected = m_selected;
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
  if (event == Event::PageDown) {
    m_selected += box_.y_max - box_.y_min;
  }
  if (event == Event::PageUp) {
    m_selected -= box_.y_max - box_.y_min;
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

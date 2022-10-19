#ifndef LOG_DISPLAYER_HPP
#define LOG_DISPLAYER_HPP

#include <ftxui/component/component.hpp>

#include "ErrorMessage.hpp"

#include <vector>

using namespace ftxui;

class LogDisplayer : public ComponentBase
{
 public:
  LogDisplayer() = default;
  Element RenderLines(std::vector<ErrorMessage*> lines);
  Element RenderLines(const std::vector<std::string>& lines);
  bool OnEvent(Event event) override;
  int selected() const;
  void incrementSelected();
  void setSelected(int index);

  virtual bool Focusable() const override {
    return true;
  };

 private:
  int m_selected = 0;
  int m_size = 0;
};

#endif  // LOG_DISPLAYER_HPP

#ifndef LOG_DISPLAYER_HPP
#define LOG_DISPLAYER_HPP

#include <ftxui/component/component_base.hpp>  // For ComponentBase
#include "ftxui/screen/box.hpp"                // For Box

#include "ErrorMessage.hpp"

#include <vector>

using namespace ftxui;

class LogDisplayer : public ComponentBase
{
 public:
  LogDisplayer() = default;
  Element RenderLines(std::vector<ErrorMessage*> lines);
  Element RenderLines(const std::vector<std::string>& lines);
  bool OnEvent(Event event) override final;
  int selected() const;
  void incrementSelected();
  void setSelected(int index);

  virtual bool Focusable() const override final {
    return true;
  };

 private:
  int m_selected = 0;
  int m_size = 0;
  Box box_;
};

#endif  // LOG_DISPLAYER_HPP

#ifndef UI_MAIN_COMPONENT_HPP
#define UI_MAIN_COMPONENT_HPP

#include <ftxui/component/component.hpp>
#include <ftxui/component/receiver.hpp>

#include "AboutComponent.hpp"
#include "ErrorMessage.hpp"
#include "LogDisplayer.hpp"

#include <map>
#include <string>
#include <vector>

using namespace ftxui;

class MainComponent : public ComponentBase
{
 public:
  MainComponent(Receiver<std::string> receiverRunOutput, Receiver<ErrorMessage> receiverErrorOutput);
  Element Render() override;
  bool OnEvent(Event event) override;

 private:
  Receiver<std::string> m_receiverRunOutput;
  Receiver<ErrorMessage> m_receiverErrorOutput;

  std::vector<std::string> m_stdout_lines;

  void ProcessErrorMessage(ErrorMessage&& errorMsg);
  std::vector<ErrorMessage> m_errors;

  void RegisterLogLevel(EnergyPlus::Error log_level);
  std::map<EnergyPlus::Error, bool> level_checkbox;

  int tab_selected_ = 0;
  std::vector<std::string> tab_entries_ = {
    "Stdout",
    "eplusout.err",
    "About",
  };
  Component toggle_ = Toggle(&tab_entries_, &tab_selected_);

  Component container_level_filter_ = Container::Vertical({});
  std::shared_ptr<LogDisplayer> m_error_displayer = Make<LogDisplayer>();
  std::shared_ptr<LogDisplayer> m_stdout_displayer = Make<LogDisplayer>();
  Component info_component_ = Make<AboutComponent>();
};

#endif /* end of include guard: UI_MAIN_COMPONENT_HPP */

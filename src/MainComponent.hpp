#ifndef MAIN_COMPONENT_HPP
#define MAIN_COMPONENT_HPP

#include "AboutComponent.hpp"                     // for AboutComponent
#include "ErrorMessage.hpp"                       // for ErrorMessage
#include "LogDisplayer.hpp"                       // for LogDisplayer
#include "sqlite/SQLiteReports.hpp"               // for SQLiteComponent
                                                  //
#include <EnergyPlus/api/TypeDefs.h>              // for Error
                                                  //
#include <ftxui/component/component.hpp>          // for Make, Button, Toggle
#include <ftxui/component/component_base.hpp>     // for ComponentBase
#include <ftxui/component/component_options.hpp>  // for ButtonOption
#include <ftxui/component/event.hpp>              // for Event
#include <ftxui/component/receiver.hpp>           // for Receiver
#include <ftxui/dom/elements.hpp>                 // for Element
                                                  //
#include <atomic>                                 // for atomic
#include <filesystem>                             // for path
#include <map>                                    // for map
#include <memory>                                 // for shared_ptr
#include <string>                                 // for string, allocator
#include <vector>                                 // for vector

using namespace ftxui;

class MainComponent : public ComponentBase
{
 public:
  MainComponent(Receiver<std::string> receiverRunOutput, Receiver<ErrorMessage> receiverErrorOutput, Component runButton, Component quitButton,
                std::atomic<int>* progress, std::filesystem::path outputDirectory);
  Element Render() override;
  bool OnEvent(Event event) override;

  void clear_state();
  bool hasAlreadyRun() const;

  void reload_results();

 private:
  Receiver<std::string> m_receiverRunOutput;
  Receiver<ErrorMessage> m_receiverErrorOutput;

  std::vector<std::string> m_stdout_lines;

  void ProcessErrorMessage(ErrorMessage&& errorMsg);
  std::vector<ErrorMessage> m_errors;
  unsigned m_numSeveres = 0;
  unsigned m_numWarnings = 0;
  void RegisterLogLevel(EnergyPlus::Error log_level);
  std::map<EnergyPlus::Error, bool> level_checkbox;

  bool m_hasAlreadyRun = false;

  // std::function<void()> onRunClicked;
  // std::string run_text = "Run";
  Component m_runButton;  // = Button(&run_text, onRunClicked, ftxui::ButtonOption::Ascii());

  // std::string quit_text = "Quit";
  // std::function<void()> onQuitClicked;
  Component m_quitButton;  // = ftxui::Button(&quit_text, onQuitClicked, ftxui::ButtonOption::Ascii());

  std::string m_runGaugeText = "Pending";

  std::atomic<int>* m_progress;

  int tab_selected_ = 0;
  std::vector<std::string> tab_entries_ = {
    "Stdout",
    "eplusout.err",
    "SQL Reports",
    "About",
  };

  Component toggle_ = Toggle(&tab_entries_, &tab_selected_);

  Component container_level_filter_ = Container::Vertical({});
  std::shared_ptr<LogDisplayer> m_error_displayer = Make<LogDisplayer>();
  std::shared_ptr<LogDisplayer> m_stdout_displayer = Make<LogDisplayer>();
  Component info_component_ = Make<AboutComponent>();

  std::string m_outputHTMLButtonText = "Open HTML";
  Component m_openHTMLButton;
  std::filesystem::path m_outputDirectory;

  std::string m_clearResultsButtonText = "Clear Results";
  Component m_clearResultsButton = Button(
    &m_clearResultsButtonText, [this]() { this->clear_state(); }, ButtonOption::Simple());

  std::shared_ptr<SQLiteComponent> m_sqlite_component = Make<SQLiteComponent>();
};

#endif  // MAIN_COMPONENT_HPP

#include <gtest/gtest.h>  // for AssertionResult, Message, TestPartResult, Test, EXPECT_EQ, ASSERT_TRUE, TestInfo, TES

#include "../src/sqlite/SQLiteReports.hpp"  // for UnmetHoursTableRow, SQLiteReports, EndUseTable
#include "../src/LogDisplayer.hpp"          // for LogDisplayer
#include "../src/ErrorMessage.hpp"          // for ErrorMessage
#include "../src/MainComponent.hpp"         // for MainComponent

#include <EnergyPlus/api/TypeDefs.h>  // for Error

#include "ftxui/component/component.hpp"           // for Button, Renderer, Vertical, operator|=
#include <ftxui/component/component_base.hpp>      // for ComponentBase
#include <ftxui/component/component_options.hpp>   // for ButtonOption
#include <ftxui/component/event.hpp>               // for Event
#include <ftxui/component/receiver.hpp>            // for MakeReceiver, Sender
#include <ftxui/screen/screen.hpp>                 // for Screen
#include <ftxui/component/screen_interactive.hpp>  // for ScreenInteractive
#include <ftxui/dom/elements.hpp>                  // for Element, text, operator|, separator, size, vbox, border, Constraint, Direction

#include <array>       // for array
#include <filesystem>  // for is_regular_file, operator/, path
#include <optional>    // for optional
#include <string>      // for string, to_string
#include <vector>      // for vector

namespace fs = std::filesystem;

TEST(SQLiteReports, HighLevelInfo) {
  const fs::path sqlFile = fs::path(TEST_DIR) / "eplusout.sql";
  ASSERT_TRUE(fs::is_regular_file(sqlFile));

  const sql::SQLiteReports report(sqlFile);
  ASSERT_TRUE(report.isValidConnection());

  EXPECT_EQ("22.2.0", report.energyPlusVersion());

  auto eui_ = report.netSiteEnergy();
  ASSERT_TRUE(eui_);
  EXPECT_EQ(225.18, eui_ ? eui_.value() : -999.0);
}

TEST(SQLiteReports, UnmetHoursTable) {
  const fs::path sqlFile = fs::path(TEST_DIR) / "eplusout.sql";
  ASSERT_TRUE(fs::is_regular_file(sqlFile));

  const sql::SQLiteReports report(sqlFile);
  ASSERT_TRUE(report.isValidConnection());

  auto unmetHoursTable = report.unmetHoursTable();
  EXPECT_EQ(7, unmetHoursTable.size());

  const std::array<sql::UnmetHoursTableRow, 7> expectedTable{{
    {"SPACE1-1", {172.0, 52.5, 3.25, 52.5}},
    {"SPACE2-1", {117.5, 39.0, 0.0, 5.25}},
    {"SPACE3-1", {162.0, 0.0, 3.25, 0.0}},
    {"SPACE4-1", {139.75, 20.0, 1.0, 0.0}},
    {"SPACE5-1", {205.5, 0.5, 0.5, 0.5}},
    {"PLENUM-1", {0.0, 0.0, 0.0, 0.0}},
    {"Facility", {209.75, 111.5, 3.25, 57.75}},
  }};

  for (size_t i = 0; i < expectedTable.size(); ++i) {
    const auto& actualRow = unmetHoursTable[i];
    const auto& expectedRow = expectedTable[i];
    EXPECT_EQ(actualRow, expectedRow);
    // Explicit check for clarity if it fails
    EXPECT_EQ(expectedRow.zoneName, actualRow.zoneName);
    EXPECT_EQ(expectedRow.duringHeating, actualRow.duringHeating);
    EXPECT_EQ(expectedRow.duringCooling, actualRow.duringCooling);
    EXPECT_EQ(expectedRow.duringOccHeating, actualRow.duringOccHeating);
    EXPECT_EQ(expectedRow.duringOccCooling, actualRow.duringOccCooling);
  }
}

TEST(SQLiteReports, EndUseTable) {
  const fs::path sqlFile = fs::path(TEST_DIR) / "eplusout.sql";
  ASSERT_TRUE(fs::is_regular_file(sqlFile));

  const sql::SQLiteReports report(sqlFile);
  ASSERT_TRUE(report.isValidConnection());

  auto endUseTable = report.endUseByFuelTable();

  const std::vector<std::string> endUseNames = {"Heating", "Cooling", "Interior Lighting", "Interior Equipment", "Fans", "Pumps", "Total End Uses"};
  EXPECT_EQ(7, endUseTable.endUseNames.size());
  EXPECT_EQ(endUseNames, endUseTable.endUseNames);

  const std::vector<std::string> fuelNames = {"Electricity [GJ]", "Natural Gas [GJ]"};
  EXPECT_EQ(2, endUseTable.fuelNames.size());
  EXPECT_EQ(fuelNames, endUseTable.fuelNames);

  const std::vector<std::vector<double>> vals{
    // {Electricity, Natural Gas}
    {0.0, 68.55},     // Heating
    {16.42, 0.0},     // Cooling
    {81.24, 0.0},     // Interior Lighting
    {47.70, 0.0},     // Interior Equipment
    {8.88, 0.0},      // Fans
    {2.39, 0.0},      // Pumps
    {156.63, 68.55},  // Total End Uses
  };
  EXPECT_EQ(endUseNames.size(), endUseTable.values.size());
  EXPECT_EQ(fuelNames.size(), endUseTable.values[0].size());
  EXPECT_EQ(vals, endUseTable.values);
}

TEST(LogDisplayer, StdoutLines) {

  LogDisplayer log;

  ASSERT_NO_THROW(log.Render());

  constexpr size_t nLines = 20;
  std::vector<std::string> lines;
  lines.resize(nLines);
  for (size_t i = 0; i < nLines; ++i) {
    lines[i] = "Line " + std::to_string(i + 1);
  }

  // Shouldn't crash
  const ftxui::Element element = log.RenderLines(lines);
  ftxui::Screen screen(25, 10);
  Render(screen, element);

  EXPECT_EQ(0, log.selected());
  log.OnEvent(Event::ArrowDown);
  EXPECT_EQ(1, log.selected());
  log.OnEvent(Event::ArrowDown);
  EXPECT_EQ(2, log.selected());
  log.OnEvent(Event::ArrowUp);
  EXPECT_EQ(1, log.selected());

  log.OnEvent(Event::End);
  EXPECT_EQ(19, log.selected());

  log.OnEvent(Event::Home);
  EXPECT_EQ(0, log.selected());

  constexpr int sizeTab = nLines / 10;

  log.OnEvent(Event::Tab);
  EXPECT_EQ(2, log.selected());
  log.OnEvent(Event::Tab);
  EXPECT_EQ(4, log.selected());
  log.OnEvent(Event::TabReverse);
  EXPECT_EQ(2, log.selected());

  const std::string expectedScreen =  //
    "╭Log────────────────────╮\r\n"
    "│Message                │\r\n"
    "├───────────────────────┤\r\n"
    "│\x1B[7mLine 1                \x1B[27m┃│\r\n"  // \x1B are for the focus line
    "│Line 2                ┃│\r\n"
    "│Line 3                 │\r\n"
    "│Line 4                 │\r\n"
    "│Line 5                 │\r\n"
    "│Line 6                 │\r\n"
    "╰───────────────────────╯";

  EXPECT_EQ(expectedScreen, screen.ToString()) << screen.ToString();
}

TEST(LogDisplayer, Eplusout) {

  LogDisplayer log;

  ASSERT_NO_THROW(log.Render());

  constexpr size_t nLines = 20;
  std::vector<ErrorMessage> errors;
  errors.reserve(nLines);
  EnergyPlus::Error error = EnergyPlus::Error::Continue;
  for (size_t i = 0; i < nLines; ++i) {
    if (i % 8 == 0) {
      error = EnergyPlus::Error::Severe;
    } else if (i % 4 == 0) {
      error = EnergyPlus::Error::Warning;
    } else {
      error = EnergyPlus::Error::Continue;
    }
    errors.emplace_back(error, "Line " + std::to_string(i + 1));
  }

  std::vector<ErrorMessage*> filtered_errorMsgs;
  filtered_errorMsgs.reserve(errors.size());
  // No filter
  for (auto& errorMsg : errors) {
    filtered_errorMsgs.push_back(&errorMsg);
  }

  // Shouldn't crash
  const ftxui::Element element = log.RenderLines(filtered_errorMsgs);

  EXPECT_EQ(0, log.selected());
  log.OnEvent(Event::ArrowDown);
  EXPECT_EQ(1, log.selected());
  log.OnEvent(Event::ArrowDown);
  EXPECT_EQ(2, log.selected());
  log.OnEvent(Event::ArrowUp);
  EXPECT_EQ(1, log.selected());

  log.OnEvent(Event::End);
  EXPECT_EQ(19, log.selected());

  log.OnEvent(Event::Home);
  EXPECT_EQ(0, log.selected());

  constexpr int sizeTab = nLines / 10;

  log.OnEvent(Event::Tab);
  EXPECT_EQ(2, log.selected());
  log.OnEvent(Event::Tab);
  EXPECT_EQ(4, log.selected());
  log.OnEvent(Event::TabReverse);
  EXPECT_EQ(2, log.selected());

  {
    ftxui::Screen screen(26, 17);
    Render(screen, element);
    // ╭Log────────────┬────────╮
    // │Type           │Message │
    // ├───────────────┼────────┤
    // │Severe         │Line 1 ┃│
    // │Continue       │Line 2 ┃│
    // │Continue       │Line 3 ┃│
    // │Continue       │Line 4 ┃│
    // ├───────────────┼───────┃│
    // │Warning        │Line 5 ┃│
    // │Continue       │Line 6 ┃│
    // │Continue       │Line 7 ╹│
    // │Continue       │Line 8  │
    // ├───────────────┼─────── │
    // │Severe         │Line 9  │
    // │Continue       │Line 10 │
    // │Continue       │Line 11 │
    // ╰───────────────┴────────╯

    const std::string expectedScreen =  //
      "╭Log────────────┬────────╮\r\n"
      "│Type           │Message │\r\n"
      "├───────────────┼────────┤\r\n"
      "│\x1B[1m\x1B[7m\x1B[91m\x1B[49mSevere         \x1B[39m\x1B[49m│Line 1 \x1B[22m\x1B[27m┃│\r\n"
      "│\x1B[2mContinue       │Line 2 \x1B[22m┃│\r\n"
      "│\x1B[2mContinue       │Line 3 \x1B[22m┃│\r\n"
      "│\x1B[2mContinue       │Line 4 \x1B[22m┃│\r\n"
      "├───────────────┼───────┃│\r\n"
      "│\x1B[33m\x1B[49mWarning        \x1B[39m\x1B[49m│Line 5 ┃│\r\n"
      "│\x1B[2mContinue       │Line 6 \x1B[22m┃│\r\n"
      "│\x1B[2mContinue       │Line 7 \x1B[22m╹│\r\n"
      "│\x1B[2mContinue       │Line 8 \x1B[22m │\r\n"
      "├───────────────┼─────── │\r\n"
      "│\x1B[1m\x1B[91m\x1B[49mSevere         \x1B[39m\x1B[49m│Line 9 \x1B[22m │\r\n"
      "│\x1B[2mContinue       │Line 10\x1B[22m │\r\n"
      "│\x1B[2mContinue       │Line 11\x1B[22m │\r\n"
      "╰───────────────┴────────╯";

    EXPECT_EQ(expectedScreen, screen.ToString()) << screen.ToString();
  }

  {
    // Test flex
    const std::string expectedScreen =  //
      "╭Log────────────┬──────────────────────────────────────────────────────────────╮\r\n"
      "│Type           │Message                                                       │\r\n"
      "╰───────────────┴──────────────────────────────────────────────────────────────╯";

    ftxui::Screen screen(80, 3);
    Render(screen, element);
    EXPECT_EQ(expectedScreen, screen.ToString()) << screen.ToString();
  }
}

TEST(MainComponent, MainComponent) {

  bool quit = false;
  bool run = false;

  std::atomic<int> progress = 0;

  const std::string quit_text = "Quit";
  auto quit_button = ftxui::Button(
    &quit_text, [&quit] { quit = true; }, ftxui::ButtonOption::Ascii());

  const std::string run_text = "Run";
  auto run_button = ftxui::Button(
    &run_text,
    [&run, &progress] {
      run = true;
      progress = 100;
    },
    ftxui::ButtonOption::Simple());

  auto receiverRunOutput = ftxui::MakeReceiver<std::string>();
  auto receiverErrorOutput = ftxui::MakeReceiver<ErrorMessage>();
  auto senderRunOutput = receiverRunOutput->MakeSender();
  auto senderErrorOutput = receiverErrorOutput->MakeSender();

  const fs::path outputDirectory(TEST_DIR);

  MainComponent main_component(std::move(receiverRunOutput), std::move(receiverErrorOutput), std::move(run_button), std::move(quit_button), &progress,
                               outputDirectory);
  ftxui::Screen screen(80, 20);

  Render(screen, main_component.Render());
  std::string screenStr = screen.ToString();
  // EnergyPlus-Cpp-Demo    │Stdout│eplusout.err│SQL Reports│About│1/0│      │← Quit
  // ───────────────────────┴──────┴────────────┴───────────┴─────┴───┴──────┴───────
  //                               ┌──────────────────┐
  //                               │Run               │
  //                               └──────────────────┘
  // ──────┬────────────────────┬───────────────────────────────┬──────────┬─────────
  // Status│      Pending       │                            0 %│0 warnings│0 severes
  // ──────┴────────────────────┴───────────────────────────────┴──────────┴─────────
  // ╭Log───────────────────────────────────────────────────────────────────────────╮
  // │Message                                                                       │
  // ├──────────────────────────────────────────────────────────────────────────────┤
  // │(empty)                                                                       │
  // ╰──────────────────────────────────────────────────────────────────────────────╯

  // EXPECT_EQ("", screenStr) << screenStr;

  EXPECT_TRUE(screenStr.find("Pending") != std::string::npos);
  EXPECT_TRUE(screenStr.find("0 %") != std::string::npos);
  EXPECT_TRUE(screenStr.find("0 warnings") != std::string::npos);
  EXPECT_TRUE(screenStr.find("0 severes") != std::string::npos);
  EXPECT_TRUE(screenStr.find("(empty)") != std::string::npos);

  // Simulate Mid Simulation

  progress = 75;
  senderRunOutput->Send("Stdout Line 1");
  senderRunOutput->Send("Stdout Line 2");
  senderRunOutput->Send("Stdout Line 3");
  senderErrorOutput->Send(ErrorMessage{EnergyPlus::Error::Severe, "A Severe Error"});
  senderErrorOutput->Send(ErrorMessage{EnergyPlus::Error::Continue, "the Severe continues"});
  senderErrorOutput->Send(ErrorMessage{EnergyPlus::Error::Warning, "A Warning"});
  senderErrorOutput->Send(ErrorMessage{EnergyPlus::Error::Continue, "the Warning continues"});
  senderErrorOutput->Send(ErrorMessage{EnergyPlus::Error::Warning, "A 2nd Warning"});
  senderErrorOutput->Send(ErrorMessage{EnergyPlus::Error::Continue, "the 2nd Warning continues"});

  main_component.OnEvent(Event::Custom);

  // This isn't an interactive screen, to recreate it
  screen = ftxui::Screen(80, 20);

  Render(screen, main_component.Render());
  screenStr = screen.ToString();

  // EnergyPlus-Cpp-Demo    │Stdout│eplusout.err│SQL Reports│About│1/3│      │↖ Quit
  // ───────────────────────┴──────┴────────────┴───────────┴─────┴───┴──────┴───────
  //                               ┌──────────────────┐
  //                               │Run               │
  //                               └──────────────────┘
  // ──────┬────────────────────┬───────────────────────────────┬──────────┬─────────
  // Status│      Running       │████████████████████▏      75 %│2 warnings│1 severes
  // ──────┴────────────────────┴───────────────────────────────┴──────────┴─────────
  // ╭Log───────────────────────────────────────────────────────────────────────────╮
  // │Message                                                                       │
  // ├──────────────────────────────────────────────────────────────────────────────┤
  // │Stdout Line 1                                                                 │
  // │Stdout Line 2                                                                 |
  // │Stdout Line 3                                                                 │
  // ╰──────────────────────────────────────────────────────────────────────────────╯

  // EXPECT_EQ("", screen.ToString()) << screen.ToString();

  EXPECT_TRUE(screenStr.find("Running") != std::string::npos);
  EXPECT_TRUE(screenStr.find("75 %") != std::string::npos);
  EXPECT_TRUE(screenStr.find("2 warnings") != std::string::npos);
  EXPECT_TRUE(screenStr.find("1 severes") != std::string::npos);
  EXPECT_TRUE(screenStr.find("Stdout Line 1") != std::string::npos);
  EXPECT_TRUE(screenStr.find("Stdout Line 2") != std::string::npos);
  EXPECT_TRUE(screenStr.find("Stdout Line 3") != std::string::npos);

  // Simulate finished run
  senderRunOutput->Send("EnergyPlus Run Time=00hr 00min 1.16sec");
  senderRunOutput->Send("Stdout Line 2");
  senderRunOutput->Send("Stdout Line 3");

  senderErrorOutput->Send(ErrorMessage{EnergyPlus::Error::Info, "Simulation Error Summary *************"});
  senderErrorOutput->Send(ErrorMessage{EnergyPlus::Error::Info, "EnergyPlus Warmup Error Summary. During Warmup: 0 Warning; 0 Severe Errors."});
  senderErrorOutput->Send(ErrorMessage{EnergyPlus::Error::Info, "EnergyPlus Sizing Error Summary. During Sizing: 0 Warning; 0 Severe Errors."});
  senderErrorOutput->Send(
    ErrorMessage{EnergyPlus::Error::Info, "EnergyPlus Completed Successfully-- 3 Warning; 0 Severe Errors; Elapsed Time=00hr 00min  1.16sec"});

  progress = 100;
  main_component.OnEvent(Event::Custom);

  // This isn't an interactive screen, so recreate it
  screen = ftxui::Screen(80, 20);
  Render(screen, main_component.Render());
  screenStr = screen.ToString();

  EXPECT_TRUE(screenStr.find("Done") != std::string::npos);
  EXPECT_TRUE(screenStr.find("100 %") != std::string::npos);
  EXPECT_TRUE(screenStr.find("2 warnings") != std::string::npos);
  EXPECT_TRUE(screenStr.find("1 severes") != std::string::npos);
  EXPECT_TRUE(screenStr.find("Stdout Line 1") != std::string::npos);
  EXPECT_TRUE(screenStr.find("Stdout Line 2") != std::string::npos);
  EXPECT_TRUE(screenStr.find("Stdout Line 3") != std::string::npos);
  EXPECT_TRUE(screenStr.find("EnergyPlus Run Time=00hr 00min 1.16sec") != std::string::npos);
  EXPECT_TRUE(screenStr.find("Open HTML") != std::string::npos);
  EXPECT_TRUE(screenStr.find("Clear Results") != std::string::npos);

  // Simulate Switching Tab to eplusout.err
  main_component.OnEvent(Event::Home);
  main_component.OnEvent(Event::ArrowUp);
  main_component.OnEvent(Event::ArrowUp);
  main_component.OnEvent(Event::ArrowUp);
  main_component.OnEvent(Event::ArrowUp);
  main_component.OnEvent(Event::ArrowRight);

  // This isn't an interactive screen, so recreate it
  screen = ftxui::Screen(80, 20);
  Render(screen, main_component.Render());
  screenStr = screen.ToString();

  // EnergyPlus-Cpp-Demo │Stdout│eplusout.err│SQL Reports│About│0/10  [10]│  │↗ Quit
  // ────────────────────┴──────┴────────────┴───────────┴─────┴──────────┴──┴───────
  // ╭Type─────╮
  // │▣ Severe │
  // │▣ Warning│
  // │▣ Info   │
  // ╰─────────╯
  // ╭Log────────────┬──────────────────────────────────────────────────────────────╮
  // │Type           │Message                                                       │
  // ├───────────────┼──────────────────────────────────────────────────────────────┤
  // │Severe         │A Severe Error                                               ┃│
  // │Continue       │the Severe continues                                         ┃│
  // ├───────────────┼─────────────────────────────────────────────────────────────┃│
  // │Warning        │A Warning                                                    ┃│
  // │Continue       │the Warning continues                                        ┃│
  // ├───────────────┼─────────────────────────────────────────────────────────────┃│
  // │Warning        │A 2nd Warning                                                ╹│
  // │Continue       │the 2nd Warning continues                                     │
  // ├───────────────┴───────────────────────────────────────────────────────────── │
  // ╰──────────────────────────────────────────────────────────────────────────────╯

  // EXPECT_EQ("", screen.ToString()) << screen.ToString();

  EXPECT_TRUE(screenStr.find("Type") != std::string::npos);
  EXPECT_TRUE(screenStr.find("Severe") != std::string::npos);
  EXPECT_TRUE(screenStr.find("Warning") != std::string::npos);
  EXPECT_TRUE(screenStr.find("Info") != std::string::npos);
  EXPECT_TRUE(screenStr.find("Log") != std::string::npos);

  EXPECT_TRUE(screenStr.find("A Severe Error") != std::string::npos);
  EXPECT_TRUE(screenStr.find("the Severe continues") != std::string::npos);
  EXPECT_TRUE(screenStr.find("A Warning") != std::string::npos);
  EXPECT_TRUE(screenStr.find("the Warning continues") != std::string::npos);
  EXPECT_TRUE(screenStr.find("A 2nd Warning") != std::string::npos);
  EXPECT_TRUE(screenStr.find("the 2nd Warning continues") != std::string::npos);

  EXPECT_FALSE(screenStr.find("EnergyPlus Completed Successfully") != std::string::npos);

  // Simulate filtering out the Warning Level

  main_component.OnEvent(Event::ArrowDown);
  main_component.OnEvent(Event::ArrowDown);
  main_component.OnEvent(Event::Return);
  screen = ftxui::Screen(80, 20);
  Render(screen, main_component.Render());
  screenStr = screen.ToString();

  // EnergyPlus-Cpp-Demo │Stdout│eplusout.err│SQL Reports│About│0/8  [10]│   │→ Quit
  // ────────────────────┴──────┴────────────┴───────────┴─────┴─────────┴───┴───────
  // ╭Type─────╮
  // │▣ Severe │
  // │☐ Warning│
  // │▣ Info   │
  // ╰─────────╯
  // ╭Log────────────┬──────────────────────────────────────────────────────────────╮
  // │Type           │Message                                                       │
  // ├───────────────┼──────────────────────────────────────────────────────────────┤
  // │Severe         │A Severe Error                                                │
  // │Continue       │the Severe continues                                          │
  // ├───────────────┼──────────────────────────────────────────────────────────────┤
  // │Info           │Simulation Error Summary *************                        │
  // │Info           │EnergyPlus Warmup Error Summary. During Warmup: 0 Warning; 0 S│
  // │Info           │EnergyPlus Sizing Error Summary. During Sizing: 0 Warning; 0 S│
  // │Info           │EnergyPlus Completed Successfully-- 3 Warning; 0 Severe Errors│
  // ╰───────────────┴──────────────────────────────────────────────────────────────╯

  // EXPECT_EQ("", screen.ToString()) << screen.ToString();

  EXPECT_TRUE(screenStr.find("Type") != std::string::npos);
  EXPECT_TRUE(screenStr.find("Severe") != std::string::npos);
  EXPECT_TRUE(screenStr.find("Warning") != std::string::npos);
  EXPECT_TRUE(screenStr.find("Info") != std::string::npos);
  EXPECT_TRUE(screenStr.find("Log") != std::string::npos);

  EXPECT_TRUE(screenStr.find("A Severe Error") != std::string::npos);
  EXPECT_TRUE(screenStr.find("the Severe continues") != std::string::npos);
  EXPECT_FALSE(screenStr.find("A Warning") != std::string::npos);
  EXPECT_FALSE(screenStr.find("the Warning continues") != std::string::npos);
  EXPECT_FALSE(screenStr.find("A 2nd Warning") != std::string::npos);
  EXPECT_FALSE(screenStr.find("the 2nd Warning continues") != std::string::npos);
  EXPECT_TRUE(screenStr.find("EnergyPlus Completed Successfully") != std::string::npos);
}

TEST(MainComponent, MainComponent_reload_results) {

  bool quit = false;
  bool run = false;

  std::atomic<int> progress = 0;

  const std::string quit_text = "Quit";
  auto quit_button = ftxui::Button(
    &quit_text, [&quit] { quit = true; }, ftxui::ButtonOption::Ascii());

  const std::string run_text = "Run";
  auto run_button = ftxui::Button(
    &run_text,
    [&run, &progress] {
      run = true;
      progress = 100;
    },
    ftxui::ButtonOption::Simple());

  auto receiverRunOutput = ftxui::MakeReceiver<std::string>();
  auto receiverErrorOutput = ftxui::MakeReceiver<ErrorMessage>();
  auto senderRunOutput = receiverRunOutput->MakeSender();
  auto senderErrorOutput = receiverErrorOutput->MakeSender();

  const fs::path outputDirectory(TEST_DIR);

  MainComponent main_component(std::move(receiverRunOutput), std::move(receiverErrorOutput), std::move(run_button), std::move(quit_button), &progress,
                               outputDirectory);
  main_component.reload_results();

  ftxui::Screen screen(80, 20);

  Render(screen, main_component.Render());
  std::string screenStr = screen.ToString();

  // EnergyPlus-Cpp-Demo    │Stdout│eplusout.err│SQL Reports│About│1/5│      │↘ Quit
  // ───────────────────────┴──────┴────────────┴───────────┴─────┴───┴──────┴───────
  //                  ┌──────────────────┐                 ┌─────────┐┌─────────────┐
  //                  │Run               │                 │Open HTML││Clear Results│
  //                  └──────────────────┘                 └─────────┘└─────────────┘
  // ──────┬────────────────────┬───────────────────────────────┬──────────┬─────────
  // Status│        Done        │██████████████████████████100 %│3 warnings│0 severes
  // ──────┴────────────────────┴───────────────────────────────┴──────────┴─────────
  // ╭Log───────────────────────────────────────────────────────────────────────────╮
  // │Message                                                                       │
  // ├──────────────────────────────────────────────────────────────────────────────┤
  // │=========================================                                     │
  // │   Results have been reloaded from disk                                       │
  // │=========================================                                     │
  // │                                                                              │
  // │   ************* EnergyPlus Completed Successfully-- 3 Warning; 0 Severe Error│
  // ╰──────────────────────────────────────────────────────────────────────────────╯

  // EXPECT_EQ("", screenStr) << screenStr;

  EXPECT_TRUE(screenStr.find("Done") != std::string::npos);
  EXPECT_TRUE(screenStr.find("100 %") != std::string::npos);
  EXPECT_TRUE(screenStr.find("3 warnings") != std::string::npos);
  EXPECT_TRUE(screenStr.find("0 severes") != std::string::npos);
  EXPECT_TRUE(screenStr.find("Results have been reloaded from disk") != std::string::npos);
  EXPECT_TRUE(screenStr.find("EnergyPlus Completed Successfully-- 3 Warning; 0 Severe Error") != std::string::npos);
  EXPECT_TRUE(screenStr.find("Open HTML") != std::string::npos);
  EXPECT_TRUE(screenStr.find("Clear Results") != std::string::npos);

  // Simulate Switching Tab to eplusout.err
  main_component.OnEvent(Event::Home);
  main_component.OnEvent(Event::ArrowUp);
  main_component.OnEvent(Event::ArrowUp);
  main_component.OnEvent(Event::ArrowUp);
  main_component.OnEvent(Event::ArrowUp);
  main_component.OnEvent(Event::ArrowRight);

  // This isn't an interactive screen, so recreate it
  screen = ftxui::Screen(80, 20);
  Render(screen, main_component.Render());
  screenStr = screen.ToString();

  // EnergyPlus-Cpp-Demo │Stdout│eplusout.err│SQL Reports│About│0/21  [21]│  │↓ Quit
  // ────────────────────┴──────┴────────────┴───────────┴─────┴──────────┴──┴───────
  // ╭Type─────╮
  // │▣ Info   │
  // │▣ Warning│
  // ╰─────────╯
  // ╭Log────────────┬──────────────────────────────────────────────────────────────╮
  // │Type           │Message                                                       │
  // ├───────────────┼──────────────────────────────────────────────────────────────┤
  // │Info           │Program Version,EnergyPlus, Version 22.2.0-23dcfc8220, YMD=20┃│
  // ├───────────────┼─────────────────────────────────────────────────────────────┃│
  // │Warning        │Weather file location will be used rather than entered (IDF) ┃│
  // │Continue       │..Location object=DENVER CENTENNIAL  GOLDEN   N_CO_USA DESIGN┃│
  // │Continue       │..Weather File Location=Chicago Ohare Intl Ap IL USA TMY3 WMO╹│
  // │Continue       │..due to location differences, Latitude difference=[2.24] deg │
  // │Continue       │..Time Zone difference=[1.0] hour(s), Elevation difference=[8 │
  // ├───────────────┼───────────────────────────────────────────────────────────── │
  // │Warning        │SetUpDesignDay: Entered DesignDay Barometric Pressure=81198 d │
  // │Continue       │...occurs in DesignDay=DENVER CENTENNIAL  GOLDEN   N ANN HTG  │
  // ╰───────────────┴──────────────────────────────────────────────────────────────╯

  // EXPECT_EQ("", screenStr) << screenStr;

  EXPECT_TRUE(screenStr.find("Type") != std::string::npos);
  EXPECT_FALSE(screenStr.find("Severe") != std::string::npos);
  EXPECT_TRUE(screenStr.find("Warning") != std::string::npos);
  EXPECT_TRUE(screenStr.find("Info") != std::string::npos);
  EXPECT_TRUE(screenStr.find("Log") != std::string::npos);

  EXPECT_TRUE(screenStr.find("Weather file location will be used rather than entered") != std::string::npos);
  EXPECT_TRUE(screenStr.find("..Location object=DENVER CENTENNIAL") != std::string::npos);
  EXPECT_TRUE(screenStr.find("Program Version,EnergyPlus") != std::string::npos);
  EXPECT_FALSE(screenStr.find("Simulation Error Summary") != std::string::npos);

  main_component.OnEvent(Event::ArrowDown);
  main_component.OnEvent(Event::ArrowDown);
  main_component.OnEvent(Event::Return);
  screen = ftxui::Screen(80, 20);
  Render(screen, main_component.Render());
  screenStr = screen.ToString();

  // EnergyPlus-Cpp-Demo │Stdout│eplusout.err│SQL Reports│About│0/12  [21]│  │↙ Quit
  // ────────────────────┴──────┴────────────┴───────────┴─────┴──────────┴──┴───────
  // ╭Type─────╮
  // │▣ Info   │
  // │☐ Warning│
  // ╰─────────╯
  // ╭Log────────────┬──────────────────────────────────────────────────────────────╮
  // │Type           │Message                                                       │
  // ├───────────────┼──────────────────────────────────────────────────────────────┤
  // │Info           │Program Version,EnergyPlus, Version 22.2.0-23dcfc8220, YMD=20┃│
  // │Info           │************* Testing Individual Branch Integrity            ┃│
  // │Info           │************* All Branches passed integrity testing          ┃│
  // │Info           │************* Testing Individual Supply Air Path Integrity   ┃│
  // │Info           │************* All Supply Air Paths passed integrity testing  ┃│
  // │Info           │************* Testing Individual Return Air Path Integrity   ┃│
  // │Info           │************* All Return Air Paths passed integrity testing  ┃│
  // │Info           │************* No node connection errors were found.          ┃│
  // │Info           │************* Beginning Simulation                           ╹│
  // │Info           │************* Simulation Error Summary *************          │
  // ╰───────────────┴──────────────────────────────────────────────────────────────╯

  // EXPECT_EQ("", screenStr) << screenStr;

  EXPECT_TRUE(screenStr.find("Type") != std::string::npos);
  EXPECT_FALSE(screenStr.find("Severe") != std::string::npos);
  EXPECT_TRUE(screenStr.find("Warning") != std::string::npos);
  EXPECT_TRUE(screenStr.find("Info") != std::string::npos);
  EXPECT_TRUE(screenStr.find("Log") != std::string::npos);

  EXPECT_FALSE(screenStr.find("Weather file location will be used rather than entered") != std::string::npos);
  EXPECT_FALSE(screenStr.find("..Location object=DENVER CENTENNIAL") != std::string::npos);
  EXPECT_TRUE(screenStr.find("Program Version,EnergyPlus") != std::string::npos);
  EXPECT_TRUE(screenStr.find("Simulation Error Summary") != std::string::npos);
}

TEST(MainComponent, MainComponent_SqlTab) {
  bool quit = false;
  bool run = false;

  std::atomic<int> progress = 0;

  const std::string quit_text = "Quit";
  auto quit_button = ftxui::Button(
    &quit_text, [&quit] { quit = true; }, ftxui::ButtonOption::Ascii());

  const std::string run_text = "Run";
  auto run_button = ftxui::Button(
    &run_text,
    [&run, &progress] {
      run = true;
      progress = 100;
    },
    ftxui::ButtonOption::Simple());

  auto receiverRunOutput = ftxui::MakeReceiver<std::string>();
  auto receiverErrorOutput = ftxui::MakeReceiver<ErrorMessage>();
  auto senderRunOutput = receiverRunOutput->MakeSender();
  auto senderErrorOutput = receiverErrorOutput->MakeSender();

  const fs::path outputDirectory(TEST_DIR);

  MainComponent main_component(std::move(receiverRunOutput), std::move(receiverErrorOutput), std::move(run_button), std::move(quit_button), &progress,
                               outputDirectory);
  main_component.reload_results();

  // Simulate Switching Tab to eplusout.err
  main_component.OnEvent(Event::Home);
  main_component.OnEvent(Event::ArrowUp);
  main_component.OnEvent(Event::ArrowUp);
  main_component.OnEvent(Event::ArrowUp);
  main_component.OnEvent(Event::ArrowUp);
  main_component.OnEvent(Event::ArrowRight);
  main_component.OnEvent(Event::ArrowRight);

  // This isn't an interactive screen, so recreate it
  ftxui::Screen screen(90, 50);
  Render(screen, main_component.Render());
  std::string screenStr = screen.ToString();

  // EnergyPlus-Cpp-Demo         │Stdout│eplusout.err│SQL Reports│About│1/5│           │← Quit
  // ────────────────────────────┴──────┴────────────┴───────────┴─────┴───┴───────────┴───────
  //
  // ╭High Level Info───────────────┬──────────────────────────────┬─────────────────────────╮
  // │             Item             │            Value             │     Units               │
  // ├──────────────────────────────┼──────────────────────────────┼─────────────────────────┤
  // │      EnergyPlus Version      │            22.2.0            │                         │
  // ├──────────────────────────────┼──────────────────────────────┼─────────────────────────┤
  // │       Net Site Energy        │            225.18            │      GJ                 │
  // │                              │                              │                         │
  // ╰──────────────────────────────┴──────────────────────────────┴─────────────────────────╯
  // ╭Unmet Hours─────────────┬──────────────┬───────────────────────┬───────────────────────╮
  // │Zone Name│During Heating│During Cooling│During Occupied Heating│During Occupied Cooling│
  // ├─────────┼──────────────┼──────────────┼───────────────────────┼───────────────────────┤
  // │SPACE1-1 │    172.00    │    52.50     │         3.25          │         52.50         │
  // ├─────────┼──────────────┼──────────────┼───────────────────────┼───────────────────────┤
  // │SPACE2-1 │    117.50    │    39.00     │         0.00          │         5.25          │
  // ├─────────┼──────────────┼──────────────┼───────────────────────┼───────────────────────┤
  // │SPACE3-1 │    162.00    │     0.00     │         3.25          │         0.00          │
  // ├─────────┼──────────────┼──────────────┼───────────────────────┼───────────────────────┤
  // │SPACE4-1 │    139.75    │    20.00     │         1.00          │         0.00          │
  // ├─────────┼──────────────┼──────────────┼───────────────────────┼───────────────────────┤
  // │SPACE5-1 │    205.50    │     0.50     │         0.50          │         0.50          │
  // ├─────────┼──────────────┼──────────────┼───────────────────────┼───────────────────────┤
  // │PLENUM-1 │     0.00     │     0.00     │         0.00          │         0.00          │
  // ├─────────┴──────────────┴──────────────┴───────────────────────┴───────────────────────┤
  // ├─────────┬──────────────┬──────────────┬───────────────────────┬───────────────────────┤
  // │Facility │    209.75    │    111.50    │         3.25          │         57.75         │
  // │         │              │              │                       │                       │
  // ╰─────────┴──────────────┴──────────────┴───────────────────────┴───────────────────────╯
  // ╭End Use by Fuel─────┬────────────────┬─────────────────────────────────────────────────╮
  // │End Use             │Electricity [GJ]│Natural Gas [GJ]                                 │
  // ├────────────────────┼────────────────┼─────────────────────────────────────────────────┤
  // │Heating             │      0.00      │     68.55                                       │
  // ├────────────────────┼────────────────┼─────────────────────────────────────────────────┤
  // │Cooling             │     16.42      │      0.00                                       │
  // ├────────────────────┼────────────────┼─────────────────────────────────────────────────┤
  // │Interior Lighting   │     81.24      │      0.00                                       │
  // ├────────────────────┼────────────────┼─────────────────────────────────────────────────┤
  // │Interior Equipment  │     47.70      │      0.00                                       │
  // ├────────────────────┼────────────────┼─────────────────────────────────────────────────┤
  // │Fans                │      8.88      │      0.00                                       │
  // ├────────────────────┼────────────────┼─────────────────────────────────────────────────┤
  // │Pumps               │      2.39      │      0.00                                       │
  // ├────────────────────┼────────────────┼─────────────────────────────────────────────────┤
  // │Total End Uses      │     156.63     │     68.55                                       │
  // ├────────────────────┴────────────────┴─────────────────────────────────────────────────┤
  // ╰───────────────────────────────────────────────────────────────────────────────────────╯

  // EXPECT_EQ("", screenStr) << screenStr;

  EXPECT_TRUE(screenStr.find("EnergyPlus Version") != std::string::npos);
  EXPECT_TRUE(screenStr.find("22.2.0") != std::string::npos);

  EXPECT_TRUE(screenStr.find("Net Site Energy") != std::string::npos);
  EXPECT_TRUE(screenStr.find("225.18") != std::string::npos);

  EXPECT_TRUE(screenStr.find("205.50") != std::string::npos);
  EXPECT_TRUE(screenStr.find("57.75") != std::string::npos);
  EXPECT_TRUE(screenStr.find("Electricity [GJ]") != std::string::npos);
  EXPECT_TRUE(screenStr.find("81.24") != std::string::npos);
  EXPECT_TRUE(screenStr.find("Total End Uses") != std::string::npos);
}

TEST(MainComponent, MainComponent_About) {
  bool quit = false;
  bool run = false;

  std::atomic<int> progress = 0;

  const std::string quit_text = "Quit";
  auto quit_button = ftxui::Button(
    &quit_text, [&quit] { quit = true; }, ftxui::ButtonOption::Ascii());

  const std::string run_text = "Run";
  auto run_button = ftxui::Button(
    &run_text,
    [&run, &progress] {
      run = true;
      progress = 100;
    },
    ftxui::ButtonOption::Simple());

  auto receiverRunOutput = ftxui::MakeReceiver<std::string>();
  auto receiverErrorOutput = ftxui::MakeReceiver<ErrorMessage>();
  auto senderRunOutput = receiverRunOutput->MakeSender();
  auto senderErrorOutput = receiverErrorOutput->MakeSender();

  const fs::path outputDirectory(TEST_DIR);

  MainComponent main_component(std::move(receiverRunOutput), std::move(receiverErrorOutput), std::move(run_button), std::move(quit_button), &progress,
                               outputDirectory);
  main_component.reload_results();

  // Simulate Switching Tab to eplusout.err
  main_component.OnEvent(Event::Home);
  main_component.OnEvent(Event::ArrowUp);
  main_component.OnEvent(Event::ArrowUp);
  main_component.OnEvent(Event::ArrowUp);
  main_component.OnEvent(Event::ArrowUp);
  main_component.OnEvent(Event::ArrowRight);
  main_component.OnEvent(Event::ArrowRight);
  main_component.OnEvent(Event::ArrowRight);

  // This isn't an interactive screen, so recreate it
  ftxui::Screen screen(80, 10);
  Render(screen, main_component.Render());
  std::string screenStr = screen.ToString();
  // EXPECT_EQ("", screenStr) << screenStr;

  EXPECT_TRUE(screenStr.find("Skipping Lines faster") != std::string::npos);
  EXPECT_FALSE(screenStr.find("[Quit]") != std::string::npos);

  main_component.OnEvent(Event::ArrowRight);
  // This isn't an interactive screen, so recreate it
  screen = ftxui::Screen(80, 1);
  Render(screen, main_component.Render());
  screenStr = screen.ToString();
  // EXPECT_EQ("", screenStr) << screenStr;

  EXPECT_TRUE(screenStr.find("[Quit]") != std::string::npos);
  EXPECT_FALSE(quit);
  main_component.OnEvent(Event::Return);
  EXPECT_TRUE(quit);
}

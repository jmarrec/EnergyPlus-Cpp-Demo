#include <gtest/gtest.h>  // for AssertionResult, Message, TestPartResult, Test, EXPECT_EQ, ASSERT_TRUE, TestInfo, TES

#include "../src/sqlite/SQLiteReports.hpp"  // for UnmetHoursTableRow, SQLiteReports, EndUseTable
#include "../src/LogDisplayer.hpp"          // for LogDisplayer

#include <ftxui/component/event.hpp>  // for Event

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

TEST(LogDisplayer, Basic) {

  LogDisplayer log;

  ASSERT_NO_THROW(log.Render());

  constexpr size_t nLines = 20;
  std::vector<std::string> lines;
  lines.resize(nLines);
  for (size_t i = 0; i < nLines; ++i) {
    lines[i] = "Line " + std::to_string(i + 1);
  }

  // Shouldn't crash
  log.RenderLines(lines);

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
}

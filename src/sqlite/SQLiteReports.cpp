#include "PreparedStatement.hpp"
#include "SQLiteReports.hpp"

#include <sqlite3.h>

#include <ctre.hpp>
#include <fmt/format.h>

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>

using namespace ftxui;

namespace sql {

SQLiteReports::SQLiteReports(std::filesystem::path databasePath) : m_databasePath(std::filesystem::temp_directory_path() / "eplusout.sql") {

  std::filesystem::copy_file(databasePath, m_databasePath, std::filesystem::copy_options::overwrite_existing);

  std::string fileName = m_databasePath.make_preferred().string();

  int code = sqlite3_open_v2(fileName.c_str(), &m_db, SQLITE_OPEN_READONLY, nullptr);
  m_connectionOpen = (code == 0);

  if (!m_connectionOpen) {
    throw std::runtime_error(fmt::format("epcli could not open the sqlfile at '{}'\n", fileName));
  }
  // create index on dictionaryIndex for large table reportvariabledata
  if (!isValidConnection()) {
    sqlite3_close(m_db);
    m_connectionOpen = false;
    throw std::runtime_error("epcli is not compatible with this file.");
  }
  // set a 1 second timeout
  // sqlite3_busy_timeout(m_db, 1000);
}

bool SQLiteReports::isValidConnection() const {
  std::string energyPlusVersion = this->energyPlusVersion();
  return !energyPlusVersion.empty();
}

SQLiteReports::~SQLiteReports() {
  close();
}

bool SQLiteReports::close() {
  if (m_connectionOpen) {
    sqlite3_close(m_db);
    m_connectionOpen = false;
  }
  return true;
}

std::string SQLiteReports::energyPlusVersion() const {
  std::string result;
  if (m_db) {
    PreparedStatement stmt("SELECT EnergyPlusVersion FROM Simulations", m_db, false);
    if (auto s_ = stmt.execAndReturnFirstString()) {
      // in 8.1 this is 'EnergyPlus-Windows-32 8.1.0.008, YMD=2014.11.08 22:49'
      // in 8.2 this is 'EnergyPlus, Version 8.2.0-8397c2e30b, YMD=2015.01.09 08:37'
      // radiance script is writing 'EnergyPlus, VERSION 8.2, (OpenStudio) YMD=2015.1.9 08:35:36'
      if (auto [whole, version] = ctre::search<R"((?<version>\d{1,}\.\d[\.\d]*))">(s_.value()); whole) {
        result = version;
      }
    }
  }
  return result;
}

std::optional<double> SQLiteReports::netSiteEnergy() const {
  std::string s = R"sql(SELECT Value FROM TabularDataWithStrings
                           WHERE ReportName='AnnualBuildingUtilityPerformanceSummary'
                           AND ReportForString='Entire Facility'
                           AND TableName='Site and Source Energy'
                           AND RowName='Net Site Energy'
                           AND ColumnName='Total Energy'
                           AND Units='GJ';)sql";

  PreparedStatement stmt(s, m_db, false);
  return stmt.execAndReturnFirstDouble();
}

std::vector<UnmetHoursTableRow> SQLiteReports::unmetHoursTable() const {

  std::vector<UnmetHoursTableRow> result;

  auto zoneNames_ =  //
    PreparedStatement{R"sql(SELECT DISTINCT(RowName) FROM TabularDataWithStrings
    WHERE ReportName='SystemSummary'
    AND ReportForString='Entire Facility'
    AND TableName='Time Setpoint Not Met';)sql",
                      m_db, false}
      .execAndReturnVectorOfString();

  if (!zoneNames_.has_value()) {
    return result;
  }

  PreparedStatement stmt(R"sql(
SELECT Value FROM TabularDataWithStrings
    WHERE ReportName='SystemSummary'
    AND ReportForString='Entire Facility'
    AND TableName='Time Setpoint Not Met'
    AND RowName=?
    AND ColumnName=?;
  )sql",
                         m_db, false);

  for (const auto& zoneName : zoneNames_.value()) {
    std::array<double, 4> vals{};
    for (int i = 0; auto colName : UnmetHoursTableRow::headers) {
      stmt.bind(1, zoneName);
      stmt.bind(2, std::string{colName});
      if (auto val_ = stmt.execAndReturnFirstDouble()) {
        vals[i] = val_.value();
      }
    }
    result.emplace_back(zoneName, vals);
  }

  return result;
}

}  // namespace sql

ftxui::Element RenderHighLevelInfo(const sql::SQLiteReports& report) {

  struct TableEntry
  {
    std::string item;
    std::string value;
    std::string units;
  };

  std::array<TableEntry, 2> entries{{
    {"EnergyPlus Version", report.energyPlusVersion(), ""},
    {"Net Site Energy", fmt::format("{:.2f}", report.netSiteEnergy().value()), "GJ"},
  }};

  Elements elementList;
  size_t size_item = 30;
  size_t size_value = 30;
  size_t size_units = 15;

  for (const auto& entry : entries) {
    size_item = std::max(size_item, entry.item.size());
    size_value = std::max(size_value, entry.value.size());
    size_units = std::max(size_units, entry.units.size());
  }

  auto header = hbox({
    hcenter(text("Item")) | ftxui::size(WIDTH, EQUAL, size_item),  //
    separator(),
    hcenter(text("Value")) | flex | ftxui::size(WIDTH, GREATER_THAN, size_value),
    separator(),
    hcenter(text("Units")) | ftxui::size(WIDTH, EQUAL, size_units),
  });

  for (const auto& entry : entries) {

    Element document =  //
      hbox({
        hcenter(text(entry.item)) | ftxui::size(WIDTH, EQUAL, size_item),
        separator(),
        hcenter(text(entry.value)) | flex | ftxui::size(WIDTH, GREATER_THAN, size_value),
        separator(),
        hcenter(text(entry.units)) | ftxui::size(WIDTH, EQUAL, size_units),
      })
      | flex;
    elementList.push_back(document);
    elementList.push_back(separator());
  }

  return window(text(L"High Level Info"), vbox({
                                            header,  //
                                            separator(),
                                            vbox(elementList) | vscroll_indicator | yframe,  //  | reflect(box_),
                                          }));
}

ftxui::Element RenderUnmetHours(const sql::SQLiteReports& report) {
  auto tableData = report.unmetHoursTable();

  std::array<size_t, sql::UnmetHoursTableRow::headers.size() + 1> col_sizes{};

  col_sizes[0] = 20;
  for (auto& tableRow : tableData) {
    col_sizes[0] = std::max(col_sizes[0], tableRow.zoneName.size());
  }

  Elements headerList;
  headerList.emplace_back(text("Zone Name") | flex);
  headerList.emplace_back(separator());
  for (size_t i = 1; auto colName : sql::UnmetHoursTableRow::headers) {
    col_sizes[i] = colName.size();
    headerList.emplace_back(text(std::string{colName})                 //
                            | ftxui::size(WIDTH, EQUAL, col_sizes[i])  //
    );
    if (i < sql::UnmetHoursTableRow::headers.size()) {
      headerList.emplace_back(separator());
    }
    ++i;
  }

  auto header = hbox(headerList);

  auto format_double = [](double d) { return fmt::format("{:.2f}", d); };

  Elements rowList;
  for (size_t i = 0; auto& tableRow : tableData) {
    Element document =  //
      hbox({
        hcenter(text(tableRow.zoneName)) | flex,  //| ftxui::size(WIDTH, EQUAL, size_item),
        separator(),
        hcenter(text(format_double(tableRow.duringHeating))) | ftxui::size(WIDTH, EQUAL, col_sizes[1]),
        separator(),
        hcenter(text(format_double(tableRow.duringCooling))) | ftxui::size(WIDTH, EQUAL, col_sizes[2]),
        separator(),
        hcenter(text(format_double(tableRow.duringOccHeating))) | ftxui::size(WIDTH, EQUAL, col_sizes[3]),
        separator(),
        hcenter(text(format_double(tableRow.duringOccCooling))) | ftxui::size(WIDTH, EQUAL, col_sizes[4]),
      })
      | flex;
    rowList.push_back(document);
    rowList.push_back(separator());
    if (i == tableData.size() - 2) {
      rowList.push_back(separator());
    }
    ++i;
  }

  return window(text(L"Unmet Hours"), vbox({
                                        header,  //
                                        separator(),
                                        vbox(rowList) | vscroll_indicator | yframe,  //  | reflect(box_),
                                      }));
}

ftxui::Element SQLiteComponent::RenderDatabase(std::filesystem::path databasePath) {
  sql::SQLiteReports report(std::move(databasePath));

  return vbox({RenderHighLevelInfo(report), RenderUnmetHours(report)});
}

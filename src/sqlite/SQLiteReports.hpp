#ifndef SQL_SQLITEREPORTS_HPP
#define SQL_SQLITEREPORTS_HPP

#include <ftxui/component/component_base.hpp>  // for ComponentBase
#include <ftxui/dom/elements.hpp>              // for Element

#include <array>        // for array
#include <filesystem>   // for path
#include <optional>     // for optional
#include <string>       // for string
#include <string_view>  // for basic_string_view, str...
#include <utility>      // for move
#include <vector>       // for vector

struct sqlite3;

namespace sql {

struct UnmetHoursTableRow
{
  static constexpr std::array<std::string_view, 4> headers = {"During Heating", "During Cooling", "During Occupied Heating",
                                                              "During Occupied Cooling"};
  UnmetHoursTableRow(std::string t_zoneName, std::array<double, 4>& vals)
    : zoneName(std::move(t_zoneName)), duringHeating(vals[0]), duringCooling(vals[1]), duringOccHeating(vals[2]), duringOccCooling(vals[3]) {}
  std::string zoneName;
  double duringHeating;
  double duringCooling;
  double duringOccHeating;
  double duringOccCooling;
};

// template <size_t ROW_SIZE, size_t COL_SIZE>
// struct EndUseTable
// {
//   std::array<std::string, ROW_SIZE> endUseNames;  // = rowNames
//   std::array<std::string, COL_SIZE> fuelNames;    // = columnNames
//   std::array<std::array<double, ROW_SIZE>, COL_SIZE> values;
// }

struct EndUseTable
{
  std::vector<std::string> endUseNames;  // = rowNames
  std::vector<std::string> fuelNames;    // = columnNames
  std::vector<std::vector<double>> values;
};

class SQLiteReports
{
 public:
  explicit SQLiteReports(std::filesystem::path databasePath);

  ~SQLiteReports();

  bool isValidConnection() const;

  std::string energyPlusVersion() const;
  std::optional<double> netSiteEnergy() const;
  std::vector<UnmetHoursTableRow> unmetHoursTable() const;

  // template <size_t ROW_SIZE, size_t COL_SIZE>
  EndUseTable endUseByFuelTable() const;

 private:
  bool close();

  sqlite3* m_db;
  std::filesystem::path m_databasePath;
  bool m_connectionOpen = false;
};

}  // namespace sql

class SQLiteComponent : public ftxui::ComponentBase
{
 public:
  SQLiteComponent() = default;
  ftxui::Element RenderDatabase(std::filesystem::path databasePath);
  virtual bool Focusable() const override {
    return true;
  };
};

#endif  // SQL_PREPAREDSTATEMENT_HPP

#ifndef SQL_SQLITEREPORTS_HPP
#define SQL_SQLITEREPORTS_HPP

#include <ftxui/component/component.hpp>

#include <array>
#include <filesystem>
#include <optional>
#include <string_view>

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

class SQLiteReports
{
 public:
  explicit SQLiteReports(std::filesystem::path databasePath);

  ~SQLiteReports();

  bool isValidConnection() const;

  std::string energyPlusVersion() const;
  std::optional<double> netSiteEnergy() const;
  std::vector<UnmetHoursTableRow> unmetHoursTable() const;

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

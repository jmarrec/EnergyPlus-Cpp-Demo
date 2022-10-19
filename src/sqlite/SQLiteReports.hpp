#ifndef SQL_SQLITEREPORTS_HPP
#define SQL_SQLITEREPORTS_HPP

#include <ftxui/component/component.hpp>

#include <filesystem>
#include <optional>

struct sqlite3;

namespace sql {

class SQLiteReports
{
 public:
  explicit SQLiteReports(std::filesystem::path databasePath);

  ~SQLiteReports();

  bool isValidConnection() const;

  std::string energyPlusVersion() const;
  std::optional<double> netSiteEnergy() const;

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

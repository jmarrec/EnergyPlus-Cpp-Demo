#ifndef SQL_PREPAREDSTATEMENT_HPP
#define SQL_PREPAREDSTATEMENT_HPP

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <fmt/format.h>

struct sqlite3;
struct sqlite3_stmt;

namespace sql {
class PreparedStatement
{
 private:
  sqlite3* m_db;
  sqlite3_stmt* m_statement;
  bool m_transaction;

  struct InternalConstructor
  {
  };

  // a helper constructor to delegate to. It has the first parameter as
  // InternalConstructor so it's distinguishable from the on with the defaulted
  // t_transaction
  PreparedStatement(InternalConstructor, const std::string& t_stmt, sqlite3* t_db, bool t_transaction);

 public:
  PreparedStatement& operator=(const PreparedStatement&) = delete;
  PreparedStatement(const PreparedStatement&) = delete;

  PreparedStatement(const std::string& t_stmt, sqlite3* t_db, bool t_transaction = false)
    : PreparedStatement(InternalConstructor{}, t_stmt, t_db, t_transaction) {}

  template <typename... Args>
  PreparedStatement(const std::string& t_stmt, sqlite3* t_db, bool t_transaction, Args&&... args)
    : PreparedStatement(InternalConstructor{}, t_stmt, t_db, t_transaction) {
    if (!bindAll(args...)) {
      throw std::runtime_error("Error bindings args with statement: " + t_stmt);
    }
  }

  ~PreparedStatement();

  bool bind(int position, const std::string& t_str);

  bool bind(int position, int val);

  bool bind(int position, unsigned int val);

  bool bind(int position, double val);

  // Makes no sense
  bool bind(int position, char val) = delete;

  /** char* to std::string. */
  bool bind(int position, const char* s) {
    return bind(position, std::string(s));
  }

  /** wstring to std::string. */
  bool bind(int position, const std::wstring& w) = delete;  // Don't want to bother

  /** wchar_t* to std::string. */
  bool bind(int position, const wchar_t* w) = delete;

  // This one would happen automatically, but I would rather be explicit
  bool bind(int position, bool val) {
    return bind(position, static_cast<int>(val));
  }

  [[nodiscard]] static int get_sqlite3_bind_parameter_count(sqlite3_stmt* statement);

  template <typename... Args>
  bool bindAll(Args&&... args) {
    const std::size_t size = sizeof...(Args);
    int nPlaceholders = get_sqlite3_bind_parameter_count(m_statement);
    if (nPlaceholders != size) {
      throw std::runtime_error(fmt::format("Wrong number of placeholders [{}] versus bindArgs [{}].", nPlaceholders, size));
    } else {
      int i = 1;
      // Call bind(i++, args[i]) until any returns false
      bool ok = (bind(i++, args) && ...);
      return ok;
    }
  }

  // Executes a **SINGLE** statement
  void execAndThrowOnError() {
    int code = execute();
    // copied in here to avoid including sqlite3 header everywhere
    constexpr auto SQLITE_DONE = 101;
    if (code != SQLITE_DONE) {
      throw std::runtime_error("Error executing SQL statement step");
    }
  }

  // Executes a **SINGLE** statement
  int execute();

  [[nodiscard]] std::optional<double> execAndReturnFirstDouble() const;

  [[nodiscard]] std::optional<int> execAndReturnFirstInt() const;

  static std::string columnText(const unsigned char* column) {
    return {reinterpret_cast<const char*>(column)};  // NOLINT
  }

  [[nodiscard]] std::optional<std::string> execAndReturnFirstString() const;

  /// execute a statement and return the results (if any) in a vector of double
  [[nodiscard]] std::optional<std::vector<double>> execAndReturnVectorOfDouble() const;

  /// execute a statement and return the results (if any) in a vector of int
  [[nodiscard]] std::optional<std::vector<int>> execAndReturnVectorOfInt() const;

  /// execute a statement and return the results (if any) in a vector of string
  [[nodiscard]] std::optional<std::vector<std::string>> execAndReturnVectorOfString() const;
};
}  // namespace sql
#endif  // UTILITIES_SQL_PREPAREDSTATEMENT_HPP

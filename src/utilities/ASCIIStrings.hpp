#ifndef UTILITIES_ASCIISTRINGS_HPP
#define UTILITIES_ASCIISTRINGS_HPP

#include <string>
#include <string_view>

namespace utilities {

inline std::string ascii_to_lower_copy(std::string_view input) {
  std::string result{input};
  constexpr auto to_lower_diff = 'a' - 'A';
  for (auto& c : result) {
    if (c >= 'A' && c <= 'Z') {
      c += to_lower_diff;
    }
  }
  return result;
}

inline std::string_view ascii_trim_left(std::string_view s) {
  return s.substr(std::min(s.find_first_not_of(" \f\n\r\t\v"), s.size()));
}

inline std::string_view ascii_trim_right(std::string_view s) {
  return s.substr(0, std::min(s.find_last_not_of(" \f\n\r\t\v") + 1, s.size()));
}

inline std::string_view ascii_trim(std::string_view s) {
  return ascii_trim_left(ascii_trim_right(s));
}

inline void ascii_trim(std::string& str) {
  str = std::string(ascii_trim(std::string_view(str)));
}

}  // namespace utilities

#endif  // UTILITIES_ASCIISTRINGS_HPP

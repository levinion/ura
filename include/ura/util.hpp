#pragma once
#include <cctype>
#include <string>
#include <vector>

namespace ura {

inline std::vector<std::string> split(std::string& s, char symbol) {
  std::vector<std::string> v;
  std::string t;
  for (int i = 0; i < s.size(); i++) {
    auto c = tolower(s[i]);
    if (c == symbol) {
      v.push_back(t);
      t.clear();
    } else {
      t.push_back(c);
    }
  }
  if (!t.empty())
    v.push_back(t);
  return v;
}
} // namespace ura

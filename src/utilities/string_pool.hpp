#pragma once
#include <string>
#include <string_view>
#include <unordered_set>

namespace util {

class StringPool {
 public:
  std::string_view Intern(std::string_view s) {
    auto [it, inserted] = pool_.emplace(s);
    return *it;
  }

 private:
  std::unordered_set<std::string, std::hash<std::string_view>, std::equal_to<>>
      pool_;
};

inline StringPool& GlobalStringPool() {
  static StringPool pool;
  return pool;
}

}  // namespace util

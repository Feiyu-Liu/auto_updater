#include <cstdlib>
#include <iostream>
#include <tuple>
#include <vector>
#include "fixtures/winsparkle_0_8_1_comparator.h"

int main() {
  int failures = 0;
  for (const auto& test : std::vector<std::tuple<std::string, std::string, int>>{
           {"1.0.7+18", "1.0.7+18", 0},
           {"1.0.6+17", "1.0.6+18", -1},
           {"1.0.6+17", "1.0.7+18", -1},
           {"1.0.9+1", "1.0.10+1", -1},
           {"1.0.7+19", "1.0.7+18", 1},
           {"1.0.8+1", "1.0.7+18", 1},
           {"1.0.7+9", "1.0.7+10", -1},
           {"1.0.6+17", "1.0.6.17", -1},
           {"1.0.6+99", "1.0.6.17", -1}}) {
    const auto& a = std::get<0>(test);
    const auto& b = std::get<1>(test);
    const int expected = std::get<2>(test);
    const int actual = winsparkle::UpdateChecker::CompareVersions(a, b);
    if ((actual > 0) - (actual < 0) != expected ||
        winsparkle::UpdateChecker::CompareVersions(b, a) != -actual) {
      std::cerr << a << " vs " << b << " comparison failed\n";
      ++failures;
    }
  }
  std::cout << "WinSparkle 0.8.1 comparator failures=" << failures << "\n";
  return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

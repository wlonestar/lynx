#include "lynx/base/timestamp.h"

#include <vector>

void passByConstReference(const lynx::Timestamp &x) {
  printf("%s\n", x.toString().c_str());
}

void passByValue(lynx::Timestamp x) { printf("%s\n", x.toString().c_str()); }

void benchmark() {
  const int number = 1000 * 1000;

  std::vector<lynx::Timestamp> stamps;
  stamps.reserve(number);
  for (int i = 0; i < number; ++i) {
    stamps.push_back(lynx::Timestamp::now());
  }
  printf("%s\n", stamps.front().toString().c_str());
  printf("%s\n", stamps.back().toString().c_str());
  printf("%f\n", timeDiff(stamps.back(), stamps.front()));

  int increments[100] = {0};
  int64_t start = stamps.front().microsecsSinceEpoch();
  for (int i = 1; i < number; ++i) {
    int64_t next = stamps[i].microsecsSinceEpoch();
    int64_t inc = next - start;
    start = next;
    if (inc < 0) {
      printf("reverse!\n");
    } else if (inc < 100) {
      ++increments[inc];
    } else {
      printf("big gap %d\n", static_cast<int>(inc));
    }
  }

  for (int i = 0; i < 100; ++i) {
    printf("%2d: %d\n", i, increments[i]);
  }
}

int main() {
  lynx::Timestamp now(lynx::Timestamp::now());
  printf("%s\n", now.toString().c_str());
  passByValue(now);
  passByConstReference(now);
  benchmark();
}

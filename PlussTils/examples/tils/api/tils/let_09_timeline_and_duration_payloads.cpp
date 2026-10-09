#include <pluss/mic.h>
#include <pluss/tils/let.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // Timelines and durations are payloads too, so they can be bound the same way.
  tils::let dates = pluss::make_timeline({"2024-01-05", "2024-02-10"});
  std::cout << "timeline bound count: " << dates.get().numel() << '\n';
  return 0;
}

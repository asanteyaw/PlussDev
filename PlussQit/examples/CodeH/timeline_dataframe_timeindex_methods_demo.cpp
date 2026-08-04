#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  const auto idx = pluss::timeindex::make({
      "2024-01-01T00:00:00Z",
      "2024-01-01T00:15:00Z",
      "2024-01-01T00:45:00Z",
      "2024-01-01T01:05:00Z",
      "2024-01-01T01:25:00Z",
  });
  const auto t = idx.timeline();
  std::cout << "Timeline axis:\n" << pluss::timeline::print(t) << "\n\n";
  std::cout << "Inferred frequency: " << idx.freq() << "\n";
  std::cout << "Is monotonic: " << (idx.is_monotonic_increasing() ? "true" : "false") << "\n\n";

  const auto values = pluss::make_series(std::vector<double>{10.0, 20.0, 30.0, 40.0, 50.0}, {5});

  const auto hourly_sum = idx.resample_agg(values, "H", "sum", 1);
  const auto hourly_mean = idx.resample_agg(values, "H", "mean", 1);
  std::cout << "Resample sum (H):\n" << pluss::print_series(hourly_sum) << "\n\n";
  std::cout << "Resample mean (H):\n" << pluss::print_series(hourly_mean) << "\n\n";

  const auto roll_window = pluss::duration::minutes(45);
  const auto roll_sum = idx.rolling_agg(values, roll_window, "sum");
  const auto roll_max = idx.rolling_agg(values, roll_window, "max");
  std::cout << "Rolling sum (45min):\n" << pluss::print_series(roll_sum) << "\n\n";
  std::cout << "Rolling max (45min):\n" << pluss::print_series(roll_max) << "\n";

  return 0;
}

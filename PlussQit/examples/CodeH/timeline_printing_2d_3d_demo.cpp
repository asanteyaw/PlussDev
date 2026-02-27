#include <pluss/pluss.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  const auto t2 = pluss::make_timeline({
      {"2024-06-01T08:00:00Z", "2024-06-01T09:30:00Z", "2024-06-01T11:00:00Z"},
      {"2024-06-02T08:15:00Z", "2024-06-02T09:45:00Z", "2024-06-02T11:15:00Z"},
  });

  std::cout << "2D timeline (default print):\n";
  std::cout << pluss::print_timeline(t2) << "\n\n";

  const auto t3 = pluss::make_timeline({
      {
          {"2024-07-01T00:00:00Z", "2024-07-01T01:00:00Z"},
          {"2024-07-01T02:00:00Z", "2024-07-01T03:00:00Z"},
      },
      {
          {"2024-07-02T00:00:00Z", "2024-07-02T01:00:00Z"},
          {"2024-07-02T02:00:00Z", "2024-07-02T03:00:00Z"},
      },
  });

  pluss::codeh::TimelinePrintOptions opt;
  opt.max_rows = 4;
  opt.max_cols = 4;
  opt.max_slices = 4;
  opt.format = "%Y-%m-%dT%H:%M:%S%z";

  std::cout << "3D timeline (custom print options):\n";
  std::cout << pluss::print_timeline(t3, opt) << "\n\n";

  const std::vector<std::string> lvalue_flat{
      "2025-01-01T00:00:00Z", "2025-01-01T06:00:00Z",
      "2025-01-02T00:00:00Z", "2025-01-02T06:00:00Z"};
  const std::vector<std::int64_t> lvalue_shape{2, 2};
  const auto lvalue_t = pluss::make_timeline(lvalue_flat, lvalue_shape);

  std::cout << "Lvalue make_timeline(flat, shape) result:\n";
  std::cout << lvalue_t << "\n";
  return 0;
}

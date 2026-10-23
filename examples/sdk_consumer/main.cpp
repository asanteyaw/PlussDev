#include <cstdint>
#include <iostream>
#include <type_traits>

#include <pluss/ma.h>
#include <pluss/pluss.h>

int main() {
  static_assert(std::is_same_v<i64_t, std::int64_t>);
  static_assert(std::is_same_v<f64_t, double>);

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0});
  const auto mean = x.mean().item<double>();
  const auto area = pluss::integ::trapezoid(x, 1.0).item<double>();
  const pluss::Series scalar = as_f64(2.0, pluss::Config{});
  if (pluss::as_f64(scalar) != 2.0 ||
      pluss::as_i64().dtype != pluss::ScalarType::Int64) {
    return 1;
  }

  // Header-surface checks for default umbrella (Qit + Mic + Pl).
  [[maybe_unused]] pluss::pl::Rgba accent =
      pluss::pl::Rgba::from_u8(42, 95, 153);
  using TableType = pluss::table::DataFrame;
  [[maybe_unused]] constexpr bool k_table_visible =
      !std::is_same_v<TableType, void>;

  std::cout << mean << ' ' << area << '\n';
  return 0;
}

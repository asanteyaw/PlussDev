#include <iostream>
#include <type_traits>

#include <pluss/pluss.h>

int main() {
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0});
  const auto mean = x.mean().item<double>();

  // Header-surface checks for default umbrella (Qit + Mic + Pl).
  [[maybe_unused]] pluss::pl::Rgba accent = pluss::pl::Rgba::from_u8(42, 95, 153);
  using TableType = pluss::table::DataFrame;
  [[maybe_unused]] constexpr bool k_table_visible = !std::is_same_v<TableType, void>;

  std::cout << mean << '\n';
  return 0;
}

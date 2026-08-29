#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string join_i64(const std::vector<std::int64_t>& values) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) out << ", ";
    out << values[i];
  }
  out << "]";
  return out.str();
}

} // namespace

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2}, {4}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.6, 0.9}, {4}, f64))},
      {"late", Thread(pluss::make_series({0, 0, 0, 0}, {4}, f64))},
      {"region", Thread(pluss::make_note({"north", "west", "north", "south"}))},
  });

  const std::vector<std::string> ordered_names =
      {"income", "balance", "late", "region"};

  const lymph::prep::Route named =
      lymph::prep::route::names({"income", "balance"});
  const lymph::prep::Route numeric = lymph::prep::route::where(
      [](std::string_view, std::string_view dtype) {
        return dtype == "float64";
      });
  const lymph::prep::Route categorical =
      lymph::prep::route::names({"region"});
  const lymph::prep::Route remainder =
      lymph::prep::route::remaining_of(categorical);

  std::cout << "== names ==\n";
  std::cout << "named = " << join_i64(named.resolve(raw, ordered_names)) << "\n\n";

  std::cout << "== where_and_remainder ==\n";
  std::cout << "numeric = " << join_i64(numeric.resolve(raw, ordered_names)) << "\n";
  std::cout << "categorical = "
            << join_i64(categorical.resolve(raw, ordered_names)) << "\n";
  std::cout << "remainder = "
            << join_i64(remainder.resolve(raw, ordered_names)) << "\n";

  return 0;
}

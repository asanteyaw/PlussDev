// What the statistics do with each kind: a missing cell is skipped, a NaN makes the result NaN, an infinity behaves as arithmetic says.
#include <pluss/pluss.h>

#include <iostream>

using pluss::table::Shelf;

namespace {
double value(const pluss::table::ShelfValue& v) { return v.to_dbl(); }
}  // namespace

int main() {
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const double inf = std::numeric_limits<double>::infinity();
  const auto present = pluss::make_series(std::vector<bool>{true, true, true, false, true});

  // Ordinary numbers and one missing cell: the missing cell is skipped, so the mean is over the four that are there.
  const auto with_null = Shelf(pluss::make_series({2.0, 4.0, 6.0, 0.0, 8.0}), present);
  std::cout << "missing cell:   count " << value(with_null.count()) << "  mean " << value(with_null.mean()) << "  sum " << value(with_null.sum()) << "\n";

  // A NaN is a value, not a gap: it is counted, and every statistic that adds it up becomes NaN.
  const auto with_nan = Shelf(pluss::make_series({2.0, 4.0, nan, 0.0, 8.0}), present);
  std::cout << "NaN value:      count " << value(with_nan.count()) << "  mean " << value(with_nan.mean()) << "  sum " << value(with_nan.sum())
            << "  max " << value(with_nan.max()) << "\n";

  // An infinity is a value too, and it propagates the way arithmetic says it should.
  const auto with_inf = Shelf(pluss::make_series({2.0, 4.0, inf, 0.0, 8.0}), present);
  std::cout << "+inf value:     count " << value(with_inf.count()) << "  mean " << value(with_inf.mean()) << "  sum " << value(with_inf.sum())
            << "  max " << value(with_inf.max()) << "  min " << value(with_inf.min()) << "\n";

  // When a statistic should ignore the NaN, remove it first. dropnan keeps the infinities; dropinvalid removes everything that is not ordinary.
  std::cout << "after dropnan:  mean " << value(with_nan.dropnan().mean()) << "\n";
  std::cout << "inf, dropinf:   mean " << value(with_inf.dropinf().mean()) << "\n";
  std::cout << "inf, dropnan:   mean " << value(with_inf.dropnan().mean()) << "\n";
}

// data::FeatureBatch example 1 -- building by hand in both layouts.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // 4 samples, 3 features, row-major.
  const pluss::Series rows = pluss::make_series(
      {1.0, 2.0, 3.0,
       4.0, 5.0, 6.0,
       7.0, 8.0, 9.0,
       10.0, 11.0, 12.0},
      {12}, f64).reshape({4, 3});

  cls::FeatureBatch row_major{};
  row_major.x = rows;
  row_major.layout = cls::ObservationLayout::Rows;
  std::cout << "row_major.defined()      = " << std::boolalpha << row_major.defined() << "\n";
  std::cout << "row_major.sample_count()  = " << row_major.sample_count() << "\n";
  std::cout << "row_major.feature_count() = " << row_major.feature_count() << "\n";

  // The transpose of the same data, column-major -- same samples/features.
  const pluss::Series cols = pluss::make_series(
      {1.0, 4.0, 7.0, 10.0,
       2.0, 5.0, 8.0, 11.0,
       3.0, 6.0, 9.0, 12.0},
      {12}, f64).reshape({3, 4});
  cls::FeatureBatch col_major{};
  col_major.x = cols;
  col_major.layout = cls::ObservationLayout::Columns;
  std::cout << "col_major.sample_count()  = " << col_major.sample_count() << "\n";
  std::cout << "col_major.feature_count() = " << col_major.feature_count() << "\n";

  const cls::FeatureBatch empty{};
  std::cout << "empty.defined() = " << empty.defined() << "\n";

  return 0;
}

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // 3 samples, 4 features, Rows form:
  const cls::Series row_data = pluss::make_series(
      {0.0, 1.0, 2.0, 3.0,
       10.0, 11.0, 12.0, 13.0,
       20.0, 21.0, 22.0, 23.0},
      {12}, f64).reshape({3, 4});
  cls::FeatureBatch row_batch{};
  row_batch.x = row_data;
  row_batch.layout = cls::ObservationLayout::Rows;

  cls::FeatureBatch col_batch{};
  col_batch.x = row_data.transpose(0, 1).contiguous();
  col_batch.layout = cls::ObservationLayout::Columns;

  const cls::FeatureBatch row_reduced = cls::take_features(row_batch, {1, 3}).value();
  const cls::FeatureBatch col_reduced = cls::take_features(col_batch, {1, 3}).value();
  std::cout << "row_reduced.layout is Rows    = " << std::boolalpha << (row_reduced.layout == cls::ObservationLayout::Rows) << "\n";
  std::cout << "col_reduced.layout is Columns = " << (col_reduced.layout == cls::ObservationLayout::Columns) << "\n";

  const double diff = (row_reduced.x - col_reduced.x.transpose(0, 1).contiguous())
                           .abs().amax(0, false).amax(0, false).item<double>();
  std::cout << "max abs diff (same values, either layout) = " << diff << "\n";

  return 0;
}

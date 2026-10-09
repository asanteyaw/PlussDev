#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // a raw text-free numeric label column, arriving as 10 / 20 / 30 rather than 0 / 1 / 2.
  const Slate raw_labels(pluss::make_series({20.0, 10.0, 30.0, 20.0}, {4}, f64));

  cls::prep::LabelEncoder encoder;
  const pluss::Series labels = encoder.learn_map(raw_labels).value();
  std::cout << "encoded labels = " << labels << "\n";

  return 0;
}

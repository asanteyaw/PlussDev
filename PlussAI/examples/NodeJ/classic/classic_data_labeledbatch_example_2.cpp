// data::LabeledBatch example 2 -- validate_labeled on a length mismatch and a
// missing label.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::LabeledBatch mismatched{};
  mismatched.features.x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({4, 1});
  mismatched.y = pluss::make_series({0.0, 1.0, 1.0}, {3}, f64);  // 3 labels, 4 samples
  const cls::Status s1 = cls::validate_labeled(mismatched);
  std::cout << "mismatched: ok = " << std::boolalpha << s1.ok() << ", message = \"" << s1.message << "\"\n";

  cls::LabeledBatch no_label{};
  no_label.features.x = pluss::make_series({1.0, 2.0}, {2}, f64).reshape({2, 1});
  const cls::Status s2 = cls::validate_labeled(no_label);
  std::cout << "no_label:   ok = " << s2.ok() << ", message = \"" << s2.message << "\"\n";

  cls::LabeledBatch valid{};
  valid.features.x = pluss::make_series({1.0, 2.0}, {2}, f64).reshape({2, 1});
  valid.y = pluss::make_series({0.0, 1.0}, {2}, f64);
  const cls::Status s3 = cls::validate_labeled(valid);
  std::cout << "valid:      ok = " << s3.ok() << "\n";

  return 0;
}

#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// groups(Series): per-row entity keys for grouped cross-validation. The
// Payload just carries them -- has_groups() reflects the attachment and
// groups() reads the vector back; selection::validate's grouped plan is the
// consumer.
int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"x1", Thread(pluss::make_series({1.0, 1.2, 3.0, 3.2, 5.0, 5.2, 7.0, 7.2}, {8}, f64))},
      {"x2", Thread(pluss::make_series({0.5, 0.7, 2.5, 2.7, 4.5, 4.7, 6.5, 6.7}, {8}, f64))},
      {"y",  Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });
  // Four entities, two rows each.
  const pluss::Series groups = pluss::make_series({0, 0, 1, 1, 2, 2, 3, 3}, {8}, f64);

  const workflow::Payload without = workflow::payload::make(raw).features({"x1", "x2"}).label("y");
  const workflow::Payload with_groups =
      workflow::payload::make(raw).features({"x1", "x2"}).label("y").groups(groups);

  std::cout << "without.has_groups()     = " << std::boolalpha << without.has_groups() << "\n";
  std::cout << "with_groups.has_groups() = " << with_groups.has_groups() << "\n";
  std::cout << "with_groups.groups().numel() = " << with_groups.groups().numel() << "\n";

  // The label/feature roles are untouched by attaching groups.
  const lymph::LabeledBatch batch = with_groups.labeled_batch();
  std::cout << "labeled_batch: rows=" << batch.features.sample_count()
            << " features=" << batch.features.feature_count() << "\n";

  return 0;
}

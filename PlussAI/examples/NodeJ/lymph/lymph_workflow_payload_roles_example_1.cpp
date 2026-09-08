#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// features(names): pin the exact feature columns and their order. Without
// it, every non-label / non-weight column is used -- naming them makes the
// input contract explicit and stable if the frame later grows columns.
int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 2.6, 3.0}, {6}, f64))},
      {"balance", Thread(pluss::make_series({0.6, 1.0, 1.3, 1.6, 1.9, 2.2}, {6}, f64))},
      {"audit_note", Thread(pluss::make_series({9.0, 9.0, 9.0, 9.0, 9.0, 9.0}, {6}, f64))},
      {"default", Thread(pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64))},
  });

  // Explicit: only income + balance are features; audit_note is ignored.
  const workflow::Payload pinned = workflow::payload::make(raw)
                                       .features({"income", "balance"})
                                       .label("default");
  std::cout << "pinned  labeled_batch features = "
            << pinned.labeled_batch().features.feature_count() << "\n";

  // Implicit: feature_names left empty -> audit_note is pulled in too.
  const workflow::Payload implicit = workflow::payload::make(raw).label("default");
  std::cout << "implicit labeled_batch features = "
            << implicit.labeled_batch().features.feature_count() << "\n";

  return 0;
}

#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// Two label edge cases: (1) the later .label(...) call wins -- has_label()
// and has_label_feature() are mutually exclusive; (2) calling
// labeled_batch() on a Payload with no label role at all throws.
int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Slate(pluss::make_series({1.0, 1.4, 1.8, 2.2, 2.6, 3.0}, {6}, f64))},
      {"balance", Slate(pluss::make_series({0.6, 1.0, 1.3, 1.6, 1.9, 2.2}, {6}, f64))},
      {"default", Slate(pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64))},
  });
  const pluss::Series y = pluss::make_series({1, 1, 1, 0, 0, 0}, {6}, f64);

  // name first, then Series -- the Series call wins.
  const workflow::Payload series_wins =
      workflow::payload::make(raw).features({"income", "balance"}).label("default").label(y);
  std::cout << "name then Series: has_label()=" << std::boolalpha << series_wins.has_label()
            << " has_label_feature()=" << series_wins.has_label_feature() << "\n";

  // Series first, then name -- the name call wins.
  const workflow::Payload name_wins =
      workflow::payload::make(raw).features({"income", "balance"}).label(y).label("default");
  std::cout << "Series then name: has_label()=" << name_wins.has_label()
            << " has_label_feature()=" << name_wins.has_label_feature() << "\n";

  // No label role at all -> labeled_batch() throws.
  const workflow::Payload unlabeled =
      workflow::payload::make(raw).features({"income", "balance"});
  try {
    (void)unlabeled.labeled_batch();
    std::cout << "labeled_batch(): no error (unexpected)\n";
  } catch (const std::exception& e) {
    std::cout << "labeled_batch() threw: " << e.what() << "\n";
  }

  return 0;
}

#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// time() builds a chronological (expanding-window) plan: every test fold is
// strictly after its training rows, and shuffling is forced off. splits(frame,
// time_feature) hands back the ordered fold boundaries.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series demand = pluss::make_series(
      {10., 12., 13., 15., 16., 18., 19., 21., 22., 24., 25., 27.}, {12}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1}, {12}, f64);
  const Slate when(pluss::make_timeline({
      "2026-01-01T00:00:00Z", "2026-01-02T00:00:00Z", "2026-01-03T00:00:00Z",
      "2026-01-04T00:00:00Z", "2026-01-05T00:00:00Z", "2026-01-06T00:00:00Z",
      "2026-01-07T00:00:00Z", "2026-01-08T00:00:00Z", "2026-01-09T00:00:00Z",
      "2026-01-10T00:00:00Z", "2026-01-11T00:00:00Z", "2026-01-12T00:00:00Z"}));
  const DataFrame frame({{"demand", Slate(demand)}, {"target", Slate(y)},
                         {"when", when}});

  // folds = 3, horizon (test rows per fold) = 2.
  lymph::options::CrossVal cv{};
  cv.folds = 3;
  cv.time_test_size = 2;
  const lymph::selection::Plan plan = lymph::selection::time(cv);

  const auto splits = plan.splits(frame, "when");
  std::cout << "chronological folds = " << splits.size() << "\n";
  for (std::size_t i = 0; i < splits.size(); ++i) {
    std::cout << "  fold " << i
              << ": train_rows=" << splits[i].train_set.ncases()
              << " test_rows=" << splits[i].test_set.ncases() << "\n";
  }

  return 0;
}

#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// lookback(n) caps the training window at n rows. With it set, each fold drops
// its oldest rows instead of accumulating them -- an expanding window becomes a
// sliding one. gap(n) additionally skips n rows between train and test.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series demand = pluss::make_series(
      {10., 11., 12., 13., 14., 15., 16., 17., 18.,
       19., 20., 21., 22., 23., 24., 25., 26., 27.}, {18}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1}, {18}, f64);
  const Thread when(pluss::make_timeline({
      "2026-01-01T00:00:00Z", "2026-01-02T00:00:00Z", "2026-01-03T00:00:00Z",
      "2026-01-04T00:00:00Z", "2026-01-05T00:00:00Z", "2026-01-06T00:00:00Z",
      "2026-01-07T00:00:00Z", "2026-01-08T00:00:00Z", "2026-01-09T00:00:00Z",
      "2026-01-10T00:00:00Z", "2026-01-11T00:00:00Z", "2026-01-12T00:00:00Z",
      "2026-01-13T00:00:00Z", "2026-01-14T00:00:00Z", "2026-01-15T00:00:00Z",
      "2026-01-16T00:00:00Z", "2026-01-17T00:00:00Z", "2026-01-18T00:00:00Z"}));
  const DataFrame frame({{"demand", Thread(demand)}, {"target", Thread(y)},
                         {"when", when}});

  lymph::options::CrossVal cv{};
  cv.folds = 3;
  cv.time_test_size = 3;
  const lymph::selection::Plan sliding =
      lymph::selection::time(cv).lookback(6).gap(1);

  const auto splits = sliding.splits(frame, "when");
  std::cout << "sliding folds = " << splits.size() << "\n";
  for (std::size_t i = 0; i < splits.size(); ++i) {
    std::cout << "  fold " << i
              << ": train_rows=" << splits[i].train_set.ncases()
              << " test_rows=" << splits[i].test_set.ncases() << "\n";
  }

  return 0;
}

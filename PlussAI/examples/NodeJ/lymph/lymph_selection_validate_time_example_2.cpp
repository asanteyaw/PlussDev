#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// last(frame, time_feature) is the single chronological holdout -- train on
// everything up to a cut point, test on the final block. It is the "will this
// hold up on the next period?" check.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series demand = pluss::make_series(
      {10., 12., 13., 15., 16., 18., 19., 21., 22., 24., 25., 27.}, {12}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1}, {12}, f64);
  const Thread when(pluss::make_timeline({
      "2026-01-01T00:00:00Z", "2026-01-02T00:00:00Z", "2026-01-03T00:00:00Z",
      "2026-01-04T00:00:00Z", "2026-01-05T00:00:00Z", "2026-01-06T00:00:00Z",
      "2026-01-07T00:00:00Z", "2026-01-08T00:00:00Z", "2026-01-09T00:00:00Z",
      "2026-01-10T00:00:00Z", "2026-01-11T00:00:00Z", "2026-01-12T00:00:00Z"}));
  const DataFrame frame({{"demand", Thread(demand)}, {"target", Thread(y)},
                         {"when", when}});

  lymph::options::CrossVal cv{};
  cv.folds = 3;
  cv.time_test_size = 3;  // final 3 rows are the test block
  const lymph::selection::Plan plan = lymph::selection::time(cv);

  const lymph::prep::TimeSeriesSplit tail = plan.last(frame, "when");
  std::cout << "train_rows = " << tail.train_set.ncases()
            << ", test_rows = " << tail.test_set.ncases() << "\n";
  std::cout << "last train 'when' = " << tail.train_set.features("when").tail(1)
            << "first test 'when' = " << tail.test_set.features("when").head(1);

  return 0;
}

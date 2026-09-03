#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// stratify(feature) keeps the split's class balance close to the whole set's;
// seed(...) makes the shuffle reproducible. The two stratify() overloads take
// either a column name or an explicit Series.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  //                             four positives only
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1}, {16}, f64);
  const DataFrame frame({{"x", Thread(x)}, {"target", Thread(y)}});

  const lymph::prep::HoldoutSplit a =
      lymph::selection::holdout().test_size(0.25).stratify("target").seed(7).split(frame);
  const lymph::prep::HoldoutSplit b =
      lymph::selection::holdout().test_size(0.25).stratify("target").seed(7).split(frame);

  std::cout << "run A: train=" << a.train_set.ncases() << " test=" << a.test_set.ncases() << "\n";
  std::cout << "run B: train=" << b.train_set.ncases() << " test=" << b.test_set.ncases() << "\n";
  std::cout << "same seed -> identical test labels: "
            << (a.test_set.features("target").as_series().to_vec<double>()
                == b.test_set.features("target").as_series().to_vec<double>()
                ? "yes" : "no")
            << "\n";

  return 0;
}

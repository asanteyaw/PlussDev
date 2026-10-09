#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // Fit the scaler on 10..50, then map a frame whose values run past both ends
  // of the learned range.
  const pluss::Series train = pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}, {5}, f64);
  const pluss::Series later = pluss::make_series({0.0, 25.0, 50.0, 75.0}, {4}, f64);
  const DataFrame train_df({{"v", Slate(train)}});
  const DataFrame later_df({{"v", Slate(later)}});

  // clip = false (default): out-of-range inputs map outside [0, 1].
  lymph::prep::MinMaxScaler open{lymph::options::MinMaxScaler{}};
  open.learn(train_df);

  // clip = true: results are held inside [0, 1].
  lymph::options::MinMaxScaler clipped_opts{};
  clipped_opts.clip = true;
  lymph::prep::MinMaxScaler clipped{clipped_opts};
  clipped.learn(train_df);

  std::cout << "clip = false (0 -> -0.25, 75 -> 1.625):\n";
  std::cout << open.map(later_df) << "\n";
  std::cout << "clip = true (held to [0, 1]):\n";
  std::cout << clipped.map(later_df);

  return 0;
}

#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame train({{"region", Thread(pluss::make_series({10., 20., 30., 10., 20., 30.}, {6}, f64))}});

  // one_based defaults to true (0 reserved for unseen). Set it false to get a
  // dense 0..K-1 coding -- the shape a downstream index-based step expects.
  lymph::prep::OrdinalEncoder zero_based(
      lymph::prep::OrdinalEncoderOptions{.one_based = false});
  std::cout << "one_based = false:\n"
            << zero_based.features({"region"}).learn_map(train) << "\n";

  // The single-column Thread overload takes one column and returns one Series,
  // and keeps string category labels in mapped_names_ (the DataFrame path over a
  // note-backed column hashes them instead).
  lymph::prep::OrdinalEncoder e;
  const pluss::Series codes =
      e.learn_map(Thread(pluss::make_note({"low", "mid", "high", "mid", "low"})));
  std::cout << "Thread overload:\n" << codes << "\n";

  return 0;
}

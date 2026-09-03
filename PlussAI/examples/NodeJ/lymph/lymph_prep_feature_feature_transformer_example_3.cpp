#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <string>
#include <vector>

// The factory has three families of overloads for supplying the ordered feature
// space: from a frame (feature order = frame.features()), from a frame plus
// explicit DataFrameBatchOptions, or from an explicit name list with no frame.
// All three build the same transformer given the same branches.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame raw({
      {"a", Thread(pluss::make_series({1., 2., 3., 4.}, {4}, f64))},
      {"b", Thread(pluss::make_series({40., 30., 20., 10.}, {4}, f64))},
  });
  const auto batch = [&] {
    return lymph::workflow::payload::make(raw).features({"a", "b"}).feature_batch();
  };

  auto branches = [] {
    return std::vector<lymph::prep::branch::Branch>{
        lymph::prep::branch::minmax_scaler({}, {"a", "b"})};
  };

  lymph::prep::FeatureTransformer from_frame = lymph::prep::feature_transformer(raw, branches());
  lymph::prep::FeatureTransformer from_names = lymph::prep::feature_transformer(
      std::vector<std::string>{"a", "b"}, branches(),
      lymph::prep::FeatureRemainderPolicy::PassThrough);

  const Series o1 = lymph::resolve(from_frame.learn_map(batch()), "f1");
  const Series o2 = lymph::resolve(from_names.learn_map(batch()), "f2");

  std::cout << "from frame:\n" << o1 << "\n";
  std::cout << "from name list:\n" << o2 << "\n";
  std::cout << "identical = " << std::boolalpha
            << (o1.sub(o2).abs().max().item<double>() == 0.0) << "\n";

  return 0;
}

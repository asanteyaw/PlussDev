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

  // Standalone with n_components = 4. Each row lands in exactly one bucket;
  // two distinct categories can collide into the same bucket by design.
  const pluss::Series c = pluss::make_series({0,1,2,3,4,0,1,2}, {8}, f64);
  const DataFrame frame({{"c", Thread(c)}});

  lymph::options::HashingEncoder o{};
  o.n_components = 4;
  lymph::prep::HashingEncoder enc{o};
  enc.features({"c"});
  const DataFrame out = enc.learn_map(frame);
  std::cout << "5 categories -> " << out.width() << " fixed buckets\n" << out;

  return 0;
}

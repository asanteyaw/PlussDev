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

  // A column with both signs and a zero. The largest magnitude is 8.
  const pluss::Series v = pluss::make_series({-8.0, -2.0, 0.0, 1.0, 4.0, 6.0}, {6}, f64);
  const DataFrame frame({{"v", Thread(v)}});

  // maxabs divides by max(|value|) = 8. Signs are kept, zero stays zero, and
  // every result lands in [-1, 1]. The stage factory and the standalone object
  // take the same (single-field) option struct.
  const lymph::prep::Stage staged = lymph::prep::stage::maxabs_scaler();
  std::cout << "staged is a MaxAbsScaler stage: " << std::boolalpha
            << (staged.kind == lymph::TransformKind::MaxAbsScaler) << "\n\n";

  std::cout << lymph::prep::MaxAbsScaler{}.learn_map(frame);

  return 0;
}

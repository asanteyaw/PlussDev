#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// time(Series values): attach the time role from an already-separate vector
// (has_time() becomes true, time() returns it). And: time(TimeOptions) is
// validated on the spot -- a negative horizon throws before the Payload is
// even built.
int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2}, {4}, f64))},
      {"balance", Thread(pluss::make_series({0.6, 1.0, 1.3, 1.6}, {4}, f64))},
  });
  const pluss::Series t = pluss::make_series({0.0, 1.0, 2.0, 3.0}, {4}, f64);

  const workflow::Payload payload =
      workflow::payload::make(raw).features({"income", "balance"}).time(t);
  std::cout << "has_time()      = " << std::boolalpha << payload.has_time() << "\n";
  std::cout << "time().defined()= " << payload.time().defined() << "\n";

  workflow::TimeOptions invalid{};
  invalid.index = "observed_at";
  invalid.horizon = -1;
  try {
    (void)workflow::payload::make(raw).features({"income", "balance"}).time(invalid);
    std::cout << "time(invalid): no error (unexpected)\n";
  } catch (const std::exception& e) {
    std::cout << "time(invalid) threw: " << e.what() << "\n";
  }

  return 0;
}

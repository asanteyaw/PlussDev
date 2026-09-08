#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// time(std::string feature_name): the simplest time role -- just name the
// column. time_feature() reads it back; time_options() stays empty (no
// horizon/gap/lookback contract attached).
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
      {"default", Thread(pluss::make_series({0, 0, 1, 1}, {4}, f64))},
      {"observed_at", Thread(pluss::make_timeline({
          "2026-01-01T00:00:00Z", "2026-01-02T00:00:00Z",
          "2026-01-03T00:00:00Z", "2026-01-04T00:00:00Z"}))},
  });

  const workflow::Payload payload = workflow::payload::make(raw)
                                        .features({"income", "balance"})
                                        .label("default")
                                        .time("observed_at");

  std::cout << "has_time()               = " << std::boolalpha << payload.has_time() << "\n";
  std::cout << "time_feature()           = " << payload.time_feature() << "\n";
  std::cout << "time_options().has_value = " << payload.time_options().has_value() << "\n";

  return 0;
}

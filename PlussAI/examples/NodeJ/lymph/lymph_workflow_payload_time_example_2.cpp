#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// time(TimeOptions options): the full leakage-aware time contract. It is
// validated immediately, and time_options() reads the whole struct back --
// selection::validate::time fills any of its own unset window fields from it.
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

  workflow::TimeOptions time_options{};
  time_options.index = "observed_at";
  time_options.horizon = workflow::steps(2);
  time_options.gap = workflow::steps(1);

  const workflow::Payload payload = workflow::payload::make(raw)
                                        .features({"income", "balance"})
                                        .label("default")
                                        .time(time_options);

  std::cout << "time_feature()             = " << payload.time_feature() << "\n";
  std::cout << "time_options().has_value() = " << std::boolalpha
            << payload.time_options().has_value() << "\n";
  std::cout << "time_options()->horizon    = " << payload.time_options()->horizon << "\n";
  std::cout << "time_options()->gap        = " << payload.time_options()->gap << "\n";

  return 0;
}

#include <pluss/ai/procedure.h>
#include <pluss/ai/lymph.h>
#include <pluss/mic.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;
  namespace lymph = pluss::ai::lymph;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame raw({
      {"id", Thread(pluss::make_series(std::vector<double>{101.0, 102.0, 103.0}, {3}, cfg))},
      {"timestamp", Thread(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3}, cfg))},
      {"income", Thread(pluss::make_series(std::vector<double>{50.0, 80.0, 35.0}, {3}, cfg))},
      {"bad", Thread(pluss::make_series(std::vector<double>{0.0, 0.0, 1.0}, {3}, cfg))},
  });

  lymph::Payload payload = lymph::payload::make(raw)
      .features({"income"})
      .label("bad")
      .time(lymph::TimeOptions{.index = "timestamp", .horizon = 1});

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("gate", "raw", "clean", lymph::Policy{
          .drop_columns = {"id"},
          .require_columns = {"timestamp", "income", "bad"},
          .reject_missing = {"timestamp", "bad"},
      }),
  });

  const ai::ProcedureResult out = proc.run({ai::procedure::input("raw", payload)});
  const lymph::Payload clean = out.get<lymph::Payload>("clean");
  std::cout << "has_time=" << clean.has_time() << "\n";
  std::cout << "time_feature=" << clean.time_feature() << "\n";
  std::cout << "clean_features=" << clean.frame().nfeatures() << "\n";
  return 0;
}

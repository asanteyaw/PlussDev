#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const Series x = pluss::series::linspace(1.0, 40.0, 40, ScalarType::Float64);
  const Series y = x.mul(0.7).add(x.sqrt());
  const DataFrame raw({
      {"record_id", Thread(pluss::series::arange(0, 40, 1, pluss::Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU)))},
      {"x", Thread(x)},
      {"sqrt_x", Thread(x.sqrt())},
      {"y", Thread(y)},
  });

  lymph::Policy gate = lymph::policy({
      .drop_columns = {"record_id"},
      .require_columns = {"x", "sqrt_x", "y"},
      .reject_missing = {"y"},
  });
  lymph::Payload data = lymph::payload::make(raw).features({"x", "sqrt_x"}).label("y");

  lymph::Workflow wf = lymph::workflow::make(
      gate,
      std::vector<lymph::WorkflowBranchSpec>{
          lymph::workflow::branch("num", {"x", "sqrt_x"}, {{"scale", lymph::stage::standard_scaler()}}),
      },
      lymph::models::linear_regression());

  wf.learn_(data);
  std::cout << "policy payload r2: " << wf.assess(data).r2.item<double>() << "\n";
  return 0;
}

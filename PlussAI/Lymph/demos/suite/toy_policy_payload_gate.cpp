#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const Series x = pluss::series::linspace(1.0, 40.0, 40, ScalarType::Float64);
  const Series y = x.mul(0.7).add(x.sqrt());
  const DataFrame raw({
      {"record_id", Slate(pluss::series::arange(0, 40, 1, pluss::Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU)))},
      {"x", Slate(x)},
      {"sqrt_x", Slate(x.sqrt())},
      {"y", Slate(y)},
  });

  lymph::workflow::Policy gate{
      .drop_columns = {"record_id"},
      .require_columns = {"x", "sqrt_x", "y"},
      .reject_missing = {"y"},
  };
  lymph::workflow::Payload data = lymph::workflow::payload::make(raw).features({"x", "sqrt_x"}).label("y");

  lymph::Workflow wf = lymph::workflow::make(
      gate,
      std::vector<lymph::WorkflowBranchSpec>{
          lymph::workflow::branch("num", {"x", "sqrt_x"}, {{"scale", lymph::prep::stage::standard_scaler()}}),
      },
      lymph::models::linear_regression());

  wf.learn_(data);
  std::cout << "policy payload r2: " << wf.assess(data).r2.item<double>() << "\n";
  return 0;
}

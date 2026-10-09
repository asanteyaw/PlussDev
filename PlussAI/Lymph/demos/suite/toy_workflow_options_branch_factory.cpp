#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const Series x = pluss::series::linspace(-1.0, 1.0, 50, ScalarType::Float64);
  const DataFrame raw({{"x", Slate(x)}, {"x2", Slate(x.square())}, {"y", Slate(x.gt(0.0).to(ScalarType::Float64))}});

  lymph::WorkflowOptions options{};
  options.workers = 4;
  options.schedule = lymph::WorkflowSchedule::Deterministic;
  options.cache = lymph::WorkflowCache::Off;

  lymph::Workflow wf = lymph::workflow::make(
      std::vector<lymph::WorkflowBranchSpec>{
          lymph::workflow::branch("shape", {"x", "x2"}, {
              {"scale", lymph::prep::stage::minmax_scaler()},
              {"norm", lymph::prep::stage::vector_normalizer()},
          }),
      },
      lymph::models::decision_tree_classifier({.max_depth = 3}),
      options);

  lymph::workflow::Payload data = lymph::workflow::payload::make(raw).features({"x", "x2"}).label("y");
  wf.learn_(data);
  std::cout << "workers: " << wf.workers() << "\n";
  std::cout << "branch count: " << wf.branch_count() << "\n";
  return 0;
}

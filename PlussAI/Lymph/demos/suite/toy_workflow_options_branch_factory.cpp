#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const Series x = pluss::series::linspace(-1.0, 1.0, 50, ScalarType::Float64);
  const DataFrame raw({{"x", Thread(x)}, {"x2", Thread(x.square())}, {"y", Thread(x.gt(0.0).to(ScalarType::Float64))}});

  lymph::WorkflowOptions options{};
  options.workers = 4;
  options.schedule = lymph::WorkflowSchedule::Deterministic;
  options.cache = lymph::WorkflowCache::Off;

  lymph::Workflow wf = lymph::workflow::make(
      std::vector<lymph::WorkflowBranchSpec>{
          lymph::workflow::branch("shape", {"x", "x2"}, {
              {"scale", lymph::stage::minmax_scaler()},
              {"norm", lymph::stage::vector_normalizer()},
          }),
      },
      lymph::model::decision_tree_classifier({.max_depth = 3}),
      options);

  lymph::Payload data = lymph::payload::make(raw).features({"x", "x2"}).label("y");
  wf.learn_(data);
  std::cout << "workers: " << wf.workers() << "\n";
  std::cout << "branch count: " << wf.branch_count() << "\n";
  return 0;
}

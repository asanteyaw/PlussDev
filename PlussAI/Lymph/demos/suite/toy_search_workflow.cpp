#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const Series x = pluss::series::linspace(-2.0, 2.0, 90, ScalarType::Float64);
  const Series x2 = x.square();
  const Series y = x.mul(2.2).sub(x2.mul(0.4)).add(0.5);
  const DataFrame raw({{"x", Thread(x)}, {"x2", Thread(x2)}, {"y", Thread(y)}});

  lymph::Payload data = lymph::payload::make(raw).features({"x", "x2"}).label("y");
  lymph::Workflow design = lymph::workflow::make(
      lymph::Policy{},
      std::vector<lymph::WorkflowBranchSpec>{
          lymph::workflow::branch("num", {"x", "x2"}, {
              {"scale", lymph::stage::standard_scaler()},
          }),
      });

  std::vector<lymph::GridAxis> space{
      {.name = "regularization_strength", .values = {0.0001, 0.001, 0.01}},
  };

  lymph::metrics::CrossValidationOptions cv{};
  cv.plan = lymph::CVPlanKind::CrossVal;
  cv.folds = 3;

  const lymph::search::Result tuned = lymph::search::grid(space, cv)
      .run([](const lymph::GridPoint& point) {
             return lymph::model::linear_regression({
                 .regularization_strength = point.at("regularization_strength"),
             });
           },
           design, data, {}, {}, lymph::MetricKind::Rmse,
           lymph::TuneObjectivePolicy::Minimize, {.fit_top = 1});

  lymph::Workflow final = design.with(tuned.best());
  final.learn_(data);
  const lymph::MetricReport report = final.assess(data);

  std::cout << "best search rmse: " << report.rmse.item<double>() << "\n";
  std::cout << "search top kept: " << tuned.top(1).size() << "\n";
  return 0;
}

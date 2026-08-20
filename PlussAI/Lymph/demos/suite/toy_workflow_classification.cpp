#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const Series x = pluss::series::linspace(-2.5, 2.5, 72, ScalarType::Float64);
  const Series energy = x.square().add(x.sin());
  const Series risk = x.mul(1.4).add(energy.mul(0.3));
  const Series target = risk.gt(0.8).to(ScalarType::Float64);

  const DataFrame raw({
      {"x", Thread(x)},
      {"energy", Thread(energy)},
      {"segment", Thread(pluss::make_note({"low", "low", "low", "low", "mid", "mid", "mid", "mid",
                                            "high", "high", "high", "high"}).repeat({6}))},
      {"target", Thread(target)},
  });

  lymph::prep::SimpleImputerOptions mode{};
  mode.strategy = lymph::prep::SimpleImputerStrategy::MostFrequent;
  lymph::prep::OneHotEncoderOptions unknown{};
  unknown.max_categories = 8;
  unknown.include_unknown_bucket = true;

  lymph::Payload data = lymph::payload::make(raw)
      .features({"x", "energy", "segment"})
      .label("target")
      .note(lymph::DataNotePolicy::Factorize);

  lymph::Workflow wf = lymph::workflow::make(
      std::vector<lymph::WorkflowBranchSpec>{
          lymph::workflow::branch("num", {"x", "energy"}, {
              {"scale", lymph::stage::standard_scaler()},
          }),
          lymph::workflow::branch("cat", {"segment"}, {
              {"fill", lymph::stage::simple_imputer(mode)},
              {"encode", lymph::stage::one_hot_encoder(unknown)},
          }),
      },
      lymph::model::logistic_regression({.regularization_strength = 0.02}));

  wf.learn_(data);
  const lymph::MetricReport score = wf.assess(data);
  const lymph::ConfusionTable table = lymph::reports::confusion_table(wf, data);
  const double threshold = lymph::reports::best_threshold(wf, data, lymph::MetricKind::F1);

  std::cout << "toy classification f1: " << score.f1.item<double>() << "\n";
  std::cout << "threshold: " << threshold << "\n";
  std::cout << "confusion cells: " << table.counts.numel() << "\n";
  return 0;
}

#include <pluss/pluss.h>

#include <iostream>

namespace {

void heading(const char* label) {
  std::cout << "=== " << label << " ===\n";
}

} // namespace

int main() {
  heading("no_grad_evaluation");
  auto weights = pluss::make_series({2.0, 3.0, 4.0});
  weights.tracks_slope_(true);
  pluss::Series prediction;
  {
    pluss::NoGradGuard guard;
    prediction = weights.square();
  }
  std::cout << "weights track: " << weights.tracks_slope() << "\n";
  std::cout << "prediction tracks: " << prediction.tracks_slope() << "\n";
  pluss::series::print(std::cout, prediction) << "\n\n";

  heading("recording_resumes");
  auto x = pluss::make_series({1.0, 2.0, 3.0});
  x.tracks_slope_(true);
  pluss::Series report;
  {
    pluss::NoGradGuard guard;
    report = x.mul(10.0);
  }
  x.square().sum().reverse();
  std::cout << "report tracks: " << report.tracks_slope() << "\n";
  pluss::series::print(std::cout, x.sensitivity()) << "\n\n";

  heading("nested_enable");
  auto nested = pluss::make_series({2.0, 3.0});
  nested.tracks_slope_(true);
  pluss::Series outside_graph;
  pluss::Series inside_graph;
  {
    pluss::NoGradGuard guard;
    outside_graph = nested.add(1.0);
    {
      pluss::AutoGradMode enable(true);
      inside_graph = nested.square();
    }
  }
  std::cout << "outer result tracks: " << outside_graph.tracks_slope() << "\n";
  std::cout << "inner result tracks: " << inside_graph.tracks_slope() << "\n\n";

  heading("pause_recording");
  auto paused = pluss::make_series({-2.0, 1.0, 4.0});
  paused.tracks_slope_(true);
  pluss::Series positive_mask;
  {
    pluss::PauseRecording pause;
    positive_mask = paused.gt(0.0);
  }
  paused.square().sum().reverse();
  std::cout << "mask tracks: " << positive_mask.tracks_slope() << "\n";
  pluss::series::print(std::cout, positive_mask) << "\n";
  pluss::series::print(std::cout, paused.sensitivity()) << "\n";
}

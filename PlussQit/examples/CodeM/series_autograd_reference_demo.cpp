#include <pluss/pluss.h>

#include <iostream>
#include <string>

namespace {

void heading(const char* label) {
  std::cout << "=== " << label << " ===\n";
}

const char* tf(bool value) {
  return value ? "true" : "false";
}

void print_error_prefix(const std::exception& e) {
  std::string msg = e.what();
  const auto nl = msg.find('\n');
  std::cout << msg.substr(0, nl);
}

} // namespace

int main() {
  heading("reverse_scalar_basic");
  auto scalar_x = pluss::make_series(1.25);
  scalar_x.tracks_slope_(true);
  auto scalar_y = scalar_x * scalar_x * scalar_x - 2.0 * (scalar_x * scalar_x) + scalar_x;
  scalar_y.reverse();
  std::cout << "y=" << scalar_y.item<double>() << "\n";
  std::cout << "dx=" << scalar_x.sensitivity().item<double>() << "\n\n";

  heading("reverse_vector_sum");
  auto sum_x = pluss::make_series({1.0, 2.0, 3.0});
  sum_x.tracks_slope_(true);
  auto sum_y = sum_x.mul(pluss::make_series({10.0, 20.0, 30.0}));
  sum_y.sum().reverse();
  std::cout << "y=" << sum_y << "\n";
  std::cout << "dx=" << sum_x.sensitivity() << "\n\n";

  heading("reverse_seeded");
  auto seeded_x = pluss::make_series({1.0, 2.0, 3.0});
  seeded_x.tracks_slope_(true);
  auto seeded_y = seeded_x.mul(seeded_x);
  seeded_y.reverse(pluss::make_series({1.0, 10.0, 100.0}));
  std::cout << "y=" << seeded_y << "\n";
  std::cout << "dx=" << seeded_x.sensitivity() << "\n\n";

  heading("reverse_unseeded_vector_error");
  auto bad_x = pluss::make_series({1.0, 2.0, 3.0});
  bad_x.tracks_slope_(true);
  auto bad_y = bad_x.mul(bad_x);
  try {
    bad_y.reverse();
  } catch (const std::exception& e) {
    print_error_prefix(e);
  }
  std::cout << "\n\n";

  heading("reverse_accumulate");
  auto acc_x = pluss::make_series(2.0);
  acc_x.tracks_slope_(true);
  auto acc_y = acc_x * acc_x;
  acc_y.reverse(true);
  std::cout << "first=" << acc_x.sensitivity().item<double>() << "\n";
  acc_y.reverse(true, true);
  std::cout << "second=" << acc_x.sensitivity().item<double>() << "\n\n";

  heading("reverse_reset_between_runs");
  auto reset_x = pluss::make_series(2.0);
  reset_x.tracks_slope_(true);
  auto reset_y = reset_x * reset_x;
  reset_y.reverse(true);
  std::cout << "first=" << reset_x.sensitivity().item<double>() << "\n";
  reset_y.reverse(true, false);
  std::cout << "second=" << reset_x.sensitivity().item<double>() << "\n\n";

  heading("reverse_untracked_leaf");
  auto silent_x = pluss::make_series({1.0, 2.0, 3.0});
  silent_x.mul(2.0).sum().reverse();
  std::cout << "has_dx=" << tf(silent_x.has_sensitivity()) << "\n\n";

  heading("tracks_slope_basic");
  auto track_flag = pluss::make_series({1.0, 2.0, 3.0});
  std::cout << "before=" << tf(track_flag.tracks_slope()) << "\n";
  track_flag.tracks_slope_();
  std::cout << "after_enable_default=" << tf(track_flag.tracks_slope()) << "\n";
  track_flag.tracks_slope_(false);
  std::cout << "after_disable=" << tf(track_flag.tracks_slope()) << "\n\n";

  heading("tracks_slope_controls_gradient");
  auto untracked = pluss::make_series({1.0, 2.0, 3.0});
  auto tracked = pluss::make_series({1.0, 2.0, 3.0});
  tracked.tracks_slope_(true);
  untracked.mul(2.0).sum().reverse();
  tracked.mul(2.0).sum().reverse();
  std::cout << "untracked_has_dx=" << tf(untracked.has_sensitivity()) << "\n";
  std::cout << "tracked_has_dx=" << tf(tracked.has_sensitivity()) << "\n";
  if (tracked.has_sensitivity()) {
    std::cout << "tracked_dx=" << tracked.sensitivity() << "\n";
  }
  std::cout << "\n";

  heading("tracks_slope_disable_clears");
  auto tracked_once = pluss::make_series({3.0, 4.0});
  tracked_once.tracks_slope_(true);
  tracked_once.mul(2.0).sum().reverse();
  std::cout << "before_disable=" << tf(tracked_once.has_sensitivity()) << "\n";
  tracked_once.tracks_slope_(false);
  std::cout << "after_disable=" << tf(tracked_once.has_sensitivity()) << "\n\n";

  heading("has_sensitivity_lifecycle");
  auto life = pluss::make_series({2.0, 4.0, 8.0});
  life.tracks_slope_(true);
  std::cout << "before=" << tf(life.has_sensitivity()) << "\n";
  life.log().sum().reverse();
  std::cout << "after_reverse=" << tf(life.has_sensitivity()) << "\n";
  life.clear_sensitivity();
  std::cout << "after_clear=" << tf(life.has_sensitivity()) << "\n\n";

  heading("sensitivity_basic");
  auto sens = pluss::make_series({
      {1.0, 2.0},
      {3.0, 4.0},
  });
  sens.tracks_slope_(true);
  sens.mul(3.0).sum().reverse();
  pluss::series::print(std::cout, sens.sensitivity()) << "\n\n";

  heading("sensitivity_before_reverse_zeros");
  auto sens_empty = pluss::make_series({1.0, 2.0, 3.0});
  sens_empty.tracks_slope_(true);
  try {
    std::cout << sens_empty.sensitivity();
  } catch (const std::exception& e) {
    print_error_prefix(e);
  }
  std::cout << "\n\n";

  heading("clear_sensitivity_reuse");
  auto reuse = pluss::make_series({1.0, 2.0, 3.0});
  reuse.tracks_slope_(true);
  reuse.mul(2.0).sum().reverse();
  std::cout << "first=" << reuse.sensitivity() << "\n";
  reuse.clear_sensitivity();
  reuse.mul(5.0).sum().reverse();
  std::cout << "second=" << reuse.sensitivity() << "\n\n";

  heading("clear_sensitivity_keeps_tracking");
  auto keep_track = pluss::make_series({9.0, 10.0});
  keep_track.tracks_slope_(true);
  keep_track.mul(4.0).sum().reverse();
  keep_track.clear_sensitivity();
  std::cout << "tracks=" << tf(keep_track.tracks_slope()) << "\n";
  std::cout << "has_dx=" << tf(keep_track.has_sensitivity()) << "\n\n";

  heading("clear_sensitivity_noop");
  auto noop = pluss::make_series({7.0, 8.0});
  noop.clear_sensitivity();
  std::cout << "has_dx=" << tf(noop.has_sensitivity()) << "\n";

  return 0;
}

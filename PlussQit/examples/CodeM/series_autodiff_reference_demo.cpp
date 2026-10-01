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
  life.clear();
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

  heading("clear_reuse");
  auto reuse = pluss::make_series({1.0, 2.0, 3.0});
  reuse.tracks_slope_(true);
  reuse.mul(2.0).sum().reverse();
  std::cout << "first=" << reuse.sensitivity() << "\n";
  reuse.clear();
  reuse.mul(5.0).sum().reverse();
  std::cout << "second=" << reuse.sensitivity() << "\n\n";

  heading("clear_keeps_tracking");
  auto keep_track = pluss::make_series({9.0, 10.0});
  keep_track.tracks_slope_(true);
  keep_track.mul(4.0).sum().reverse();
  keep_track.clear();
  std::cout << "tracks=" << tf(keep_track.tracks_slope()) << "\n";
  std::cout << "has_dx=" << tf(keep_track.has_sensitivity()) << "\n\n";

  heading("clear_noop");
  auto noop = pluss::make_series({7.0, 8.0});
  noop.clear();
  std::cout << "has_dx=" << tf(noop.has_sensitivity()) << "\n\n";

  heading("detach_preserves_values");
  auto detach_base = pluss::make_series({1.0, 2.0, 3.0});
  auto detached_view = detach_base.detach();
  std::cout << "base=" << detach_base << "\n";
  std::cout << "detached=" << detached_view << "\n\n";

  heading("detach_result_mutates_independently");
  auto detach_copy_base = pluss::make_series({1.0, 2.0, 3.0});
  auto detached_copy = detach_copy_base.detach();
  detached_copy.add_(10.0);
  std::cout << "base=" << detach_copy_base << "\n";
  std::cout << "detached=" << detached_copy << "\n\n";

  heading("detach_blocks_upstream_sensitivity");
  auto detach_x = pluss::make_series({1.0, 2.0, 3.0});
  detach_x.tracks_slope_(true);
  auto detach_y = detach_x.detach().mul(3.0);
  detach_y.sum().reverse();
  std::cout << "has_dx=" << tf(detach_x.has_sensitivity()) << "\n\n";

  heading("detach_clears_tracking_on_result");
  auto detach_state = pluss::make_series({4.0, 5.0});
  detach_state.tracks_slope_(true);
  auto detached_state = detach_state.detach();
  std::cout << "source_tracks=" << tf(detach_state.tracks_slope()) << "\n";
  std::cout << "detached_tracks=" << tf(detached_state.tracks_slope()) << "\n\n";

  heading("detach_inplace_blocks_existing_path");
  auto inplace_x = pluss::make_series({1.0, 2.0, 3.0});
  inplace_x.tracks_slope_(true);
  auto inplace_y = inplace_x.mul(2.0);
  inplace_y.detach_();
  inplace_y.sum().reverse();
  std::cout << "has_dx=" << tf(inplace_x.has_sensitivity()) << "\n\n";

  heading("detach_inplace_clears_tracking");
  auto inplace_state = pluss::make_series({6.0, 7.0});
  inplace_state.tracks_slope_(true);
  std::cout << "before=" << tf(inplace_state.tracks_slope()) << "\n";
  inplace_state.detach_();
  std::cout << "after=" << tf(inplace_state.tracks_slope()) << "\n\n";

  heading("retains_grad_default_and_enabled");
  auto retain_state_x = pluss::make_series({1.0, 2.0});
  retain_state_x.tracks_slope_(true);
  auto retain_state_mid = retain_state_x.mul(3.0);
  std::cout << "before=" << tf(retain_state_mid.retains_grad().item<bool>()) << "\n";
  retain_state_mid.retain_grad();
  std::cout << "after=" << tf(retain_state_mid.retains_grad().item<bool>()) << "\n\n";

  heading("retain_grad_intermediate");
  auto retain_x = pluss::make_series({1.0, 2.0, 3.0});
  retain_x.tracks_slope_(true);
  auto retained_mid = retain_x.mul(2.0);
  retained_mid.retain_grad();
  retained_mid.sum().reverse();
  std::cout << "retains=" << tf(retained_mid.retains_grad().item<bool>()) << "\n";
  std::cout << "mid_has_dx=" << tf(retained_mid.has_sensitivity()) << "\n";
  if (retained_mid.has_sensitivity()) {
    std::cout << "mid_dx=" << retained_mid.sensitivity() << "\n";
  }
  std::cout << "leaf_dx=" << retain_x.sensitivity() << "\n\n";

  heading("retain_grad_toggle_off");
  auto retain_off_x = pluss::make_series({1.0, 2.0});
  retain_off_x.tracks_slope_(true);
  auto retain_off_mid = retain_off_x.mul(4.0);
  retain_off_mid.retain_grad(false);
  retain_off_mid.sum().reverse();
  std::cout << "retains=" << tf(retain_off_mid.retains_grad().item<bool>()) << "\n";
  std::cout << "mid_has_dx=" << tf(retain_off_mid.has_sensitivity()) << "\n\n";

  heading("count_bool_true_false");
  auto mask = pluss::make_series({true, false, true, true, false}, pluss::ScalarType::Bool);
  std::cout << "true_count=" << mask.count() << "\n";
  std::cout << "false_count=" << mask.count(false) << "\n\n";

  heading("count_numeric_total");
  auto numeric = pluss::make_series({
      {5.0, 0.0, -2.0},
      {8.0, 9.0, 10.0},
  });
  pluss::series::print(std::cout, numeric.count()) << "\n\n";

  heading("count_numeric_false_error");
  try {
    std::cout << numeric.count(false) << "\n";
  } catch (const std::exception& e) {
    print_error_prefix(e);
  }
  std::cout << "\n\n";

  heading("storage_version_nonmutating_read");
  auto versioned_read = pluss::make_series(1.0).to(pluss::ScalarType::Float64);
  std::cout << "before=" << versioned_read.storage_version() << "\n";
  (void)versioned_read.exp();
  std::cout << "after=" << versioned_read.storage_version() << "\n\n";

  heading("storage_version_inplace_bump");
  auto versioned = pluss::make_series(1.0).to(pluss::ScalarType::Float64);
  std::cout << "before=" << versioned.storage_version() << "\n";
  versioned.relu_();
  std::cout << "after=" << versioned.storage_version() << "\n";

  return 0;
}

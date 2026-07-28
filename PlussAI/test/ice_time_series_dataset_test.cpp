// Correctness coverage for pluss::ai::ice::data::TimeSeriesDataSet: windowing
// (per-group sliding window, gap detection, short-group dropping),
// categorical vocabulary fitting/reuse/out-of-vocabulary handling, and the
// from_series() escape hatch for callers who do their own windowing.
#include <pluss/ai/ice/data/time_series_dataset.h>
#include <pluss/mic.h>
#include <pluss/pluss.h>

#include <cmath>
#include <iostream>
#include <random>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::table::DataFrame;
using pluss::table::Thread;
namespace data = pluss::ai::ice::data;

Config float_cfg() { return Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU); }

// Builds a synthetic long-format panel: `groups` series, `steps_per_group`
// contiguous timesteps each (time_idx 0..steps_per_group-1 per group).
//   region:    static categorical, 2 distinct values ("north"/"south"), alternating by group.
//   base:      static continuous, a per-group constant level.
//   position:  known (encoder+decoder) continuous, t / (steps_per_group-1).
//   noisy_obs: observed (encoder-only) continuous, base + small noise -- only meaningful historically.
//   y:         target, base + 0.5*position + small noise.
DataFrame make_panel(std::int64_t groups, std::int64_t steps_per_group, std::uint64_t seed) {
  std::mt19937_64 rng(seed);
  std::normal_distribution<double> noise(0.0, 0.01);

  std::vector<std::string> group_col{};
  std::vector<std::string> region_col{};
  std::vector<float> time_col{};
  std::vector<float> base_col{};
  std::vector<float> position_col{};
  std::vector<float> noisy_obs_col{};
  std::vector<float> y_col{};

  for (std::int64_t g = 0; g < groups; ++g) {
    const double base = static_cast<double>(g) * 2.0 - 1.0; // -1, 1, 3, ...
    const std::string region = (g % 2 == 0) ? "north" : "south";
    for (std::int64_t t = 0; t < steps_per_group; ++t) {
      const double position = static_cast<double>(t) / static_cast<double>(steps_per_group - 1);
      group_col.push_back("group" + std::to_string(g));
      region_col.push_back(region);
      time_col.push_back(static_cast<float>(t));
      base_col.push_back(static_cast<float>(base));
      position_col.push_back(static_cast<float>(position));
      noisy_obs_col.push_back(static_cast<float>(base + noise(rng)));
      y_col.push_back(static_cast<float>(base + 0.5 * position + noise(rng)));
    }
  }

  return DataFrame({
      {"group", Thread(pluss::make_note(group_col))},
      {"region", Thread(pluss::make_note(region_col))},
      {"t", Thread(pluss::make_series(time_col, {static_cast<std::int64_t>(time_col.size())}, float_cfg()))},
      {"base", Thread(pluss::make_series(base_col, {static_cast<std::int64_t>(base_col.size())}, float_cfg()))},
      {"position",
       Thread(pluss::make_series(position_col, {static_cast<std::int64_t>(position_col.size())}, float_cfg()))},
      {"noisy_obs",
       Thread(pluss::make_series(noisy_obs_col, {static_cast<std::int64_t>(noisy_obs_col.size())}, float_cfg()))},
      {"y", Thread(pluss::make_series(y_col, {static_cast<std::int64_t>(y_col.size())}, float_cfg()))},
  });
}

data::TimeSeriesDataSetOptions make_options(std::int64_t encoder_length, std::int64_t decoder_length) {
  data::TimeSeriesDataSetOptions options{};
  options.time_idx = "t";
  options.group_ids = {"group"};
  options.target = "y";
  options.variables = {
      {"region", data::VariableKind::StaticCategorical},
      {"base", data::VariableKind::StaticContinuous},
      {"position", data::VariableKind::KnownContinuous},
      {"noisy_obs", data::VariableKind::ObservedContinuous},
  };
  options.encoder_length = encoder_length;
  options.decoder_length = decoder_length;
  options.cfg = float_cfg();
  return options;
}

bool run_basic_windowing_checks(std::ostream* err = nullptr) {
  constexpr std::int64_t kGroups = 2;
  constexpr std::int64_t kStepsPerGroup = 20;
  constexpr std::int64_t kEncoderLength = 10;
  constexpr std::int64_t kDecoderLength = 5;

  const auto df = make_panel(kGroups, kStepsPerGroup, /*seed=*/1);
  const auto options = make_options(kEncoderLength, kDecoderLength);
  const auto dataset = data::TimeSeriesDataSet::from_dataframe(df, options);

  const std::int64_t windows_per_group = kStepsPerGroup - (kEncoderLength + kDecoderLength) + 1; // 6
  const std::int64_t expected_size = windows_per_group * kGroups; // 12
  if (dataset.size() != expected_size) {
    if (err) *err << "TimeSeriesDataSet size mismatch: expected " << expected_size << " got " << dataset.size() << "\n";
    return false;
  }

  const auto& batch = dataset.batch();
  if (batch.static_size != 2 || batch.encoder_size != 2 || batch.decoder_size != 1) {
    if (err) {
      *err << "TimeSeriesDataSet variable counts wrong: static=" << batch.static_size
           << " encoder=" << batch.encoder_size << " decoder=" << batch.decoder_size << " (expected 2,2,1)\n";
    }
    return false;
  }
  if (batch.static_categoricals.size() != 1 || batch.static_categoricals[0].variable_index != 0 ||
      batch.static_categoricals[0].cardinality != 3) { // "north","south" + reserved unknown bucket.
    if (err) *err << "TimeSeriesDataSet static_categoricals wrong (expected one slot 0, cardinality 3)\n";
    return false;
  }
  if (!batch.encoder_categoricals.empty() || !batch.decoder_categoricals.empty()) {
    if (err) *err << "TimeSeriesDataSet should have no encoder/decoder categoricals in this panel\n";
    return false;
  }

  const auto sizes = batch.encoder_inputs.sizes();
  if (sizes.size() != 3 || sizes[0] != expected_size || sizes[1] != kEncoderLength || sizes[2] != 2) {
    if (err) *err << "TimeSeriesDataSet encoder_inputs shape wrong\n";
    return false;
  }
  if (batch.decoder_inputs.sizes()[1] != kDecoderLength || batch.decoder_inputs.sizes()[2] != 1) {
    if (err) *err << "TimeSeriesDataSet decoder_inputs shape wrong\n";
    return false;
  }
  if (batch.target.sizes()[0] != expected_size || batch.target.sizes()[1] != kDecoderLength) {
    if (err) *err << "TimeSeriesDataSet target shape wrong\n";
    return false;
  }

  // Value check: static "base" (slot 1, since region is slot 0) for every
  // window of group 0 must equal that group's base level (-1.0) exactly --
  // it's a per-group constant, unaffected by which window within the group
  // was sampled.
  const auto static_flat = batch.static_inputs.to(pluss::pCPU, ScalarType::Float64).reshape({batch.static_inputs.numel()}).to_vec<double>();
  bool group0_base_ok = true;
  for (std::int64_t w = 0; w < windows_per_group; ++w) {
    const double base_value = static_flat[static_cast<std::size_t>(w * 2 + 1)]; // slot 1 == "base"
    if (std::abs(base_value - (-1.0)) > 1e-4) group0_base_ok = false;
  }
  if (!group0_base_ok) {
    if (err) *err << "TimeSeriesDataSet static 'base' values wrong for group 0's windows\n";
    return false;
  }

  // Value check: window 0 is the first window of the first group after
  // sorting ("group0" < "group1" lexicographically), spanning absolute
  // rows [0, encoder_length+decoder_length) -- encoder rows t=0..9,
  // decoder rows t=10..14. Checks the encoder/decoder *content*, not just
  // shapes/counts, since an off-by-one in the gather offsets (the part
  // most likely to have a subtle bug) wouldn't necessarily change any
  // shape or window count.
  const auto encoder_position =
      batch.encoder_inputs.narrow_copy(2, 0, 1).to(pluss::pCPU, ScalarType::Float64).reshape({expected_size, kEncoderLength}).to_vec<double>();
  const auto decoder_position =
      batch.decoder_inputs.narrow_copy(2, 0, 1).to(pluss::pCPU, ScalarType::Float64).reshape({expected_size, kDecoderLength}).to_vec<double>();
  const auto target_flat = batch.target.to(pluss::pCPU, ScalarType::Float64).reshape({expected_size, kDecoderLength}).to_vec<double>();

  for (std::int64_t s = 0; s < kEncoderLength; ++s) {
    const double expected = static_cast<double>(s) / 19.0;
    const double actual = encoder_position[static_cast<std::size_t>(s)]; // window 0, step s
    if (std::abs(actual - expected) > 1e-4) {
      if (err) *err << "TimeSeriesDataSet encoder 'position' content wrong at step " << s << ": expected " << expected
                    << " got " << actual << "\n";
      return false;
    }
  }
  for (std::int64_t s = 0; s < kDecoderLength; ++s) {
    const double t = static_cast<double>(kEncoderLength + s);
    const double expected_position = t / 19.0;
    const double actual_position = decoder_position[static_cast<std::size_t>(s)];
    if (std::abs(actual_position - expected_position) > 1e-4) {
      if (err) *err << "TimeSeriesDataSet decoder 'position' content wrong at step " << s << ": expected "
                    << expected_position << " got " << actual_position << "\n";
      return false;
    }
    const double expected_target = -1.0 + 0.5 * expected_position; // base(group0) + 0.5*position (+- small noise)
    const double actual_target = target_flat[static_cast<std::size_t>(s)];
    if (std::abs(actual_target - expected_target) > 0.1) {
      if (err) *err << "TimeSeriesDataSet target content wrong at step " << s << ": expected ~" << expected_target
                    << " got " << actual_target << "\n";
      return false;
    }
  }

  return true;
}

bool run_gap_and_short_group_checks(std::ostream* err = nullptr) {
  // Group "gapped": t=0..9 then a gap, then t=15..19 (5 more rows) -- 10 +
  // 5 = 15 rows total, but split by a gap into runs of length 10 and 5.
  // Group "short": only 8 rows total, shorter than the 12-step window.
  // Group "clean": 20 contiguous rows, same as the basic test.
  constexpr std::int64_t kEncoderLength = 8;
  constexpr std::int64_t kDecoderLength = 4;
  constexpr std::int64_t kWindowLength = kEncoderLength + kDecoderLength; // 12

  std::vector<std::string> group_col{};
  std::vector<std::string> region_col{};
  std::vector<float> time_col{};
  std::vector<float> base_col{};
  std::vector<float> position_col{};
  std::vector<float> noisy_obs_col{};
  std::vector<float> y_col{};

  const auto push_row = [&](const std::string& group, std::int64_t t) {
    group_col.push_back(group);
    region_col.push_back("north");
    time_col.push_back(static_cast<float>(t));
    base_col.push_back(0.0F);
    position_col.push_back(static_cast<float>(t) / 19.0F);
    noisy_obs_col.push_back(0.0F);
    y_col.push_back(0.0F);
  };

  for (std::int64_t t = 0; t < 10; ++t) push_row("gapped", t);
  for (std::int64_t t = 15; t < 20; ++t) push_row("gapped", t);
  for (std::int64_t t = 0; t < 8; ++t) push_row("short", t);
  for (std::int64_t t = 0; t < 20; ++t) push_row("clean", t);

  const DataFrame df({
      {"group", Thread(pluss::make_note(group_col))},
      {"region", Thread(pluss::make_note(region_col))},
      {"t", Thread(pluss::make_series(time_col, {static_cast<std::int64_t>(time_col.size())}, float_cfg()))},
      {"base", Thread(pluss::make_series(base_col, {static_cast<std::int64_t>(base_col.size())}, float_cfg()))},
      {"position",
       Thread(pluss::make_series(position_col, {static_cast<std::int64_t>(position_col.size())}, float_cfg()))},
      {"noisy_obs",
       Thread(pluss::make_series(noisy_obs_col, {static_cast<std::int64_t>(noisy_obs_col.size())}, float_cfg()))},
      {"y", Thread(pluss::make_series(y_col, {static_cast<std::int64_t>(y_col.size())}, float_cfg()))},
  });

  const auto options = make_options(kEncoderLength, kDecoderLength);
  const auto dataset = data::TimeSeriesDataSet::from_dataframe(df, options);

  // "gapped": run of 10 (< 12, 0 windows) + run of 5 (< 12, 0 windows) = 0.
  // "short": 8 rows (< 12) = 0 windows.
  // "clean": 20 rows, window_length 12 -> 20-12+1 = 9 windows.
  constexpr std::int64_t kExpected = 9;
  if (dataset.size() != kExpected) {
    if (err) {
      *err << "TimeSeriesDataSet gap/short-group handling wrong: expected " << kExpected << " windows "
              "(only from 'clean'), got " << dataset.size()
           << " -- a gap or a too-short group produced a spurious window\n";
    }
    return false;
  }
  return true;
}

bool run_categorical_vocabulary_checks(std::ostream* err = nullptr) {
  const auto train_df = make_panel(2, 20, /*seed=*/2); // regions: "north" (group0), "south" (group1)
  const auto options = make_options(10, 5);
  const auto train_dataset = data::TimeSeriesDataSet::from_dataframe(train_df, options);

  if (train_dataset.vocabularies().empty()) {
    if (err) *err << "TimeSeriesDataSet should report a fitted vocabulary for the categorical 'region' variable\n";
    return false;
  }
  const auto& vocab = train_dataset.vocabularies().front();
  if (vocab.variable_name != "region" || vocab.categories.size() != 3) {
    if (err) *err << "TimeSeriesDataSet 'region' vocabulary should have 2 categories + 1 unknown bucket\n";
    return false;
  }

  // A held-out dataframe with a THIRD, never-seen region ("east") for one
  // group, reusing the training vocabulary.
  std::vector<std::string> group_col{};
  std::vector<std::string> region_col{};
  std::vector<float> time_col{};
  std::vector<float> base_col{};
  std::vector<float> position_col{};
  std::vector<float> noisy_obs_col{};
  std::vector<float> y_col{};
  for (std::int64_t t = 0; t < 20; ++t) {
    group_col.push_back("held_out");
    region_col.push_back("east"); // unseen during training.
    time_col.push_back(static_cast<float>(t));
    base_col.push_back(5.0F);
    position_col.push_back(static_cast<float>(t) / 19.0F);
    noisy_obs_col.push_back(5.0F);
    y_col.push_back(5.0F);
  }
  const DataFrame held_out_df({
      {"group", Thread(pluss::make_note(group_col))},
      {"region", Thread(pluss::make_note(region_col))},
      {"t", Thread(pluss::make_series(time_col, {static_cast<std::int64_t>(time_col.size())}, float_cfg()))},
      {"base", Thread(pluss::make_series(base_col, {static_cast<std::int64_t>(base_col.size())}, float_cfg()))},
      {"position",
       Thread(pluss::make_series(position_col, {static_cast<std::int64_t>(position_col.size())}, float_cfg()))},
      {"noisy_obs",
       Thread(pluss::make_series(noisy_obs_col, {static_cast<std::int64_t>(noisy_obs_col.size())}, float_cfg()))},
      {"y", Thread(pluss::make_series(y_col, {static_cast<std::int64_t>(y_col.size())}, float_cfg()))},
  });

  const auto held_out_dataset = data::TimeSeriesDataSet::from_dataframe(held_out_df, options, train_dataset);
  const auto static_flat = held_out_dataset.batch()
                                .static_inputs.to(pluss::pCPU, ScalarType::Float64)
                                .reshape({held_out_dataset.batch().static_inputs.numel()})
                                .to_vec<double>();
  const double unknown_code = static_cast<double>(vocab.categories.size() - 1);
  bool all_unknown = true;
  for (std::size_t w = 0; w < static_cast<std::size_t>(held_out_dataset.size()); ++w) {
    if (std::abs(static_flat[w * 2] - unknown_code) > 1e-9) all_unknown = false; // slot 0 == "region"
  }
  if (!all_unknown) {
    if (err) *err << "TimeSeriesDataSet should map an unseen category to the reserved 'unknown' code\n";
    return false;
  }

  return true;
}

bool run_from_series_checks(std::ostream* err = nullptr) {
  const auto cfg = float_cfg();
  data::TimeSeriesBatch batch{};
  batch.static_inputs = pluss::randn({4, 3}, cfg, 1ULL);
  batch.encoder_inputs = pluss::randn({4, 6, 2}, cfg, 2ULL);
  batch.decoder_inputs = pluss::randn({4, 3, 2}, cfg, 3ULL);
  batch.encoder_target = pluss::randn({4, 6}, cfg, 4ULL);
  batch.target = pluss::randn({4, 3}, cfg, 4ULL);
  // static_size/encoder_size/decoder_size deliberately left at 0 -- from_series() should derive them.

  const auto dataset = data::TimeSeriesDataSet::from_series(batch);
  if (dataset.size() != 4) {
    if (err) *err << "TimeSeriesDataSet::from_series size() should read N off encoder_inputs\n";
    return false;
  }
  if (dataset.batch().static_size != 3 || dataset.batch().encoder_size != 2 || dataset.batch().decoder_size != 2) {
    if (err) *err << "TimeSeriesDataSet::from_series should auto-derive static/encoder/decoder_size from tensor shapes\n";
    return false;
  }
  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("ice time series dataset tranche", "[ice][data]") {
  if (!run_basic_windowing_checks(&std::cerr)) FAIL("basic windowing checks failed");
  if (!run_gap_and_short_group_checks(&std::cerr)) FAIL("gap/short-group checks failed");
  if (!run_categorical_vocabulary_checks(&std::cerr)) FAIL("categorical vocabulary checks failed");
  if (!run_from_series_checks(&std::cerr)) FAIL("from_series checks failed");
}
#else
int main() {
  bool ok = true;
  ok = run_basic_windowing_checks(&std::cerr) && ok;
  ok = run_gap_and_short_group_checks(&std::cerr) && ok;
  ok = run_categorical_vocabulary_checks(&std::cerr) && ok;
  ok = run_from_series_checks(&std::cerr) && ok;
  if (!ok) return 1;
  std::cout << "ice time series dataset checks passed\n";
  return 0;
}
#endif

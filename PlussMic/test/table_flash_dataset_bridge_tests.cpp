#include <cmath>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "pluss/pluss.h"

namespace {

using pluss::table::DataFrame;
using pluss::table::Thread;

[[noreturn]] void fail(std::string_view message) {
  throw std::runtime_error(std::string(message));
}

void expect(bool condition, std::string_view message) {
  if (!condition) {
    fail(message);
  }
}

pluss::Series bool_mask(std::initializer_list<bool> flags) {
  auto out = pluss::Series::zeros({static_cast<std::int64_t>(flags.size())}, pluss::ScalarType::Bool);
  auto* ptr = reinterpret_cast<bool*>(out.raw_data());
  std::size_t i = 0;
  for (const bool flag : flags) {
    ptr[i++] = flag;
  }
  return out;
}

std::int64_t scalar_tick(std::string_view iso_date) {
  std::string normalized(iso_date);
  if (normalized.size() == 10 && normalized[4] == '-' && normalized[7] == '-') {
    normalized += "T00:00:00Z";
  }
  return pluss::make_timeline(std::vector<std::string>{normalized})
      .epoch_nanos()
      .to(pluss::Platform::CPU)
      .to_vec<std::int64_t>()[0];
}

pluss::Timeline timeline_with_nat(std::initializer_list<std::string_view> values,
                                  std::initializer_list<bool> valid) {
  std::vector<std::int64_t> ticks;
  ticks.reserve(values.size());
  auto vit = valid.begin();
  for (const auto value : values) {
    const bool keep = vit != valid.end() ? *vit++ : true;
    ticks.push_back(keep ? scalar_tick(value) : pluss::Timeline::kNaT);
  }
  const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(ticks.size())};
  return pluss::codeh::ops_internal::timeline_from_epoch_nanos(
      ticks, shape);
}

void test_from_frame_preserves_order_kinds_and_masks() {
  const auto mask = bool_mask({true, false, true});
  const auto frame = DataFrame({
      {"student", Thread(pluss::make_note({"Ada", "", "Cy"}), mask)},
      {"observed_at", Thread(timeline_with_nat({"2026-01-01", "2026-01-02", "2026-01-03"},
                                               {true, false, true}),
                             mask)},
      {"score", Thread(pluss::make_series(std::vector<double>{91.5, 0.0, 88.0}), mask)},
  });

  const auto data = pluss::pl::flash::Dataset::from_frame(frame);
  expect(data.feature_order.has_value(), "flash::Dataset::from_frame should populate feature_order");
  expect(data.feature_kinds.has_value(), "flash::Dataset::from_frame should populate feature_kinds");
  expect(data.feature_order->at(0) == "student", "feature order should preserve the DataFrame order");
  expect(data.feature_order->at(1) == "observed_at", "feature order should preserve the DataFrame order");
  expect(data.feature_order->at(2) == "score", "feature order should preserve the DataFrame order");
  expect(data.feature_kinds->at(0) == "text", "text columns should be tagged as text");
  expect(data.feature_kinds->at(1) == "temporal", "timeline columns should be tagged as temporal");
  expect(data.feature_kinds->at(2) == "numeric", "series columns should be tagged as numeric");
  expect(data.text_validity_mask("student").has_value(), "text validity should be preserved");
  expect(data.temporal_validity_mask("observed_at").has_value(), "temporal validity should be preserved");
  expect(data.numeric_validity_mask("score").has_value(), "numeric validity should be preserved");
}

void test_round_trip_rebuilds_frame() {
  const auto frame = DataFrame({
      {"student", pluss::make_note({"Ada", "Bo", "Cy"})},
      {"observed_at", timeline_with_nat({"2026-01-01", "2026-01-02", "2026-01-03"},
                                         {true, true, true})},
      {"score", pluss::make_series(std::vector<double>{91.5, 84.0, 88.0})},
  }).title("grades");

  const auto data = pluss::pl::flash::Dataset::from_frame(frame);
  const auto rebuilt = data.frame({.title = "rebuilt"});

  expect(rebuilt.features() == std::vector<std::string>({"student", "observed_at", "score"}),
         "flash::Dataset::frame should use dataset feature_order when present");
  expect(rebuilt.ncases() == 3, "flash::Dataset::frame should preserve case count for rectangular datasets");
  expect(rebuilt.features("student").as_note().at(1) == "Bo",
         "flash::Dataset::frame should preserve text payloads");
  expect(rebuilt.features("observed_at").as_timeline().to_note_iso().at(2).substr(0, 10) == "2026-01-03",
         "flash::Dataset::frame should preserve temporal payloads");
  expect(std::fabs(rebuilt.features("score").as_series().to_vec<double>()[0] - 91.5) < 1e-12,
         "flash::Dataset::frame should preserve numeric payloads");
  expect(rebuilt.title() == "rebuilt", "flash::Dataset::frame should accept an explicit output title");
}

void test_ragged_dataset_requires_policy_or_pads() {
  pluss::pl::flash::Dataset data;
  data.feature_order = pluss::make_note({"time", "value", "label"});
  data.feature_kinds = pluss::make_note({"temporal", "numeric", "text"});
  data.temporal_names = pluss::make_note({"time"});
  data.temporal_values.push_back(
      timeline_with_nat({"2026-01-01", "2026-01-02"}, {true, true}));
  data.temporal_validity.push_back(std::nullopt);
  data.numeric_names = pluss::make_note({"value"});
  data.numeric_values.push_back(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0}));
  data.numeric_validity.push_back(std::nullopt);
  data.text_names = pluss::make_note({"label"});
  data.text_values.push_back(pluss::make_note({"alpha", "beta"}));
  data.text_validity.push_back(std::nullopt);

  bool threw = false;
  try {
    (void)data.frame();
  } catch (...) {
    threw = true;
  }
  expect(threw, "ragged datasets should reject strict frame conversion");

  const auto padded = data.frame(
      {.policy = pluss::pl::flash::FramePolicy::PadNull});
  expect(padded.ncases() == 3, "PadNull should rectangularize to the maximum column length");
  expect(padded.features() == std::vector<std::string>({"time", "value", "label"}),
         "PadNull should still preserve feature order");
  expect(pluss::table::detail::validity_mask(padded.features("label"))->to_vec<bool>() ==
             std::vector<bool>({true, true, false}),
         "PadNull should set false validity on padded note rows");
  expect(pluss::table::detail::validity_mask(padded.features("time"))->to_vec<bool>() ==
             std::vector<bool>({true, true, false}),
         "PadNull should set false validity on padded temporal rows");
}

} // namespace

int main() {
  test_from_frame_preserves_order_kinds_and_masks();
  test_round_trip_rebuilds_frame();
  test_ragged_dataset_requires_policy_or_pads();
  return 0;
}

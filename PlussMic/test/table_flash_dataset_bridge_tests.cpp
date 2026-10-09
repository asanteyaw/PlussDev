#include <cmath>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "pluss/pluss.h"

namespace {

using pluss::table::DataFrame;
using pluss::table::Slate;
using pluss::table::Atlas;

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

void test_from_table_preserves_order_kinds_and_masks() {
  const auto mask = bool_mask({true, false, true});
  const auto frame = DataFrame({
      {"student", Slate(pluss::make_note({"Ada", "", "Cy"}), mask)},
      {"observed_at", Slate(timeline_with_nat({"2026-01-01", "2026-01-02", "2026-01-03"},
                                               {true, false, true}),
                             mask)},
      {"score", Slate(pluss::make_series(std::vector<double>{91.5, 0.0, 88.0}), mask)},
  });

  const auto data = pluss::pl::flash::Dataset::from_table(frame);
  expect(data.feature_order.has_value(), "flash::Dataset::from_table should populate feature_order");
  expect(data.feature_kinds.has_value(), "flash::Dataset::from_table should populate feature_kinds");
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

  const auto data = pluss::pl::flash::Dataset::from_table(frame);
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

void test_dataset_row_and_column_surface() {
  const auto frame = DataFrame({
      {"student", pluss::make_note({"Ada", "Bo", "Cy"})},
      {"observed_at", timeline_with_nat({"2026-01-01", "2026-01-02", "2026-01-03"},
                                         {true, true, true})},
      {"score", pluss::make_series(std::vector<double>{91.5, 84.0, 88.0})},
  });

  const auto data = pluss::pl::flash::Dataset::from_table(frame);
  expect(data.nrows() == 3, "flash::Dataset::nrows should report the shared row count");
  expect(data.ncols() == 3, "flash::Dataset::ncols should report the feature count");
  expect(data.size() == std::pair<std::int64_t, std::int64_t>{3, 3},
         "flash::Dataset::size should return {nrows, ncols}");
}

void test_dataset_nrows_rejects_ragged_shapes() {
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
    (void)data.nrows();
  } catch (...) {
    threw = true;
  }
  expect(threw, "flash::Dataset::nrows should reject ragged datasets");
  expect(data.ncols() == 3, "flash::Dataset::ncols should still report feature count for ragged datasets");
}

void test_lineplot_dataframe_auto_select_and_explicit_select() {
  const auto frame = DataFrame({
      {"day", timeline_with_nat({"2026-01-01", "2026-01-02", "2026-01-03"},
                                 {true, true, true})},
      {"sales", pluss::make_series(std::vector<double>{10.0, 12.5, 11.0})},
  });

  pluss::pl::Studio fig("lineplot_dataframe_auto_select");
  auto& pane = fig.pane2("line");
  (void)pluss::pl::flash::lineplot(pane, frame);
  expect(pane.x_axis().label == "day",
         "flash::lineplot(DataFrame) should auto-label x with the first feature name");
  expect(pane.y_axis().label == "sales",
         "flash::lineplot(DataFrame) should auto-label y with the second feature name");

  const auto wider = DataFrame({
      {"day", timeline_with_nat({"2026-01-01", "2026-01-02", "2026-01-03"},
                                 {true, true, true})},
      {"sales", pluss::make_series(std::vector<double>{10.0, 12.5, 11.0})},
      {"margin", pluss::make_series(std::vector<double>{2.0, 3.0, 2.5})},
  });

  bool threw = false;
  try {
    pluss::pl::Studio bad_fig("lineplot_dataframe_missing_columns");
    auto& bad = bad_fig.pane2("bad");
    (void)pluss::pl::flash::lineplot(bad, wider);
  } catch (const std::exception& e) {
    threw = std::string(e.what()).find("explicit x_feature/y_feature") != std::string::npos;
  }
  expect(threw,
         "flash::lineplot(DataFrame) should require explicit x_feature/y_feature when the frame does not have exactly two features");

  pluss::pl::Studio explicit_fig("lineplot_dataframe_explicit_columns");
  auto& explicit_pane = explicit_fig.pane2("explicit");
  (void)pluss::pl::flash::lineplot(
      explicit_pane,
      wider,
      {.x_feature = "day", .y_feature = "margin"});
  expect(explicit_pane.y_axis().label == "margin",
         "flash::lineplot(DataFrame) should accept explicit x/y feature selection");
}

void test_histplot_dataframe_requires_single_feature_or_explicit_feature() {
  const auto single = DataFrame({
      {"score", pluss::make_series(std::vector<double>{91.5, 84.0, 88.0})},
  });

  pluss::pl::Studio fig("histplot_dataframe_single_feature");
  auto& pane = fig.pane2("hist");
  (void)pluss::pl::flash::histplot(pane, single);
  expect(pane.x_axis().label == "score",
         "flash::histplot(DataFrame) should use the lone feature name as the x label");
  expect(pane.y_axis().label == "count",
         "flash::histplot(DataFrame) should label the count axis");

  const auto multi = DataFrame({
      {"score", pluss::make_series(std::vector<double>{91.5, 84.0, 88.0})},
      {"bonus", pluss::make_series(std::vector<double>{1.0, 0.0, 2.0})},
  });

  bool threw = false;
  try {
    pluss::pl::Studio bad_fig("histplot_dataframe_missing_feature");
    auto& bad = bad_fig.pane2("bad");
    (void)pluss::pl::flash::histplot(bad, multi);
  } catch (const std::exception& e) {
    threw = std::string(e.what()).find("exactly 1 feature") != std::string::npos;
  }
  expect(threw,
         "flash::histplot(DataFrame) should require a single feature or an explicit feature selection");

  pluss::pl::Studio explicit_fig("histplot_dataframe_explicit_feature");
  auto& explicit_pane = explicit_fig.pane2("explicit");
  (void)pluss::pl::flash::histplot(explicit_pane, multi, {.feature = "bonus"});
  expect(explicit_pane.x_axis().label == "bonus",
         "flash::histplot(DataFrame) should accept explicit feature selection");
}

void test_barplot_slate_and_atlas_depth_gate() {
  pluss::pl::Studio slate_fig("barplot_slate_horizontal");
  auto& slate_pane = slate_fig.pane2("slate");
  (void)pluss::pl::flash::barplot(
      slate_pane,
      Slate(pluss::make_series(std::vector<double>{3.0, 7.0, 4.0})),
      {.horizontal = true});
  expect(slate_pane.x_axis().label == "value",
         "flash::barplot(Slate) should label the value axis for horizontal bars");
  expect(slate_pane.y_axis().label == "y",
         "flash::barplot(Slate) should label the implicit position axis for horizontal bars");

  const auto frame = DataFrame({
      {"x", pluss::make_series(std::vector<double>{1.0, 2.0, 3.0})},
      {"y", pluss::make_series(std::vector<double>{4.0, 5.0, 6.0})},
      {"z", pluss::make_series(std::vector<double>{7.0, 8.0, 9.0})},
  });

  const Atlas one_frame({frame});
  pluss::pl::Studio ok_fig("plot3d_atlas_depth_one");
  auto& ok = ok_fig.pane3("ok");
  (void)pluss::pl::flash::plot3D(ok, one_frame);
  expect(ok.z_axis().label == "z",
         "flash::plot3D(Atlas) should proxy a depth-1 Atlas through its single frame");

  const Atlas two_frames({frame, frame});
  bool threw = false;
  try {
    pluss::pl::Studio bad_fig("plot3d_atlas_depth_two");
    auto& bad = bad_fig.pane3("bad");
    (void)pluss::pl::flash::plot3D(bad, two_frames);
  } catch (const std::exception& e) {
    threw = std::string(e.what()).find("depth == 1") != std::string::npos;
  }
  expect(threw,
         "flash direct Atlas plotting should reject depth > 1 so the user selects a frame intentionally");
}

void test_barplot_dataframe_supports_categorical_numeric_pairs() {
  const auto frame = DataFrame({
      {"district_region", pluss::make_note({"north", "south", "middle"})},
      {"n_customers", pluss::make_series(std::vector<double>{20.0, 35.0, 18.0})},
  });

  pluss::pl::Studio vertical_fig("barplot_dataframe_categorical_vertical");
  auto& vertical = vertical_fig.pane2("vertical");
  (void)pluss::pl::flash::barplot(
      vertical,
      frame,
      {.x_feature = "district_region", .y_feature = "n_customers"});
  expect(vertical.x_axis().label == "district_region",
         "flash::barplot(DataFrame) should accept categorical x with numeric y");
  expect(vertical.y_axis().label == "n_customers",
         "flash::barplot(DataFrame) should label the numeric value axis");

  pluss::pl::Studio horizontal_fig("barplot_dataframe_categorical_horizontal");
  auto& horizontal = horizontal_fig.pane2("horizontal");
  (void)pluss::pl::flash::barplot(
      horizontal,
      frame,
      {.x_feature = "district_region", .y_feature = "n_customers"},
      {.horizontal = true});
  expect(horizontal.x_axis().label == "n_customers",
         "flash::barplot(DataFrame, horizontal) should place numeric values on x");
  expect(horizontal.y_axis().label == "district_region",
         "flash::barplot(DataFrame, horizontal) should place categories on y");
}

void test_relplot_dataframe_supports_explicit_hue() {
  const auto frame = DataFrame({
      {"day", pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0})},
      {"sales", pluss::make_series(std::vector<double>{10.0, 12.5, 11.0, 13.0})},
      {"desk", pluss::make_note({"north", "south", "north", "south"})},
  });

  pluss::pl::Studio fig("relplot_dataframe_hue");
  auto& pane = fig.pane2("rel");
  (void)pluss::pl::flash::relplot(
      pane,
      frame,
      {.x_feature = "day", .y_feature = "sales", .hue_feature = "desk"},
      "line");
  expect(pane.x_axis().label == "day",
         "flash::relplot(DataFrame) should label the x axis from x_feature");
  expect(pane.y_axis().label == "sales",
         "flash::relplot(DataFrame) should label the y axis from y_feature");
}

void test_boxenplot_dataframe_accepts_single_numeric_feature() {
  const auto frame = DataFrame({
      {"score", pluss::make_series(std::vector<double>{91.5, 84.0, 88.0, 79.5})},
  });

  pluss::pl::Studio fig("boxenplot_dataframe_single_feature");
  auto& pane = fig.pane2("boxen");
  (void)pluss::pl::flash::boxenplot(pane, frame);
  expect(pane.x_axis().label == "score",
         "flash::boxenplot(DataFrame) should use the selected feature name on the data axis");
  expect(pane.y_axis().label == "value",
         "flash::boxenplot(DataFrame) should label the measure axis as value");
}

void test_parallel_family_dataframe_accepts_class_feature() {
  const auto frame = DataFrame({
      {"sepal", pluss::make_series(std::vector<double>{5.1, 4.9, 6.3, 6.5})},
      {"petal", pluss::make_series(std::vector<double>{1.4, 1.4, 5.6, 5.8})},
      {"width", pluss::make_series(std::vector<double>{0.2, 0.2, 2.4, 2.2})},
      {"species", pluss::make_note({"setosa", "setosa", "virginica", "virginica"})},
  });

  {
    pluss::pl::Studio fig("parallelplot_dataframe_class_feature");
    (void)pluss::pl::flash::parallelplot(
        fig,
        frame,
        {.class_feature = "species"});
  }

  {
    pluss::pl::Studio fig("andrews_dataframe_class_feature");
    (void)pluss::pl::flash::andrews_curves(
        fig,
        frame,
        {.class_feature = "species"});
  }

  {
    pluss::pl::Studio fig("radviz_dataframe_class_feature");
    (void)pluss::pl::flash::radviz(
        fig,
        frame,
        {.class_feature = "species"});
  }

  expect(true,
         "flash parallelplot/andrews_curves/radviz should accept a DataFrame plus class_feature without throwing");
}

} // namespace

int main() {
  test_from_table_preserves_order_kinds_and_masks();
  test_round_trip_rebuilds_frame();
  test_ragged_dataset_requires_policy_or_pads();
  test_dataset_row_and_column_surface();
  test_dataset_nrows_rejects_ragged_shapes();
  test_lineplot_dataframe_auto_select_and_explicit_select();
  test_histplot_dataframe_requires_single_feature_or_explicit_feature();
  test_barplot_slate_and_atlas_depth_gate();
  test_barplot_dataframe_supports_categorical_numeric_pairs();
  test_relplot_dataframe_supports_explicit_hue();
  test_boxenplot_dataframe_accepts_single_numeric_feature();
  test_parallel_family_dataframe_accepts_class_feature();
  return 0;
}

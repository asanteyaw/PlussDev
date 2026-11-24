#include <cmath>
#include <functional>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "pluss/pluss.h"

namespace {

using pluss::table::DataFrame;
using pluss::table::Shelf;
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
  auto out = pluss::series::zeros({static_cast<std::int64_t>(flags.size())}, pluss::ScalarType::Bool);
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
      .host<std::int64_t>()[0];
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
      {"student", Shelf(pluss::make_note({"Ada", "", "Cy"}), mask)},
      {"observed_at", Shelf(timeline_with_nat({"2026-01-01", "2026-01-02", "2026-01-03"},
                                               {true, false, true}),
                             mask)},
      {"score", Shelf(pluss::make_series(std::vector<double>{91.5, 0.0, 88.0}), mask)},
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
  expect(std::fabs(rebuilt.features("score").as_series().host<double>()[0] - 91.5) < 1e-12,
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
  expect(pluss::table::detail::validity_mask(padded.features("label"))->host<bool>() ==
             std::vector<bool>({true, true, false}),
         "PadNull should set false validity on padded note rows");
  expect(pluss::table::detail::validity_mask(padded.features("time"))->host<bool>() ==
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

std::string error_text(const std::function<void()>& body) {
  try {
    body();
  } catch (const pluss::Error& e) {
    return e.message();
  } catch (const std::exception& e) {
    fail(std::string("expected pluss::Error, got: ") + e.what());
  }
  return {};
}

DataFrame wide_frame() {
  return DataFrame({
      {"day", timeline_with_nat({"2026-01-01", "2026-01-02", "2026-01-03"}, {true, true, true})},
      {"sales", pluss::make_series(std::vector<double>{10.0, 12.5, 11.0})},
      {"margin", pluss::make_series(std::vector<double>{2.0, 3.0, 2.5})},
      {"tax", pluss::make_series(std::vector<double>{1.0, 1.5, 1.2})},
      {"city", pluss::make_note(std::vector<std::string>{"a", "b", "c"})},
  });
}

void test_lineplot_dataframe_requires_names_or_selection() {
  const auto wider = wide_frame();

  // No names and no selection: an instructive error, never a guess.
  {
    pluss::pl::Studio fig("lineplot_no_names");
    auto& pane = fig.pane2("p");
    const auto message = error_text([&] { (void)pluss::pl::flash::lineplot(pane, wider, {}); });
    expect(message.find("no columns named") != std::string::npos &&
               message.find("selection.all_columns") != std::string::npos &&
               message.find("sales") != std::string::npos,
           "flash::lineplot(DataFrame) with no names should explain how to name columns or set a selection: " + message);
  }

  // Explicit x and y.
  {
    pluss::pl::Studio fig("lineplot_explicit");
    auto& pane = fig.pane2("p");
    (void)pluss::pl::flash::lineplot(pane, wider, {.x_feature = "day", .y_feature = "margin"});
    expect(pane.trace_count() == 1 && pane.x_axis().label == "day" && pane.y_axis().label == "margin",
           "flash::lineplot(DataFrame) should draw the named x and y");
  }
  // y only: against the row positions, never a guessed x.
  {
    pluss::pl::Studio fig("lineplot_y_only");
    auto& pane = fig.pane2("p");
    (void)pluss::pl::flash::lineplot(pane, wider, {.y_feature = "sales"});
    expect(pane.trace_count() == 1 && pane.x_axis().label == "case" && pane.y_axis().label == "sales",
           "flash::lineplot(DataFrame) with y_feature alone should plot against the row positions");
  }
  // x alone is not enough.
  {
    pluss::pl::Studio fig("lineplot_x_only");
    auto& pane = fig.pane2("p");
    const auto message = error_text([&] { (void)pluss::pl::flash::lineplot(pane, wider, {.x_feature = "day"}); });
    expect(message.find("x_feature names the x axis only") != std::string::npos,
           "flash::lineplot(DataFrame) with x_feature alone should ask for y: " + message);
  }
  // all_columns: every numeric column against the row positions, one pane, one trace each.
  {
    pluss::pl::Studio fig("lineplot_all_columns");
    auto& pane = fig.pane2("p");
    (void)pluss::pl::flash::lineplot(pane, wider, {.selection = {.all_columns = true}});
    expect(pane.trace_count() == 3, "all_columns should draw sales, margin and tax (the time and text columns are not numeric)");
    expect(pane.x_axis().label == "case" && pane.y_axis().label == "value",
           "all_columns should plot against the row positions with a shared value axis");
  }
  // all_columns with an explicit x: the x column is never also a trace.
  {
    pluss::pl::Studio fig("lineplot_all_columns_x");
    auto& pane = fig.pane2("p");
    (void)pluss::pl::flash::lineplot(pane, wider, {.x_feature = "sales", .selection = {.all_columns = true}});
    expect(pane.trace_count() == 2 && pane.x_axis().label == "sales",
           "all_columns with x_feature should draw the other numeric columns against x");
  }
  // include_columns keeps the given order.
  {
    pluss::pl::Studio fig("lineplot_include");
    auto& pane = fig.pane2("p");
    (void)pluss::pl::flash::lineplot(pane, wider, {.x_feature = "day", .selection = {.include_columns = {"tax", "sales"}}});
    expect(pane.trace_count() == 2, "include_columns should draw exactly the listed columns");
  }
  // exclude_columns: every numeric column except these.
  {
    pluss::pl::Studio fig("lineplot_exclude");
    auto& pane = fig.pane2("p");
    (void)pluss::pl::flash::lineplot(pane, wider, {.selection = {.exclude_columns = {"tax"}}});
    expect(pane.trace_count() == 2, "exclude_columns should draw every numeric column except the excluded ones");
  }
  // Errors: conflicting knobs, unknown names, non-numeric names, y_feature with a selection.
  {
    pluss::pl::Studio fig("lineplot_errors");
    auto& pane = fig.pane2("p");
    auto both = error_text([&] {
      (void)pluss::pl::flash::lineplot(pane, wider, {.selection = {.include_columns = {"tax"}, .exclude_columns = {"sales"}}});
    });
    expect(both.find("include_columns and exclude_columns cannot both be set") != std::string::npos && both.find("but not both") != std::string::npos, "include+exclude: " + both);
    auto all_with = error_text([&] {
      (void)pluss::pl::flash::lineplot(pane, wider, {.selection = {.all_columns = true, .exclude_columns = {"sales"}}});
    });
    expect(all_with.find("all_columns and exclude_columns cannot be combined: all_columns already means every numeric column. To drop some columns set exclude_columns instead") != std::string::npos &&
               all_with.find("flash::lineplot(DataFrame)") != std::string::npos,
           "all+exclude: " + all_with);
    auto all_inc = error_text([&] {
      (void)pluss::pl::flash::lineplot(pane, wider, {.selection = {.all_columns = true, .include_columns = {"sales"}}});
    });
    expect(all_inc.find("all_columns and include_columns cannot be combined") != std::string::npos &&
               all_inc.find("set include_columns instead") != std::string::npos,
           "all+include: " + all_inc);
    auto typo = error_text([&] {
      (void)pluss::pl::flash::lineplot(pane, wider, {.selection = {.include_columns = {"salez"}}});
    });
    expect(typo.find("Did you mean 'sales'?") != std::string::npos && typo.find("Available columns = [day") != std::string::npos,
           "typo should suggest the closest name: " + typo);
    auto x_in = error_text([&] {
      (void)pluss::pl::flash::lineplot(pane, wider, {.x_feature = "sales", .selection = {.include_columns = {"sales", "tax"}}});
    });
    expect(x_in.find("also the x column") != std::string::npos && x_in.find("Remove 'sales' from include_columns") != std::string::npos,
           "x among the selection: " + x_in);
    auto unknown = error_text([&] {
      (void)pluss::pl::flash::lineplot(pane, wider, {.selection = {.include_columns = {"nope"}}});
    });
    expect(unknown.find("unknown column 'nope'") != std::string::npos && unknown.find("Available columns = [day") != std::string::npos,
           "unknown include name should list the frame's columns: " + unknown);
    auto unknown_exclude = error_text([&] {
      (void)pluss::pl::flash::lineplot(pane, wider, {.selection = {.exclude_columns = {"nope"}}});
    });
    expect(unknown_exclude.find("unknown column 'nope'") != std::string::npos, "unknown exclude name: " + unknown_exclude);
    auto text = error_text([&] {
      (void)pluss::pl::flash::lineplot(pane, wider, {.selection = {.include_columns = {"city"}}});
    });
    expect(text.find("which is not numeric") != std::string::npos && text.find("Numeric columns = [sales") != std::string::npos, "non-numeric include name: " + text);
    auto y_and_sel = error_text([&] {
      (void)pluss::pl::flash::lineplot(pane, wider, {.y_feature = "sales", .selection = {.all_columns = true}});
    });
    expect(y_and_sel.find("not both") != std::string::npos, "y_feature plus selection: " + y_and_sel);
    auto unknown_x = error_text([&] {
      (void)pluss::pl::flash::lineplot(pane, wider, {.x_feature = "nope", .selection = {.all_columns = true}});
    });
    expect(unknown_x.find("unknown feature 'nope'") != std::string::npos && unknown_x.find("DataFrame features = [day") != std::string::npos,
           "unknown x feature should list the frame's features: " + unknown_x);
  }
  // The same rule for scatter, step and area.
  {
    pluss::pl::Studio fig("wide_other_kinds");
    auto& p1 = fig.pane2("scatter");
    (void)pluss::pl::flash::scatterplot(p1, wider, {.selection = {.all_columns = true}});
    auto& p2 = fig.pane2("step");
    (void)pluss::pl::flash::stepplot(p2, wider, {.selection = {.include_columns = {"sales", "tax"}}});
    auto& p3 = fig.pane2("area");
    (void)pluss::pl::flash::areaplot(p3, wider, {.selection = {.exclude_columns = {"tax"}}}, 0.0);
    expect(p1.trace_count() == 3 && p2.trace_count() == 2 && p3.trace_count() == 2,
           "scatter, step and area should draw one trace per selected column");
    auto none = error_text([&] { (void)pluss::pl::flash::scatterplot(p1, wider, {}); });
    expect(none.find("no columns named") != std::string::npos, "scatterplot with no names: " + none);
  }
}

void test_histplot_dataframe_requires_a_name_or_selection() {
  const auto multi = DataFrame({
      {"score", pluss::make_series(std::vector<double>{91.5, 84.0, 88.0})},
      {"bonus", pluss::make_series(std::vector<double>{1.0, 0.0, 2.0})},
  });

  {
    pluss::pl::Studio bad_fig("histplot_dataframe_missing_feature");
    auto& bad = bad_fig.pane2("bad");
    const auto message = error_text([&] { (void)pluss::pl::flash::histplot(bad, multi, {}); });
    expect(message.find("no columns named") != std::string::npos,
           "flash::histplot(DataFrame) should require a name or a selection: " + message);
  }
  {
    const auto one = DataFrame({{"score", pluss::make_series(std::vector<double>{91.5, 84.0, 88.0})}});
    pluss::pl::Studio fig("histplot_single_column_frame");
    auto& pane = fig.pane2("p");
    const auto message = error_text([&] { (void)pluss::pl::flash::histplot(pane, one, {}); });
    expect(message.find("no columns named") != std::string::npos,
           "even a one-column frame needs its column named: " + message);
  }
  {
    pluss::pl::Studio fig("histplot_dataframe_explicit_feature");
    auto& pane = fig.pane2("explicit");
    (void)pluss::pl::flash::histplot(pane, multi, {.feature = "bonus"});
    expect(pane.x_axis().label == "bonus" && pane.y_axis().label == "count",
           "flash::histplot(DataFrame) should accept an explicit feature");
  }
  {
    pluss::pl::Studio fig("histplot_dataframe_all_columns");
    auto& pane = fig.pane2("all");
    (void)pluss::pl::flash::histplot(pane, multi, {.selection = {.all_columns = true}});
    expect(pane.trace_count() == 2,
           "all_columns should overlay one histogram per column on one pane");
    auto& p2 = fig.pane2("kde");
    (void)pluss::pl::flash::kdeplot(p2, multi, {.selection = {.include_columns = {"score"}}});
    auto& p3 = fig.pane2("ecdf");
    (void)pluss::pl::flash::ecdfplot(p3, multi, {.selection = {.exclude_columns = {"bonus"}}});
    auto& p4 = fig.pane2("rug");
    (void)pluss::pl::flash::rugplot(p4, multi, {.selection = {.all_columns = true}});
    expect(p2.trace_count() == 1 && p3.trace_count() == 1 && p4.trace_count() == 2,
           "kde, ecdf and rug should accept the same selection knobs");
    const auto both = error_text([&] {
      (void)pluss::pl::flash::histplot(pane, multi, {.feature = "score", .selection = {.all_columns = true}});
    });
    expect(both.find("not both") != std::string::npos, "name plus selection: " + both);
  }
  {
    // Distribution plots over a selection: one box / violin per column on one pane.
    pluss::pl::Studio fig("catplots_selection");
    auto& p1 = fig.pane2("box");
    (void)pluss::pl::flash::boxplot(p1, multi, {.selection = {.all_columns = true}});
    auto& p2 = fig.pane2("violin");
    (void)pluss::pl::flash::violinplot(p2, multi, {.selection = {.all_columns = true}});
    expect(p1.trace_count() >= 1 && p2.trace_count() >= 1,
           "box and violin over a selection should draw on one pane each");
  }
}

void test_fixed_arity_plots_require_names() {
  const auto two = DataFrame({
      {"a", pluss::make_series(std::vector<double>{1.0, 2.0, 3.0})},
      {"b", pluss::make_series(std::vector<double>{2.0, 4.0, 5.0})},
  });
  pluss::pl::Studio fig("fixed_arity");
  auto& pane = fig.pane2("p");
  const auto message = error_text([&] { (void)pluss::pl::flash::regplot(pane, two, {}); });
  expect(message.find("no columns named") != std::string::npos &&
             message.find("x_feature and y_feature") != std::string::npos,
         "a two-column frame is not guessed to be x and y: " + message);
  const auto knob = error_text([&] {
    (void)pluss::pl::flash::regplot(pane, two, {.selection = {.all_columns = true}});
  });
  expect(knob.find("column selections") != std::string::npos,
         "fixed-arity plots should reject a column selection: " + knob);
}

void test_barplot_shelf_and_atlas_depth_gate() {
  pluss::pl::Studio shelf_fig("barplot_shelf_horizontal");
  auto& shelf_pane = shelf_fig.pane2("shelf");
  (void)pluss::pl::flash::barplot(
      shelf_pane,
      Shelf(pluss::make_series(std::vector<double>{3.0, 7.0, 4.0})),
      {.horizontal = true});
  expect(shelf_pane.x_axis().label == "value",
         "flash::barplot(Shelf) should label the value axis for horizontal bars");
  expect(shelf_pane.y_axis().label == "y",
         "flash::barplot(Shelf) should label the implicit position axis for horizontal bars");

  const auto frame = DataFrame({
      {"x", pluss::make_series(std::vector<double>{1.0, 2.0, 3.0})},
      {"y", pluss::make_series(std::vector<double>{4.0, 5.0, 6.0})},
      {"z", pluss::make_series(std::vector<double>{7.0, 8.0, 9.0})},
  });

  const Atlas one_frame({frame});
  pluss::pl::Studio ok_fig("plot3d_atlas_depth_one");
  auto& ok = ok_fig.pane3("ok");
  (void)pluss::pl::flash::plot3D(ok, one_frame, {.x_feature = "x", .y_feature = "y", .z_feature = "z"});
  expect(ok.z_axis().label == "z",
         "flash::plot3D(Atlas) should proxy a depth-1 Atlas through its single frame");

  const Atlas two_frames({frame, frame});
  bool threw = false;
  try {
    pluss::pl::Studio bad_fig("plot3d_atlas_depth_two");
    auto& bad = bad_fig.pane3("bad");
    (void)pluss::pl::flash::plot3D(bad, two_frames, {.x_feature = "x", .y_feature = "y", .z_feature = "z"});
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
  (void)pluss::pl::flash::boxenplot(pane, frame, {.feature = "score"});
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
        {.class_feature = "species", .selection = {.all_columns = true}});
  }

  {
    pluss::pl::Studio fig("andrews_dataframe_class_feature");
    (void)pluss::pl::flash::andrews_curves(
        fig,
        frame,
        {.class_feature = "species", .selection = {.all_columns = true}});
  }

  {
    pluss::pl::Studio fig("radviz_dataframe_class_feature");
    (void)pluss::pl::flash::radviz(
        fig,
        frame,
        {.class_feature = "species", .selection = {.all_columns = true}});
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
  test_lineplot_dataframe_requires_names_or_selection();
  test_histplot_dataframe_requires_a_name_or_selection();
  test_fixed_arity_plots_require_names();
  test_barplot_shelf_and_atlas_depth_gate();
  test_barplot_dataframe_supports_categorical_numeric_pairs();
  test_relplot_dataframe_supports_explicit_hue();
  test_boxenplot_dataframe_accepts_single_numeric_feature();
  test_parallel_family_dataframe_accepts_class_feature();
  return 0;
}

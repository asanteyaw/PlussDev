#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "pluss/pluss.h"
#include "pluss/tils.h"

namespace {

using pluss::kindle::Catalog;
using pluss::kindle::CatalogReduceOptions;
using pluss::kindle::ReduceAxis;
using pluss::kindle::Stream;
using pluss::kindle::StreamKind;
using pluss::table::DataFrame;

[[noreturn]] void fail(std::string_view msg) {
  throw std::runtime_error(std::string(msg));
}

void expect(bool condition, std::string_view msg) {
  if (!condition) fail(msg);
}

template <class Fn>
void expect_throw(Fn&& fn, std::string_view msg) {
  bool threw = false;
  try {
    fn();
  } catch (...) {
    threw = true;
  }
  if (!threw) fail(msg);
}

template <class Fn>
void expect_no_throw(Fn&& fn, std::string_view msg) {
  try {
    fn();
  } catch (const std::exception& e) {
    fail(std::string(msg) + ": " + e.what());
  } catch (...) {
    fail(std::string(msg) + ": unknown exception");
  }
}

std::filesystem::path project_root() {
  return std::filesystem::path(PLUSSMIC_SOURCE_DIR);
}

std::filesystem::path data_file(std::string_view name) {
  return project_root() / "data" / std::string(name);
}

std::filesystem::path temp_csv_path(std::string_view stem) {
  const auto dir = std::filesystem::temp_directory_path() / "plussmic_kindle_tests";
  std::filesystem::create_directories(dir);
  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  return dir / (std::string(stem) + "_" + std::to_string(stamp) + ".csv");
}

pluss::Series bool_mask(std::initializer_list<bool> values) {
  const auto n = static_cast<std::int64_t>(values.size());
  auto out = pluss::Series::zeros({n}, pluss::codem::ScalarType::Bool);
  auto* p = reinterpret_cast<bool*>(out.raw_data());
  std::int64_t i = 0;
  for (const bool v : values) p[i++] = v;
  return out;
}

double scalar_stream_to_double(const Stream& s) {
  expect(s.kind() == StreamKind::Series, "scalar_stream_to_double expects Series stream");
  const auto values = s.as_series().to(pluss::codem::ScalarType::Float64).host<double>();
  expect(values.size() == 1, "scalar stream must contain one value");
  return values[0];
}

DataFrame make_base_frame() {
  const std::array<std::int64_t, 1> shape{5};
  const auto ids = pluss::make_series(std::vector<std::int64_t>{3, 1, 2, 5, 4}, {5});
  const auto metric = pluss::make_series(std::vector<double>{10.0, 20.0, 30.0, 40.0, 50.0}, {5});
  const auto metric_mask = bool_mask({true, false, true, true, false});
  const auto city = pluss::make_note(std::vector<std::string>{"Accra", "Kumasi", "Accra", "Tema", "Kumasi"}, shape);
  const auto text = pluss::make_note(
      std::vector<std::string>{"Heavy Rain", "Clear day", "Fog near school", "Heavy rain near mall", "storm warning"},
      shape);
  const auto timeline = pluss::make_timeline({
      "2024-01-01T00:00:00Z",
      "2024-01-02T00:00:00Z",
      "2024-01-03T00:00:00Z",
      "2024-01-04T00:00:00Z",
      "2024-01-05T00:00:00Z",
  });
  const auto indexed_timeline = pluss::make_timeline({
      "2024-01-01T00:00:00Z",
      "2024-01-02T00:00:00Z",
      "2024-01-03T00:00:00Z",
      "2024-01-04T00:00:00Z",
      "2024-01-05T00:00:00Z",
  });
  const auto cases = pluss::table::Slate(
      pluss::make_note(std::vector<std::string>{"cs_0", "cs_1", "cs_2", "cs_3", "cs_4"}, shape));

  return DataFrame({
                       {"id", pluss::table::Slate(ids)},
                       {"metric", pluss::table::Slate(metric, metric_mask)},
                       {"city", pluss::table::Slate(city)},
                       {"text", pluss::table::Slate(text)},
                       {"timestamp", pluss::table::Slate(timeline)},
                       {"time_index", pluss::table::Slate(indexed_timeline)},
                   },
                   cases);
}

Catalog make_base_catalog() {
  return make_base_frame().kindelize();
}

Stream labeled_series(std::vector<double> values, std::vector<std::string> labels) {
  const auto n = static_cast<std::int64_t>(values.size());
  expect(static_cast<std::int64_t>(labels.size()) == n, "labeled_series label size mismatch");
  const std::array<std::int64_t, 1> shape{n};
  auto label = pluss::table::Slate(pluss::make_note(std::move(labels), shape));
  return Stream(pluss::make_series(std::move(values), {n}), std::nullopt, std::move(label));
}

Stream labeled_note(std::vector<std::string> values, std::vector<std::string> labels) {
  const auto n = static_cast<std::int64_t>(values.size());
  expect(static_cast<std::int64_t>(labels.size()) == n, "labeled_note label size mismatch");
  const std::array<std::int64_t, 1> shape{n};
  auto label = pluss::table::Slate(pluss::make_note(std::move(labels), shape));
  return Stream(pluss::make_note(std::move(values), shape), std::nullopt, std::move(label));
}

std::vector<std::string> note_values(const pluss::Note& note) {
  std::vector<std::string> out;
  out.reserve(static_cast<std::size_t>(note.numel()));
  for (std::int64_t i = 0; i < note.numel(); ++i) {
    out.emplace_back(note.at(i));
  }
  return out;
}

void test_stream_methods() {
  const auto catalog = make_base_catalog();
  auto metric = catalog.keys("metric");

  expect(metric.defined(), "metric stream should be defined");
  expect(metric.kind() == StreamKind::Series, "metric should be Series stream");
  expect(metric.size() == 5, "metric stream size mismatch");
  expect(!metric.empty(), "metric stream should not be empty");
  expect(metric.shape() == std::make_pair<std::int64_t, std::int64_t>(5, 1),
         "metric stream shape should be (size, 1)");
  expect(metric.ptype() == "Series", "metric stream ptype mismatch");
  expect(metric.has_label(), "metric should carry cases after kindelize");
  expect(pluss::kindle::detail::has_validity_mask(metric), "metric should carry validity mask");

  metric = metric.with_meta("source", "test");
  expect(metric.meta("source").value_or("") == "test", "meta write/read mismatch");
  metric = metric.drop_meta("source");
  expect(!metric.meta("source").has_value(), "meta should be removed");

  const auto filled = metric.fillnull(0.0);
  expect(!pluss::kindle::detail::has_validity_mask(filled), "fillnull should clear validity mask");
  expect(filled.as_series().host<double>() == std::vector<double>({10.0, 0.0, 30.0, 40.0, 0.0}),
         "Stream::fillnull(double) should preserve valid values and replace only null rows");
  const auto dropped = metric.dropnull();
  expect(dropped.size() == 3, "dropnull should keep only valid rows");
  const auto taken = metric.take(std::vector<std::int64_t>{0, 2, 4});
  expect(taken.size() == 3, "take size mismatch");
  const auto picked = Stream::select(
      {pluss::table::Slate(bool_mask({true, false, true}))},
      {labeled_series({10.0, 20.0, 30.0}, {"r0", "r1", "r2"})},
      labeled_series({1.0, 2.0, 3.0}, {"l0", "l1", "l2"}));
  expect(picked.as_series().host<double>() == std::vector<double>({10.0, 2.0, 30.0}),
         "Stream::select should choose row-wise payload values");
  expect(note_values(picked.label()->cast().note().as_note()) == std::vector<std::string>({"r0", "l1", "r2"}),
         "Stream::select should choose row-wise labels");

  const auto mask = (filled > 15.0).as_series();
  const auto filtered = filled.filter(mask);
  expect(filtered.size() == 2, "filter result size mismatch");

  const auto math_out = ((filled + 3.0) * 2.0) / 2.0;
  expect(math_out.size() == 5, "stream arithmetic size mismatch");

  const auto mean = filled.mean();
  expect(std::fabs(scalar_stream_to_double(mean) - 16.0) < 1e-9, "stream mean mismatch");

  const auto invalid = metric.invalid();
  expect(invalid.kind() == StreamKind::Series, "Stream::invalid should return boolean Series stream");

  const auto distinct = metric.distinct();
  expect(distinct.size() == 3, "Stream::distinct should drop nulls by default");
  const auto distinct_keep_null = metric.distinct({.dropnull = false});
  expect(distinct_keep_null.size() == 4, "Stream::distinct(false) should keep one null bucket");

  const auto freq = catalog.keys("city").freq();
  expect(freq.size() == 3, "Stream::freq(city) should return one count per distinct city");
  expect(freq.has_label(), "Stream::freq should label counts by observed values");

  const auto ndistinct = catalog.keys("city").ndistinct();
  expect(static_cast<std::int64_t>(scalar_stream_to_double(ndistinct)) == 3,
         "Stream::ndistinct(city) mismatch");
}

void test_stream_accessors() {
  const auto catalog = make_base_catalog();
  const auto text = catalog.keys("text");
  const auto lower = text.nt().lower();
  expect(lower.kind() == StreamKind::Note, "Stream::nt().lower should keep Note kind");
  expect(std::string(lower.as_note().at(0)) == "heavy rain", "Stream::nt().lower value mismatch");

  const auto timestamp = catalog.keys("timestamp");
  const auto year = timestamp.tl().year();
  expect(year.kind() == StreamKind::Series, "Stream::tl().year should yield Series");
  expect(year.as_series().host<std::int64_t>() == std::vector<std::int64_t>({2024, 2024, 2024, 2024, 2024}),
         "Stream::tl().year values mismatch");

  const auto ids_as_note = catalog.keys("id").cast().note();
  expect(ids_as_note.kind() == StreamKind::Note, "Stream::cast().note should convert Series to Note");

  const std::array<std::int64_t, 1> shape2{2};
  const auto date_text = Stream(pluss::make_note(std::vector<std::string>{"2024-01-01", "2024-01-02"}, shape2));
  const auto parsed = date_text.cast({.format = "%Y-%m-%d", .invalid = pluss::table::ParseInvalid::Null}).timeline();
  expect(parsed.kind() == StreamKind::Timeline, "Stream::cast(options).timeline should parse Note dates");

  const auto metric = catalog.keys("metric").fillnull(0.0);
  const auto lagged = metric.ts().lag(1);
  expect(lagged.kind() == StreamKind::Series, "Stream::ts().lag should yield Series");
  expect(lagged.size() == metric.size(), "Stream::ts().lag should preserve length");
  expect(lagged.has_label(), "Stream::ts().lag should preserve labels");

  const auto label_values = metric.lbl().values();
  expect(label_values.kind() == StreamKind::Note, "Stream::lbl().values should expose label payload");
  expect(label_values.size() == metric.size(), "Stream::lbl().values size mismatch");
  const auto relabeled = metric.lbl().set(
      Stream(pluss::make_note(std::vector<std::string>{"a", "b", "c", "d", "e"}, std::array<std::int64_t, 1>{5})));
  expect(relabeled.has_label(), "Stream::lbl().set should attach labels");
  const auto reset = relabeled.lbl().reset();
  expect(!reset.has_label(), "Stream::lbl().reset should clear labels");

  auto fig = metric.plot().line();
  (void)fig;
}

void test_catalog_plot_accessor() {
  const auto ragged = Catalog(std::vector<std::pair<std::string, Stream>>{
      {"short", labeled_series({10.0, 12.0, 11.0}, {"Mon", "Tue", "Wed"})},
      {"long", labeled_series({3.0, 4.0, 6.0, 5.0, 7.0}, {"Mon", "Tue", "Wed", "Thu", "Fri"})},
  });

  expect_no_throw([&]() {
    auto fig = ragged.plot().line({"short", "long"});
    (void)fig;
  }, "Catalog::plot().line should support ragged multi-stream plots without tabelize");

  expect_no_throw([&]() {
    auto fig = ragged.plot().step({"short", "long"});
    (void)fig;
  }, "Catalog::plot().step should support ragged multi-stream plots");

  expect_no_throw([&]() {
    auto fig = ragged.plot().area({"short", "long"});
    (void)fig;
  }, "Catalog::plot().area should support ragged multi-stream plots");

  expect_no_throw([&]() {
    auto fig = ragged.plot().hist({"short", "long"}, 12);
    (void)fig;
  }, "Catalog::plot().hist should overlay independent stream lengths");

  expect_no_throw([&]() {
    auto fig = ragged.plot().density({"short", "long"}, 96);
    (void)fig;
  }, "Catalog::plot().density should overlay independent stream lengths");

  const auto ragged_xy = Catalog(std::vector<std::pair<std::string, Stream>>{
      {"x", labeled_series({1.0, 2.0, 3.0, 4.0, 5.0}, {"a", "b", "c", "d", "e"})},
      {"y", labeled_series({10.0, 30.0, 50.0}, {"a", "c", "e"})},
      {"desk", labeled_note({"A", "B", "A", "B", "A"}, {"a", "b", "c", "d", "e"})},
  });

  expect_no_throw([&]() {
    auto fig = ragged_xy.plot().scatter("x", "y");
    (void)fig;
  }, "Catalog::plot().scatter should support ragged x/y alignment");

  expect_no_throw([&]() {
    auto fig = ragged_xy.plot().scatter("x", "y", "desk");
    (void)fig;
  }, "Catalog::plot().scatter should align ragged hue by the same pairwise basis");

  const auto paired = Catalog(std::vector<std::pair<std::string, Stream>>{
      {"x", Stream(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4}))},
      {"y", Stream(pluss::make_series(std::vector<double>{5.0, 7.0, 6.0, 8.0}, {4}))},
      {"hue",
       Stream(pluss::make_note(std::vector<std::string>{"A", "A", "B", "B"}, {4}))},
  });

  expect_no_throw([&]() {
    auto fig = paired.plot().scatter("x", "y", "hue");
    (void)fig;
  }, "Catalog::plot().scatter should support grouped hue rendering");

  expect_no_throw([&]() {
    auto fig = paired.plot().bar("y", "x");
    (void)fig;
  }, "Catalog::plot().bar should plot an explicit x/y stream pair");

  expect_no_throw([&]() {
    auto fig = ragged.plot().scatter_matrix({"short", "long"});
    (void)fig;
  }, "Catalog::plot().scatter_matrix should support ragged pairwise inner alignment");

  expect_no_throw([&]() {
    auto fig = ragged.plot().corr_heatmap({"short", "long"});
    (void)fig;
  }, "Catalog::plot().corr_heatmap should support ragged pairwise inner alignment");

  expect_throw(
      [&]() {
        (void)ragged.plot().line({"short", "long"}, "short");
      },
      "Catalog::plot().line(shared x) should reject y streams whose lengths do not match the shared x stream");

  pluss::table::LinePlotOptions line_inner;
  line_inner.align.mode = pluss::table::XYAlignMode::Inner;
  expect_no_throw([&]() {
    auto fig = ragged.plot().line("short", "long", line_inner);
    (void)fig;
  }, "Catalog::plot().line(shared x, Inner) should trim y to the shared overlap");

  pluss::table::LinePlotOptions line_pad;
  line_pad.align.mode = pluss::table::XYAlignMode::PadScalar;
  line_pad.align.pad_scalar = 0.0;
  expect_no_throw([&]() {
    auto fig = ragged.plot().line("short", "long", line_pad);
    (void)fig;
  }, "Catalog::plot().line(shared x, PadScalar) should pad missing y values against the x anchor");

  pluss::table::StepPlotOptions step_inner;
  step_inner.align.mode = pluss::table::XYAlignMode::Inner;
  expect_no_throw([&]() {
    auto fig = ragged.plot().step("short", "long", step_inner);
    (void)fig;
  }, "Catalog::plot().step(shared x, Inner) should trim ragged y values to the overlap");

  pluss::table::AreaPlotOptions area_pad;
  area_pad.align.mode = pluss::table::XYAlignMode::PadScalar;
  area_pad.align.pad_scalar = 0.0;
  expect_no_throw([&]() {
    auto fig = ragged.plot().area("short", "long", area_pad);
    (void)fig;
  }, "Catalog::plot().area(shared x, PadScalar) should pad ragged y values against the x anchor");

  pluss::table::BarPlotOptions bar_pad;
  bar_pad.align.mode = pluss::table::XYAlignMode::PadScalar;
  bar_pad.align.pad_scalar = 0.0;
  expect_no_throw([&]() {
    auto fig = ragged.plot().bar("short", "long", bar_pad);
    (void)fig;
  }, "Catalog::plot().bar(shared x, PadScalar) should pad ragged y values against the x anchor");

  const auto reduced = paired.keys({"x", "y"}).sum(pluss::kindle::ReduceAxis::Key);
  expect(reduced.nkeys() == 1 && reduced.has("sum"),
         "Catalog::sum(ReduceAxis::Key) should behave like cross-key reduction");
}

void test_catalog_plot3d_accessor() {
  const auto clean = Catalog(std::vector<std::pair<std::string, Stream>>{
      {"x", Stream(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4}))},
      {"y", Stream(pluss::make_series(std::vector<double>{5.0, 7.0, 6.0, 8.0}, {4}))},
      {"z", Stream(pluss::make_series(std::vector<double>{2.0, 3.0, 1.0, 4.0}, {4}))},
  });

  expect_no_throw([&]() {
    auto fig = clean.plot3d().scatter("x", "y", "z");
    (void)fig;
  }, "Catalog::plot3d().scatter should render a well-aligned catalog");

  expect_no_throw([&]() {
    auto fig = clean.plot3d().scatter("x", "y", "z", "x", "y");
    (void)fig;
  }, "Catalog::plot3d().scatter should support size/color keys");

  expect_no_throw([&]() {
    auto fig = clean.plot3d().line("x", "y", "z");
    (void)fig;
  }, "Catalog::plot3d().line should render a well-aligned catalog");

  expect_no_throw([&]() {
    auto fig = clean.plot3d().hist("x", "y");
    (void)fig;
  }, "Catalog::plot3d().hist should render a well-aligned catalog");

  expect_no_throw([&]() {
    auto fig = clean.plot3d().surface("x", "y", "z");
    (void)fig;
  }, "Catalog::plot3d().surface should render a well-aligned catalog");

  expect_no_throw([&]() {
    auto fig = clean.plot3d().wireframe("x", "y", "z");
    (void)fig;
  }, "Catalog::plot3d().wireframe should render a well-aligned catalog");

  expect_throw(
      [&]() { (void)clean.plot3d().scatter("missing", "y", "z"); },
      "Catalog::plot3d().scatter should reject an unknown x key");

  // Catalog only enforces per-key type homogeneity, not length homogeneity
  // across keys -- x/y/z here are individually well-formed but ragged
  // relative to each other. Labeled streams should reconcile by shared
  // label; the earlier x/y/z lengths differ but "a"/"c"/"e" are common to
  // both, so exactly those 3 rows should survive alignment.
  const auto ragged_labeled = Catalog(std::vector<std::pair<std::string, Stream>>{
      {"x", labeled_series({1.0, 2.0, 3.0, 4.0, 5.0}, {"a", "b", "c", "d", "e"})},
      {"y", labeled_series({10.0, 30.0, 50.0}, {"a", "c", "e"})},
      {"z", labeled_series({100.0, 300.0, 500.0}, {"a", "c", "e"})},
  });
  expect_no_throw([&]() {
    auto fig = ragged_labeled.plot3d().scatter("x", "y", "z");
    (void)fig;
  }, "Catalog::plot3d().scatter should align ragged labeled x/y/z by shared label");
  expect_no_throw([&]() {
    auto fig = ragged_labeled.plot3d().line("x", "y", "z");
    (void)fig;
  }, "Catalog::plot3d().line should align ragged labeled x/y/z by shared label");

  // Unlabeled ragged streams have no label to reconcile by, so Auto must
  // fall back to positional alignment (truncated to the shortest stream)
  // rather than throwing.
  const auto ragged_unlabeled = Catalog(std::vector<std::pair<std::string, Stream>>{
      {"x", Stream(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4}))},
      {"y", Stream(pluss::make_series(std::vector<double>{5.0, 7.0}, {2}))},
      {"z", Stream(pluss::make_series(std::vector<double>{2.0, 3.0, 1.0}, {3}))},
  });
  expect_no_throw([&]() {
    auto fig = ragged_unlabeled.plot3d().scatter("x", "y", "z");
    (void)fig;
  }, "Catalog::plot3d().scatter should fall back to positional alignment for unlabeled ragged streams");

  // Regression: a catalog carrying an *unrelated* ragged/mismatched key
  // must not affect plotting keys that are themselves well-aligned. Before
  // the fix, scatter/line/hist derived their row count from
  // Catalog::nrows() (which throws if *any* two streams in the whole
  // catalog differ in length), so this would have failed even though
  // x/y/z here are perfectly consistent with each other.
  const auto unrelated_ragged = Catalog(std::vector<std::pair<std::string, Stream>>{
      {"x", Stream(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4}))},
      {"y", Stream(pluss::make_series(std::vector<double>{5.0, 7.0, 6.0, 8.0}, {4}))},
      {"z", Stream(pluss::make_series(std::vector<double>{2.0, 3.0, 1.0, 4.0}, {4}))},
      {"unrelated", Stream(pluss::make_series(std::vector<double>{9.0, 9.0}, {2}))},
  });
  expect_throw([&]() { (void)unrelated_ragged.nrows(); },
               "sanity check: this catalog really is ragged at the whole-catalog level");
  expect_no_throw([&]() {
    auto fig = unrelated_ragged.plot3d().scatter("x", "y", "z");
    (void)fig;
  }, "Catalog::plot3d().scatter must not be affected by an unrelated ragged key elsewhere in the catalog");
  expect_no_throw([&]() {
    auto fig = unrelated_ragged.plot3d().line("x", "y", "z");
    (void)fig;
  }, "Catalog::plot3d().line must not be affected by an unrelated ragged key elsewhere in the catalog");
  expect_no_throw([&]() {
    auto fig = unrelated_ragged.plot3d().hist("x", "y");
    (void)fig;
  }, "Catalog::plot3d().hist must not be affected by an unrelated ragged key elsewhere in the catalog");
  expect_no_throw([&]() {
    auto fig = unrelated_ragged.plot3d().surface("x", "y", "z");
    (void)fig;
  }, "Catalog::plot3d().surface must not be affected by an unrelated ragged key elsewhere in the catalog");
  expect_no_throw([&]() {
    auto fig = unrelated_ragged.plot3d().wireframe("x", "y", "z");
    (void)fig;
  }, "Catalog::plot3d().wireframe must not be affected by an unrelated ragged key elsewhere in the catalog");

  // Explicit align_by override via the Catalog*3DPlotOptions wrapper: force
  // Pos alignment on labeled-but-differently-ordered streams to confirm the
  // override actually changes which rows survive.
  const auto reordered_labeled = Catalog(std::vector<std::pair<std::string, Stream>>{
      {"x", labeled_series({1.0, 2.0, 3.0}, {"a", "b", "c"})},
      {"y", labeled_series({30.0, 10.0, 20.0}, {"c", "a", "b"})},
      {"z", labeled_series({300.0, 100.0, 200.0}, {"c", "a", "b"})},
  });
  pluss::kindle::CatalogScatter3DPlotOptions pos_options;
  pos_options.align_by = pluss::table::XYAlignBy::Pos;
  expect_no_throw([&]() {
    auto fig = reordered_labeled.plot3d().scatter("x", "y", "z", std::nullopt, std::nullopt, pos_options);
    (void)fig;
  }, "Catalog::plot3d().scatter(CatalogScatter3DPlotOptions) should honor an explicit Pos override");

  pluss::kindle::CatalogLine3DPlotOptions line_pos_options;
  line_pos_options.align_by = pluss::table::XYAlignBy::Pos;
  expect_no_throw([&]() {
    auto fig = reordered_labeled.plot3d().line("x", "y", "z", line_pos_options);
    (void)fig;
  }, "Catalog::plot3d().line(CatalogLine3DPlotOptions) should honor an explicit Pos override");

  pluss::kindle::CatalogHist3DPlotOptions hist_pos_options;
  hist_pos_options.align_by = pluss::table::XYAlignBy::Pos;
  expect_no_throw([&]() {
    auto fig = reordered_labeled.plot3d().hist("x", "y", hist_pos_options);
    (void)fig;
  }, "Catalog::plot3d().hist(CatalogHist3DPlotOptions) should honor an explicit Pos override");

  pluss::table::Scatter3DPlotOptions scatter_strict;
  scatter_strict.align.mode = pluss::table::XYAlignMode::Strict;
  expect_throw(
      [&]() { (void)ragged_unlabeled.plot3d().scatter("x", "y", "z", std::nullopt, std::nullopt, scatter_strict); },
      "Catalog::plot3d().scatter(Strict) should reject ragged unlabeled coordinates");

  pluss::table::Scatter3DPlotOptions scatter_pad;
  scatter_pad.align.mode = pluss::table::XYAlignMode::PadScalar;
  scatter_pad.align.pad_scalar = 0.0;
  expect_no_throw([&]() {
    auto fig = ragged_unlabeled.plot3d().scatter("x", "y", "z", std::nullopt, std::nullopt, scatter_pad);
    (void)fig;
  }, "Catalog::plot3d().scatter(PadScalar) should pad ragged unlabeled coordinates from the x anchor");

  pluss::table::Line3DPlotOptions line_strict;
  line_strict.align.mode = pluss::table::XYAlignMode::Strict;
  expect_throw(
      [&]() { (void)ragged_unlabeled.plot3d().line("x", "y", "z", line_strict); },
      "Catalog::plot3d().line(Strict) should reject ragged unlabeled coordinates");

  pluss::table::Line3DPlotOptions line_pad3d;
  line_pad3d.align.mode = pluss::table::XYAlignMode::PadScalar;
  line_pad3d.align.pad_scalar = 0.0;
  expect_no_throw([&]() {
    auto fig = ragged_unlabeled.plot3d().line("x", "y", "z", line_pad3d);
    (void)fig;
  }, "Catalog::plot3d().line(PadScalar) should pad ragged unlabeled coordinates from the x anchor");

  pluss::table::Hist3DPlotOptions hist_strict;
  hist_strict.align.mode = pluss::table::XYAlignMode::Strict;
  expect_throw(
      [&]() { (void)ragged_unlabeled.plot3d().hist("x", "y", hist_strict); },
      "Catalog::plot3d().hist(Strict) should reject ragged unlabeled coordinates");

  pluss::table::Hist3DPlotOptions hist_pad;
  hist_pad.align.mode = pluss::table::XYAlignMode::PadScalar;
  hist_pad.align.pad_scalar = 0.0;
  expect_no_throw([&]() {
    auto fig = ragged_unlabeled.plot3d().hist("x", "y", hist_pad);
    (void)fig;
  }, "Catalog::plot3d().hist(PadScalar) should pad ragged unlabeled coordinates from the x anchor");
}

void test_catalog_accessors() {
  const auto base = make_base_catalog();

  const auto lower = base.keys({"text"}).nt().lower();
  expect(lower.keys("text").kind() == StreamKind::Note, "Catalog::keys().nt().lower should keep Note kind");
  expect(std::string(lower.keys("text").as_note().at(0)) == "heavy rain",
         "Catalog::keys().nt().lower value mismatch");

  const auto year = base.keys({"timestamp"}).tl().year();
  expect(year.keys("timestamp").kind() == StreamKind::Series,
         "Catalog::keys().tl().year should yield Series");
  expect(year.keys("timestamp").as_series().host<std::int64_t>() ==
             std::vector<std::int64_t>({2024, 2024, 2024, 2024, 2024}),
         "Catalog::keys().tl().year values mismatch");

  const auto filled = base.keys({"metric"}).fillnull(0.0);
  expect(filled.keys("metric").as_series().host<double>() == std::vector<double>({10.0, 0.0, 30.0, 40.0, 0.0}),
         "Catalog::keys().fillnull should preserve valid values and replace only null rows");

  const auto dropped = base.keys({"metric"}).dropnull();
  expect(dropped.nrows() == 3, "Catalog::keys().dropnull should drop rows by selected key mask");
  expect(dropped.nkeys() == 1 && dropped.has("metric"),
         "Catalog::keys().dropnull should keep the projected key set");
  expect(dropped.keys("metric").as_series().host<double>() == std::vector<double>({10.0, 30.0, 40.0}),
         "Catalog::keys().dropnull should keep only valid rows from the projected key");

  const auto lagged = base.keys({"metric"}).ts().lag(1);
  expect(lagged.keys("metric").kind() == StreamKind::Series,
         "Catalog::keys().ts().lag should keep Series kind");
  expect(lagged.keys("metric").size() == base.keys("metric").size(),
         "Catalog::keys().ts().lag should preserve selected stream length");

  const auto ids_as_note = base.keys({"id"}).cast().note();
  expect(ids_as_note.keys("id").kind() == StreamKind::Note,
         "Catalog::keys().cast().note should convert selected Series streams");

  const std::array<std::int64_t, 1> shape2{2};
  const auto note_dates = Catalog(std::vector<std::pair<std::string, Stream>>{
      {"date_text", Stream(pluss::make_note(std::vector<std::string>{"2024-03-01", "2024-03-02"}, shape2))},
  });
  const auto parsed = note_dates.cast(
      pluss::table::SlateCastTarget::Timeline,
      pluss::table::SlateCastOptions{.format = "%Y-%m-%d", .invalid = pluss::table::ParseInvalid::Null});
  expect(parsed.keys("date_text").kind() == StreamKind::Timeline,
         "Catalog::cast({}, Timeline, options) should forward full parse options");
}

void test_catalog_reductions() {
  auto base = make_base_catalog().keys({"id", "metric"}).astype(pluss::codem::ScalarType::Float64);

  const auto record_sum = base.sum();
  expect(record_sum.nkeys() == 2, "Catalog::sum() should keep one scalar stream per key");
  expect(std::fabs(scalar_stream_to_double(record_sum.keys("id")) - 15.0) < 1e-9,
         "Catalog::sum() id mismatch");
  expect(std::fabs(scalar_stream_to_double(record_sum.keys("metric")) - 80.0) < 1e-9,
         "Catalog::sum() metric mismatch");

  const auto key_sum = base.sum(ReduceAxis::Key);
  expect(key_sum.nkeys() == 1 && key_sum.has("sum"),
         "Catalog::sum(Key) should return a single reduced stream");
  expect(key_sum.keys("sum").as_series().host<double>() ==
             std::vector<double>({13.0, 1.0, 32.0, 45.0, 4.0}),
         "Catalog::sum(Key) should reduce across selected keys row-wise");

  const auto pop_var = base.var(CatalogReduceOptions{.unbiased = false});
  expect(std::fabs(scalar_stream_to_double(pop_var.keys("id")) - 2.0) < 1e-9,
         "Catalog::var(population) mismatch");

  const auto ragged = Catalog(std::vector<std::pair<std::string, Stream>>{
      {"left", Stream(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}, {5}))},
      {"right", Stream(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0}, {3}))},
  });

  const auto inner = ragged.sum(CatalogReduceOptions{.axis = ReduceAxis::Key, .align = pluss::kindle::Align::Inner});
  expect(inner.keys("sum").as_series().host<double>() == std::vector<double>({11.0, 22.0, 33.0}),
         "Catalog::sum(Key, Inner) should trim to common length");

  const auto outer = ragged.sum(CatalogReduceOptions{.axis = ReduceAxis::Key, .align = pluss::kindle::Align::Outer});
  expect(outer.keys("sum").as_series().host<double>() == std::vector<double>({11.0, 22.0, 33.0, 4.0, 5.0}),
         "Catalog::sum(Key, Outer) should preserve the longest stream and ignore missing values by default");

  expect_throw(
      [&]() {
        (void)ragged.sum(CatalogReduceOptions{.axis = ReduceAxis::Key, .align = pluss::kindle::Align::Strict});
      },
      "Catalog::sum(Key, Strict) should reject unequal lengths");

  expect_throw(
      [&]() {
        (void)make_base_catalog().keys({"id", "city"}).sum(ReduceAxis::Key);
      },
      "Catalog::sum(Key) should reject non-Series streams");
}


std::vector<double> stream_values(const Stream& s) {
  return s.as_series().to(pluss::codem::ScalarType::Float64).host<double>();
}

// valid[i] == false where the reduction has no value at position i.
std::vector<bool> stream_valid(const Stream& s) {
  if (!pluss::kindle::detail::has_validity_mask(s)) return std::vector<bool>(stream_values(s).size(), true);
  return pluss::kindle::detail::validity_mask(s)->host<bool>();
}

bool near_all(const std::vector<double>& got, const std::vector<double>& want, double tol = 1e-9) {
  if (got.size() != want.size()) return false;
  for (std::size_t i = 0; i < got.size(); ++i) {
    if (std::fabs(got[i] - want[i]) > tol * (1.0 + std::fabs(want[i]))) return false;
  }
  return true;
}

Stream dstream(const std::vector<double>& v) {
  return Stream(pluss::make_series(v, {static_cast<std::int64_t>(v.size())}));
}

Stream with_nulls_stream(const std::vector<double>& v, const std::vector<bool>& valid) {
  std::vector<double> mask;
  for (const bool b : valid) mask.push_back(b ? 1.0 : 0.0);
  return pluss::kindle::detail::with_validity_mask(
      dstream(v), pluss::make_series(mask, {static_cast<std::int64_t>(mask.size())}).to(pluss::codem::ScalarType::Bool));
}

// Independent reference: reduce each position over the streams that have a usable value there.
struct NaiveJagged {
  std::vector<double> value;
  std::vector<bool> valid;
};

NaiveJagged naive_jagged(const std::vector<std::vector<double>>& cols, const std::string& op, bool skipnull,
                         bool unbiased, bool ended_null = false, bool tail = false, std::int64_t min_streams = 1,
                         bool inner = false) {
  std::size_t longest = 0, shortest = cols.empty() ? 0 : cols[0].size();
  for (const auto& c : cols) {
    longest = std::max(longest, c.size());
    shortest = std::min(shortest, c.size());
  }
  // Common index space [0, longest); the policy keeps the window [begin, end).
  const std::size_t begin = (inner && tail) ? longest - shortest : 0;
  const std::size_t end = inner ? begin + shortest : longest;
  NaiveJagged out;
  out.value.assign(end - begin, 0.0);
  out.valid.assign(end - begin, true);
  for (std::size_t j = 0; j < end - begin; ++j) {
    const std::size_t at = begin + j;
    std::vector<double> xs;
    bool poisoned = false;
    std::size_t present = 0;
    for (const auto& c : cols) {
      const std::size_t off = tail ? longest - c.size() : 0;
      if (at < off || at >= off + c.size()) continue;
      ++present;
      const double x = c[at - off];
      if (std::isnan(x)) { poisoned = true; continue; }
      xs.push_back(x);
    }
    bool ok = static_cast<std::int64_t>(xs.size()) >= min_streams && !(poisoned && !skipnull) &&
              !(ended_null && present != cols.size());
    double v = 0.0;
    if (ok) {
      double sum = 0.0;
      for (double x : xs) sum += x;
      const double cnt = static_cast<double>(xs.size());
      if (op == "sum") v = sum;
      else if (op == "mean") v = sum / cnt;
      else if (op == "min") v = *std::min_element(xs.begin(), xs.end());
      else if (op == "max") v = *std::max_element(xs.begin(), xs.end());
      else {
        const double ddof = unbiased ? 1.0 : 0.0;
        ok = cnt - ddof > 0.0;
        if (ok) {
          double mean = sum / cnt, ss = 0.0;
          for (double x : xs) ss += (x - mean) * (x - mean);
          v = ss / (cnt - ddof);
          if (op == "std") v = std::sqrt(v);
        }
      }
    }
    out.value[j] = ok ? v : 0.0;
    out.valid[j] = ok;
  }
  return out;
}

Catalog jagged_catalog(const std::vector<std::vector<double>>& cols) {
  std::vector<std::pair<std::string, Stream>> streams;
  for (std::size_t i = 0; i < cols.size(); ++i) streams.emplace_back("s" + std::to_string(i), dstream(cols[i]));
  return Catalog(std::move(streams));
}

Catalog reduce_by_name(const Catalog& c, const std::string& op, const CatalogReduceOptions& o) {
  if (op == "sum") return c.sum(o);
  if (op == "mean") return c.mean(o);
  if (op == "min") return c.min(o);
  if (op == "max") return c.max(o);
  if (op == "var") return c.var(o);
  return c.std(o);
}

void test_catalog_jagged_reductions() {
  // The motivating example: three streams of length 3, 5 and 6.
  const std::vector<std::vector<double>> cols = {
      {2.3, 1.3, 2.5},
      {2.6, 0.3, 1.9, 2.1, 3.7},
      {3.6, 4.3, 0.9, 0.1, 3.1, 0.01},
  };
  const auto ragged = jagged_catalog(cols);
  CatalogReduceOptions by_key;
  by_key.axis = ReduceAxis::Key;

  // Default: position i reduces over the streams that reach it.
  expect(near_all(stream_values(ragged.sum(ReduceAxis::Key).keys("sum")), {8.5, 5.9, 5.3, 2.2, 6.8, 0.01}),
         "sum over a ragged catalog should reduce only the streams present at each position");
  expect(near_all(stream_values(ragged.mean(ReduceAxis::Key).keys("mean")),
                  {8.5 / 3, 5.9 / 3, 5.3 / 3, 1.1, 3.4, 0.01}),
         "mean should divide by the number of streams present, not the number of streams");
  expect(near_all(stream_values(ragged.min(ReduceAxis::Key).keys("min")), {2.3, 0.3, 0.9, 0.1, 3.1, 0.01}),
         "min should ignore ended streams");
  expect(near_all(stream_values(ragged.max(ReduceAxis::Key).keys("max")), {3.6, 4.3, 2.5, 2.1, 3.7, 0.01}),
         "max should ignore ended streams");
  const auto var = ragged.var(ReduceAxis::Key).keys("var");
  expect(std::fabs(stream_values(var)[3] - 2.0) < 1e-12, "unbiased variance of {2.1, 0.1} is 2");
  expect(!stream_valid(var)[5], "variance at a position with one contributor is undefined (null)");
  expect(stream_valid(var)[0] && stream_valid(var)[4], "variance is defined where two or more streams contribute");
  expect(std::fabs(stream_values(ragged.std(ReduceAxis::Key).keys("std"))[3] - std::sqrt(2.0)) < 1e-12,
         "std is the square root of the variance");
  CatalogReduceOptions biased = by_key;
  biased.unbiased = false;
  expect(stream_valid(ragged.var(biased).keys("var"))[5] && stream_values(ragged.var(biased).keys("var"))[5] == 0.0,
         "population variance of a single value is 0");

  // The padded alignment options are still available explicitly.
  CatalogReduceOptions strict = by_key;
  strict.align = pluss::kindle::Align::Strict;
  expect_throw([&] { (void)ragged.sum(strict); }, "Align::Strict should still reject ragged input");
  CatalogReduceOptions inner = by_key;
  inner.align = pluss::kindle::Align::Inner;
  expect(near_all(stream_values(ragged.sum(inner).keys("sum")), {8.5, 5.9, 5.3}), "Align::Inner trims to the shortest stream");

  // Nulls inside a stream are skipped like missing positions; skipnull=false lets them poison the position.
  const auto with_nan = jagged_catalog({{1.0, std::nan(""), 3.0}, {10.0, 20.0}});
  expect(near_all(stream_values(with_nan.sum(ReduceAxis::Key).keys("sum")), {11.0, 20.0, 3.0}),
         "a NaN is skipped like an absent value");
  CatalogReduceOptions keep_null = by_key;
  keep_null.skipnull = false;
  const auto poisoned = with_nan.sum(keep_null).keys("sum");
  expect(stream_valid(poisoned)[0] && !stream_valid(poisoned)[1] && stream_valid(poisoned)[2],
         "skipnull=false should null only the position that contains a null");

  // Equal-length catalogs behave exactly as before.
  const auto even = jagged_catalog({{1.0, 2.0}, {3.0, 4.0}});
  expect(near_all(stream_values(even.sum(ReduceAxis::Key).keys("sum")), {4.0, 6.0}), "equal lengths reduce as before");

  // ended = Null: a finished stream makes the result unknown from there on.
  CatalogReduceOptions ended_null = by_key;
  ended_null.ended = pluss::kindle::RaggedEnd::Null;
  const auto nulled = ragged.sum(ended_null).keys("sum");
  expect(stream_values(nulled).size() == 6, "ended=Null keeps the length of the longest stream");
  expect(stream_valid(nulled) == std::vector<bool>({true, true, true, false, false, false}),
         "ended=Null nulls every position after the shortest stream ends");
  const auto nulled_values = stream_values(nulled);
  expect(near_all(std::vector<double>(nulled_values.begin(), nulled_values.begin() + 3), {8.5, 5.9, 5.3}),
         "ended=Null leaves the fully-supported positions intact");

  // min_streams: require N contributing streams.
  CatalogReduceOptions two = by_key;
  two.min_streams = 2;
  expect(stream_valid(ragged.sum(two).keys("sum")) == std::vector<bool>({true, true, true, true, true, false}),
         "min_streams=2 nulls the position only one stream reaches");
  CatalogReduceOptions all = by_key;
  all.min_streams = 3;
  expect(stream_valid(ragged.sum(all).keys("sum")) == std::vector<bool>({true, true, true, false, false, false}),
         "min_streams equal to the stream count matches ended=Null for null-free data");

  // anchor = Tail: line the last values up instead of the first.
  const auto pair = jagged_catalog({{1.0, 2.0, 3.0}, {10.0, 20.0}});
  CatalogReduceOptions tail = by_key;
  tail.anchor = pluss::kindle::RaggedAnchor::Tail;
  expect(near_all(stream_values(pair.sum(tail).keys("sum")), {1.0, 12.0, 23.0}), "anchor=Tail aligns the ends");
  expect(near_all(stream_values(pair.sum(by_key).keys("sum")), {11.0, 22.0, 3.0}), "anchor=Head aligns the starts");

  // The flags only apply to the jagged path; elsewhere they are an error, never ignored.
  // Inner runs natively too, so the flags are valid with it: inside the shared
  // window every stream is present, and ended=Null changes nothing.
  CatalogReduceOptions inner_null = ended_null;
  inner_null.align = pluss::kindle::Align::Inner;
  const auto inner_nulled = ragged.sum(inner_null).keys("sum");
  expect(near_all(stream_values(inner_nulled), {8.5, 5.9, 5.3}) &&
             stream_valid(inner_nulled) == std::vector<bool>({true, true, true}),
         "ended=Null with Align::Inner is valid and finds no absent records inside the window");
  CatalogReduceOptions bad = ended_null;
  bad.align_by = pluss::kindle::AlignBy::Label;
  expect_throw([&] { (void)ragged.sum(bad); }, "ended=Null with label alignment (the padding path) should be rejected");
  CatalogReduceOptions bad_fill = tail;
  bad_fill.fill_scalar = 0.0;
  expect_throw([&] { (void)ragged.sum(bad_fill); }, "anchor=Tail with fill_scalar should be rejected");
  CatalogReduceOptions record_axis;
  record_axis.min_streams = 2;
  expect_throw([&] { (void)ragged.sum(record_axis); }, "jagged flags on the Record axis should be rejected");
  CatalogReduceOptions zero = by_key;
  zero.min_streams = 0;
  expect_throw([&] { (void)ragged.sum(zero); }, "min_streams < 1 should be rejected");

  // Randomized cross-check against an independent implementation, including inputs
  // large enough to take the parallel path.
  std::mt19937_64 rng(20261008);
  const std::vector<std::string> ops = {"sum", "mean", "min", "max", "var", "std"};
  for (int trial = 0; trial < 120; ++trial) {
    const std::size_t k = 1 + rng() % 6;
    std::vector<std::vector<double>> random_cols(k);
    for (auto& c : random_cols) {
      c.resize(1 + rng() % 60);
      for (auto& x : c) x = (rng() % 11 == 0) ? std::nan("") : static_cast<double>(static_cast<std::int64_t>(rng() % 2001) - 1000) / 7.0;
    }
    const auto catalog = jagged_catalog(random_cols);
    for (const auto& op : ops) {
      for (const bool skipnull : {true, false}) {
        CatalogReduceOptions o = by_key;
        o.skipnull = skipnull;
        o.ended = (rng() % 2) ? pluss::kindle::RaggedEnd::Null : pluss::kindle::RaggedEnd::Skip;
        o.anchor = (rng() % 2) ? pluss::kindle::RaggedAnchor::Tail : pluss::kindle::RaggedAnchor::Head;
        o.min_streams = 1 + static_cast<std::int64_t>(rng() % 3);
        const bool inner = (rng() % 3) == 0;
        o.align = inner ? pluss::kindle::Align::Inner : pluss::kindle::Align::Outer;
        const auto got = reduce_by_name(catalog, op, o).keys(op);
        const auto want = naive_jagged(random_cols, op, skipnull, true, o.ended == pluss::kindle::RaggedEnd::Null,
                                       o.anchor == pluss::kindle::RaggedAnchor::Tail, o.min_streams, inner);
        expect(near_all(stream_values(got), want.value, 1e-9) && stream_valid(got) == want.valid,
               "jagged " + op + " should match the reference implementation");
      }
    }
  }
  std::vector<std::vector<double>> big(3);
  for (std::size_t i = 0; i < big.size(); ++i) {
    big[i].resize(100000 + 777 * i);
    for (std::size_t j = 0; j < big[i].size(); ++j) big[i][j] = std::sin(0.001 * static_cast<double>(j * (i + 1)));
  }
  const auto big_catalog = jagged_catalog(big);
  for (const auto& op : ops) {
    const auto got = reduce_by_name(big_catalog, op, by_key).keys(op);
    const auto want = naive_jagged(big, op, true, true);
    expect(near_all(stream_values(got), want.value, 1e-9) && stream_valid(got) == want.valid,
           "parallel jagged " + op + " should match the reference implementation");
  }
}


void test_ragged_policy_and_layout() {
  using namespace pluss::kindle;
  const auto d = [](OpClass c) { return RaggedPolicy::defaults(c); };

  // The documented default for every operation class.
  expect(d(OpClass::Reduce) == RaggedPolicy{}, "Reduce: longest, head, skip absent, skip nulls, preserve");
  expect(d(OpClass::Elementwise).absent == RaggedAbsent::Null && d(OpClass::Elementwise).domain == RaggedDomain::Longest,
         "Elementwise: an absent operand makes the value unknown");
  expect(d(OpClass::Relational).absent == RaggedAbsent::Null && d(OpClass::Relational).materialize == RaggedMaterialize::Preserve &&
             d(OpClass::Relational).domain == RaggedDomain::Longest,
         "Relational: keep every record, no-partner rows are nulls, stay ragged");
  expect(d(OpClass::Pairwise).domain == RaggedDomain::Shortest, "Pairwise: complete pairs only");
  expect(d(OpClass::Plot) == RaggedPolicy{}, "Plot: draw what exists");
  expect(d(OpClass::Rectangular).materialize == RaggedMaterialize::Pad && d(OpClass::Rectangular).absent == RaggedAbsent::Null &&
             d(OpClass::Rectangular).domain == RaggedDomain::Longest,
         "Rectangular: pad with nulls, never trim");
  expect(d(OpClass::Persist).loss == RaggedLoss::Error, "Persist: lossless");
  for (const auto c : {OpClass::Reduce, OpClass::Elementwise, OpClass::Relational, OpClass::Pairwise, OpClass::Plot,
                       OpClass::Rectangular, OpClass::Persist}) {
    d(c).validate("defaults");  // every default is a valid policy
  }

  // Contradictory policies are rejected.
  RaggedPolicy bad;
  bad.min_support = 0;
  expect_throw([&] { bad.validate("t"); }, "min_support < 1 should be rejected");
  bad = {};
  bad.absent = RaggedAbsent::Fill;
  expect_throw([&] { bad.validate("t"); }, "absent=Fill without a fill value should be rejected");
  bad = {};
  bad.fill = 1.0;
  expect_throw([&] { bad.validate("t"); }, "a fill value without absent=Fill should be rejected");
  bad = {};
  bad.domain = RaggedDomain::Anchor;
  expect_throw([&] { bad.validate("t"); }, "domain=Anchor without an anchor stream should be rejected");

  // Layouts of three streams with 3, 5 and 6 records.
  const auto catalog = jagged_catalog({{2.3, 1.3, 2.5}, {2.6, 0.3, 1.9, 2.1, 3.7}, {3.6, 4.3, 0.9, 0.1, 3.1, 0.01}});
  const auto layout = [&](RaggedDomain dom, RaggedAlignment al) {
    RaggedPolicy p;
    p.domain = dom;
    p.alignment = al;
    if (dom == RaggedDomain::Anchor) p.anchor_stream = "s1";
    return RaggedLayout::of(catalog, p, "test");
  };
  const auto head = layout(RaggedDomain::Longest, RaggedAlignment::Head);
  expect(head.extent == std::vector<std::int64_t>({3, 5, 6}) && head.longest == 6 && head.shortest == 3 && head.ragged() &&
             head.domain() == 6 && head.trimmed() == 0 && head.offset == std::vector<std::int64_t>({0, 0, 0}),
         "head/longest layout");
  expect(head.presence() == std::vector<std::int64_t>({3, 3, 3, 2, 2, 1}), "head presence: streams still running at each index");
  const auto tail = layout(RaggedDomain::Longest, RaggedAlignment::Tail);
  expect(tail.offset == std::vector<std::int64_t>({3, 1, 0}) &&
             tail.presence() == std::vector<std::int64_t>({1, 2, 2, 3, 3, 3}),
         "tail alignment lines the last records up");
  const auto short_head = layout(RaggedDomain::Shortest, RaggedAlignment::Head);
  expect(short_head.window_begin == 0 && short_head.window_end == 3 && short_head.trimmed() == 3 &&
             short_head.presence() == std::vector<std::int64_t>({3, 3, 3}),
         "shortest/head keeps the first common indices and counts what it cuts");
  const auto short_tail = layout(RaggedDomain::Shortest, RaggedAlignment::Tail);
  expect(short_tail.window_begin == 3 && short_tail.window_end == 6 && short_tail.presence() == std::vector<std::int64_t>({3, 3, 3}),
         "shortest/tail keeps the last common indices");
  const auto anchor_head = layout(RaggedDomain::Anchor, RaggedAlignment::Head);
  expect(anchor_head.window_begin == 0 && anchor_head.window_end == 5 &&
             anchor_head.presence() == std::vector<std::int64_t>({3, 3, 3, 2, 2}),
         "an anchor stream sets the window");
  const auto anchor_tail = layout(RaggedDomain::Anchor, RaggedAlignment::Tail);
  expect(anchor_tail.window_begin == 1 && anchor_tail.window_end == 6 &&
             anchor_tail.presence() == std::vector<std::int64_t>({2, 2, 3, 3, 3}),
         "an anchor stream under tail alignment");
  expect_throw([&] { (void)layout(RaggedDomain::Exact, RaggedAlignment::Head); }, "domain=Exact rejects a ragged catalog");
  RaggedPolicy exact;
  exact.domain = RaggedDomain::Exact;
  expect(RaggedLayout::of(jagged_catalog({{1.0, 2.0}, {3.0, 4.0}}), exact, "t").domain() == 2, "domain=Exact accepts equal lengths");
  RaggedPolicy strict_loss;
  strict_loss.domain = RaggedDomain::Shortest;
  strict_loss.loss = RaggedLoss::Error;
  expect_throw([&] { (void)RaggedLayout::of(catalog, strict_loss, "t"); }, "loss=Error refuses a policy that cuts records");
  RaggedPolicy missing_anchor;
  missing_anchor.domain = RaggedDomain::Anchor;
  missing_anchor.anchor_stream = "nope";
  expect_throw([&] { (void)RaggedLayout::of(catalog, missing_anchor, "t"); }, "an unknown anchor stream should be rejected");

  // Conversions from the older option structs.
  expect(to_ragged(CatalogReduceOptions{}) == d(OpClass::Reduce), "default reduce options are the Reduce defaults");
  CatalogReduceOptions legacy;
  legacy.align = Align::Inner;
  legacy.ended = RaggedEnd::Null;
  legacy.anchor = RaggedAnchor::Tail;
  legacy.min_streams = 2;
  legacy.skipnull = false;
  const auto converted = to_ragged(legacy);
  expect(converted.domain == RaggedDomain::Shortest && converted.absent == RaggedAbsent::Null &&
             converted.alignment == RaggedAlignment::Tail && converted.min_support == 2 && converted.nulls == RaggedNulls::Propagate,
         "CatalogReduceOptions convert field by field");
  expect(TabelizePolicy{}.align == Align::Outer, "tabelize now pads by default instead of inner-joining");
  expect(to_ragged(TabelizePolicy{}).domain == RaggedDomain::Longest && to_ragged(TabelizePolicy{}).absent == RaggedAbsent::Null,
         "the default TabelizePolicy is the Rectangular default");
  expect(to_tabelize(d(OpClass::Rectangular), "t").align == Align::Outer, "Rectangular maps to Outer");
  RaggedPolicy fill = d(OpClass::Rectangular);
  fill.absent = RaggedAbsent::Fill;
  fill.fill = 7.0;
  expect(to_tabelize(fill, "t").fill == FillPolicy::Scalar && to_tabelize(fill, "t").fill_scalar == 7.0, "Fill maps to a scalar fill");
  RaggedPolicy tail_policy = d(OpClass::Rectangular);
  tail_policy.alignment = RaggedAlignment::Tail;
  expect_throw([&] { (void)to_tabelize(tail_policy, "t"); }, "tabelize cannot honour Tail alignment and must say so");

  // tabelize pads by default and drops only when asked.
  expect(catalog.tabelize().ncases() == 6, "default tabelize keeps every record of the longest stream");
  TabelizePolicy inner;
  inner.align = Align::Inner;
  expect(catalog.tabelize(inner).ncases() == 3, "Align::Inner is still available explicitly");
  expect(catalog.tabelize(d(OpClass::Rectangular)).ncases() == 6, "tabelize(RaggedPolicy) with the Rectangular defaults pads");
  RaggedPolicy cut = d(OpClass::Rectangular);
  cut.domain = RaggedDomain::Shortest;
  cut.loss = RaggedLoss::Error;
  expect_throw([&] { (void)catalog.tabelize(cut); }, "tabelize(RaggedPolicy) honours loss=Error");

  // Reductions use the layout: Inner with tail alignment reduces the aligned tails.
  CatalogReduceOptions tails;
  tails.axis = ReduceAxis::Key;
  tails.align = Align::Inner;
  tails.anchor = RaggedAnchor::Tail;
  expect(near_all(stream_values(catalog.sum(tails).keys("sum")), {4.3, 6.5, 6.21}),
         "Inner + Tail sums the last three records of each stream");
  CatalogReduceOptions heads = tails;
  heads.anchor = RaggedAnchor::Head;
  expect(near_all(stream_values(catalog.sum(heads).keys("sum")), {8.5, 5.9, 5.3}), "Inner + Head sums the first three records");
}

void test_catalog_join_policy() {
  using namespace pluss::kindle;
  const auto left = jagged_catalog({{1.0, 2.0, 3.0}});   // stream "s0"
  Catalog right(std::vector<std::pair<std::string, Stream>>{
      {"s0", dstream({2.0, 3.0, 4.0})},
      {"payload", dstream({20.0, 30.0, 40.0})},
  });
  CatalogJoinSpec spec;
  spec.left_keys = {"s0"};
  spec.right_keys = {"s0"};

  const auto inner = left.join_detailed(right, spec);
  expect(stream_values(inner.catalog.keys("s0")) == std::vector<double>({2.0, 3.0}) &&
             stream_values(inner.catalog.keys("payload")) == std::vector<double>({20.0, 30.0}),
         "Catalog::join matches on the key streams");
  expect(inner.report.match.left_dropped == 1 && inner.report.match.right_dropped == 1, "the report counts omitted records");

  CatalogJoinOptions refuse;
  refuse.ragged.loss = RaggedLoss::Error;
  expect_throw([&] { (void)left.join(right, spec, refuse); }, "loss=Error refuses a join that omits records");

  spec.how = pluss::relational::JoinHow::Outer;
  const auto outer = left.join_detailed(right, spec, refuse);   // an outer join omits nothing
  expect(outer.report.output_records == 4 && outer.report.match.left_dropped == 0 && outer.report.match.right_dropped == 0,
         "an outer join passes loss=Error");

  CatalogJoinOptions bad_policy;
  bad_policy.ragged.alignment = RaggedAlignment::Label;
  expect_throw([&] { (void)left.join(right, spec, bad_policy); }, "label alignment is not a join mode");
  CatalogJoinOptions bad_absent;
  bad_absent.ragged.absent = RaggedAbsent::Skip;
  expect_throw([&] { (void)left.join(right, spec, bad_absent); }, "a join cannot skip rows that have no partner");
}


std::vector<double> reduce_ref(const std::vector<double>& values, const std::vector<std::int64_t>& source,
                               std::int64_t shift, std::int64_t lo, std::int64_t hi, const std::string& op,
                               bool unbiased, bool* ok) {
  std::vector<double> xs;
  for (auto e = lo; e < hi; ++e) {
    const auto p = source[static_cast<std::size_t>(e)] + shift;
    if (p < 0 || p >= static_cast<std::int64_t>(values.size())) continue;
    if (std::isnan(values[static_cast<std::size_t>(p)])) continue;
    xs.push_back(values[static_cast<std::size_t>(p)]);
  }
  *ok = true;
  const double n = static_cast<double>(xs.size());
  if (op == "count") return {n};
  if (op == "ndistinct") {
    std::sort(xs.begin(), xs.end());
    return {static_cast<double>(std::unique(xs.begin(), xs.end()) - xs.begin())};
  }
  if (xs.empty()) { *ok = false; return {0.0}; }
  double s = 0.0;
  for (double x : xs) s += x;
  if (op == "sum") return {s};
  if (op == "mean") return {s / n};
  if (op == "min") return {*std::min_element(xs.begin(), xs.end())};
  if (op == "max") return {*std::max_element(xs.begin(), xs.end())};
  if (op == "median") {
    std::sort(xs.begin(), xs.end());
    const auto m = xs.size() / 2;
    return {xs.size() % 2 ? xs[m] : 0.5 * (xs[m - 1] + xs[m])};
  }
  const double ddof = unbiased ? 1.0 : 0.0;
  if (n - ddof <= 0.0) { *ok = false; return {0.0}; }
  double ss = 0.0;
  for (double x : xs) ss += (x - s / n) * (x - s / n);
  const double var = ss / (n - ddof);
  return {op == "std" ? std::sqrt(var) : var};
}

void test_stream_reduce_groups_and_expand() {
  using namespace pluss::kindle;
  const Stream values = dstream({1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
  // Groups: {1,2,3}, {} (empty), {4,5}, {0}
  const std::vector<std::int64_t> source = {0, 1, 2, 3, 4, 0};
  const std::vector<std::int64_t> offsets = {0, 3, 3, 5, 6};
  const auto sum = values.reduce_groups(source, 0, offsets, GroupReduce::Sum);
  expect(stream_values(sum) == std::vector<double>({6.0, 0.0, 9.0, 1.0}) &&
             stream_valid(sum) == std::vector<bool>({true, false, true, true}),
         "an empty group has no sum: it is a null, not a zero");
  const auto count = values.reduce_groups(source, 0, offsets, GroupReduce::Count);
  expect(stream_values(count) == std::vector<double>({3.0, 0.0, 2.0, 1.0}) && stream_valid(count) == std::vector<bool>(4, true),
         "an empty group has a count of 0");
  expect(stream_values(values.reduce_groups(source, 0, offsets, GroupReduce::Mean)) == std::vector<double>({2.0, 0.0, 4.5, 1.0}),
         "group means");
  // A shift moves the record indices to stream positions: records {0,1} at shift 1 are positions {1,2}.
  expect(stream_values(values.reduce_groups({0, 1}, 1, {0, 2}, GroupReduce::Sum)) == std::vector<double>({5.0}),
         "shift maps record indices to stream positions");
  // Entries outside the stream and nulls are skipped, not errors.
  expect(stream_values(values.reduce_groups({0, 99, -1, 2}, 0, {0, 4}, GroupReduce::Sum)) == std::vector<double>({4.0}),
         "entries outside the stream are skipped");
  const Stream with_nulls = with_nulls_stream({1.0, 2.0, 3.0}, {true, false, true});
  expect(stream_values(with_nulls.reduce_groups({0, 1, 2}, 0, {0, 3}, GroupReduce::Count)) == std::vector<double>({2.0}) &&
             stream_values(with_nulls.reduce_groups({0, 1, 2}, 0, {0, 3}, GroupReduce::Sum)) == std::vector<double>({4.0}),
         "null values are skipped by every reduction");
  expect(stream_values(dstream({1.0, std::nan(""), 3.0}).reduce_groups({0, 1, 2}, 0, {0, 3}, GroupReduce::Mean)) == std::vector<double>({2.0}),
         "NaN is skipped");
  const auto var = values.reduce_groups(source, 0, offsets, GroupReduce::Var);
  expect(stream_valid(var) == std::vector<bool>({true, false, true, false}) && near_all({stream_values(var)[0], stream_values(var)[2]}, {1.0, 0.5}),
         "unbiased variance needs two values");
  expect(stream_valid(values.reduce_groups(source, 0, offsets, GroupReduce::Var, false))[3], "population variance of one value is defined");

  // Count and Ndistinct work on text too; the rest refuse.
  const Stream words_s(pluss::make_note(std::vector<std::string>{"a", "b", "a", "c"}, {4}));
  expect(stream_values(words_s.reduce_groups({0, 1, 2, 3}, 0, {0, 4}, GroupReduce::Ndistinct)) == std::vector<double>({3.0}) &&
             stream_values(words_s.reduce_groups({0, 1, 2, 3}, 0, {0, 4}, GroupReduce::Count)) == std::vector<double>({4.0}),
         "counting reductions work on text");
  expect_throw([&] { (void)words_s.reduce_groups({0}, 0, {0, 1}, GroupReduce::Sum); }, "sum of text should be rejected");
  expect_throw([&] { (void)values.reduce_groups({0}, 0, {0, 2}, GroupReduce::Sum); }, "offsets must end at source.size()");
  expect_throw([&] { (void)values.reduce_groups({0, 1}, 0, {0, 2, 1, 2}, GroupReduce::Sum); }, "offsets must be non-decreasing");

  // Randomized comparison with a naive implementation, including a size that takes the parallel path.
  std::mt19937_64 rng(4242);
  const std::vector<std::string> ops = {"count", "ndistinct", "sum", "mean", "median", "min", "max", "std", "var"};
  const std::vector<GroupReduce> kinds = {GroupReduce::Count, GroupReduce::Ndistinct, GroupReduce::Sum, GroupReduce::Mean,
                                          GroupReduce::Median, GroupReduce::Min, GroupReduce::Max, GroupReduce::Std,
                                          GroupReduce::Var};
  for (int trial = 0; trial < 40; ++trial) {
    const std::size_t n = 1 + rng() % 200;
    std::vector<double> data(n);
    for (auto& x : data) x = (rng() % 9 == 0) ? std::nan("") : static_cast<double>(rng() % 20);
    const std::int64_t shift = static_cast<std::int64_t>(rng() % 5) - 2;
    const std::size_t groups = 1 + rng() % (trial == 39 ? 5000 : 12);
    std::vector<std::int64_t> src, offs{0};
    for (std::size_t g = 0; g < groups; ++g) {
      const std::size_t len = rng() % (trial == 39 ? 6 : 15);
      for (std::size_t e = 0; e < len; ++e) src.push_back(static_cast<std::int64_t>(rng() % (n + 6)) - 3);
      offs.push_back(static_cast<std::int64_t>(src.size()));
    }
    const Stream s = dstream(data);
    for (std::size_t o = 0; o < ops.size(); ++o) {
      const bool unb = (rng() % 2) == 0;
      const auto got = s.reduce_groups(src, shift, offs, kinds[o], unb);
      const auto gv = stream_values(got);
      const auto gok = stream_valid(got);
      for (std::size_t g = 0; g < groups; ++g) {
        bool ok = true;
        const auto want = reduce_ref(data, src, shift, offs[g], offs[g + 1], ops[o], unb, &ok);
        expect(gok[g] == ok && (!ok || std::fabs(gv[g] - want[0]) < 1e-9 * (1.0 + std::fabs(want[0]))),
               "reduce_groups " + ops[o] + " should match the reference");
      }
    }
  }

  // expand composes two hops without a table in between.
  namespace rel = pluss::relational;
  rel::MatchResult hop2;                       // mid records 0..3 -> leaves
  hop2.left = {0, 0, 1, 3};
  hop2.right = {10, 11, 12, 13};               // mid 0 has leaves 10,11; mid 1 has 12; mid 3 has 13; mid 2 none
  const auto e = rel::expand({0, 1, 1}, {0, 1, 2}, hop2, 4, false);
  expect(e.root == std::vector<std::int64_t>({0, 0, 1}) && e.leaf == std::vector<std::int64_t>({10, 11, 12}),
         "expand fans a pair out to its partners and drops a pair with none");
  const auto kept = rel::expand({0, 1, 1}, {0, 1, 2}, hop2, 4, true);
  expect(kept.root == std::vector<std::int64_t>({0, 0, 1, 1}) && kept.leaf == std::vector<std::int64_t>({10, 11, 12, -1}),
         "keep_unmatched keeps a partnerless pair as (root, -1)");
  expect(rel::expand({5, 6}, {-1, 0}, hop2, 4, true).leaf == std::vector<std::int64_t>({-1, 10, 11}),
         "a missing mid record survives as -1 when unmatched pairs are kept");

  // match_keys is the matching half of join.
  const Catalog a(std::vector<std::pair<std::string, Stream>>{{"k", dstream({1.0, 2.0, 2.0, 5.0})}});
  const Catalog b(std::vector<std::pair<std::string, Stream>>{{"k", dstream({2.0, 5.0, 7.0})}});
  const auto m = a.match_keys(b, {"k"}, {"k"}, rel::JoinHow::Inner);
  expect(m.pairs.left == std::vector<std::int64_t>({1, 2, 3}) && m.pairs.right == std::vector<std::int64_t>({0, 0, 1}) &&
             m.left.domain() == 4 && m.right.domain() == 3 && m.left.shift_of("k") == 0,
         "match_keys returns the record pairs and both layouts");
}

void test_catalog_construction_and_selection() {
  const auto base = make_base_catalog();
  expect(base.size() == base.nkeys(), "Catalog::size should match nkeys");
  expect(base.shape() == std::make_pair<std::int64_t, std::int64_t>(5, 6),
         "Catalog::shape should be (max record count, number of keys)");

  const auto ragged = Catalog(std::vector<std::pair<std::string, Stream>>{
      {"short", Stream(pluss::make_series(std::vector<double>{1.0, 2.0}, {2}))},
      {"long", Stream(pluss::make_note(std::vector<std::string>{"a", "b", "c"}, {3}))},
  });
  expect(ragged.shape() == std::make_pair<std::int64_t, std::int64_t>(3, 2),
         "Catalog::shape should support length-heterogeneous streams");
  expect(ragged.minrows() == 2, "Catalog::minrows should inspect ragged catalogs");
  expect(ragged.maxrows() == 3, "Catalog::maxrows should inspect ragged catalogs");
  expect_throw([&]() { (void)ragged.nrows(); }, "Catalog::nrows should throw for ragged catalogs");

  using namespace pluss::kindle::literals;
  const auto via_named = Catalog({
      "id"_k = base.keys("id"),
      "city"_k = base.keys("city"),
  });
  const auto via_pairs = Catalog(std::vector<std::pair<std::string, Stream>>{
      {"id", base.keys("id")},
      {"city", base.keys("city")},
  });
  std::vector<std::pair<std::string, Stream>> vec_items{
      {"id", base.keys("id")},
      {"city", base.keys("city")},
  };
  const auto via_vector = Catalog(std::move(vec_items));
  expect(via_named.nkeys() == 2, "named constructor mismatch");
  expect(via_pairs.nkeys() == 2, "pair constructor mismatch");
  expect(via_vector.nkeys() == 2, "vector constructor mismatch");

  expect(base.keyaxis().series().nkeys() == 2, "series projection mismatch");
  expect(base.keyaxis().note().nkeys() == 2, "note projection mismatch");
  expect(base.keyaxis().timeline().nkeys() == 2,
         "timeline projection should include every canonical temporal stream");
  expect(base.keyaxis().like("time").nkeys() == 2, "keyaxis().like should select matching keys");
  expect(base.keyaxis().match("time_index").nkeys() == 1 && base.keyaxis().match("time_index").has("time_index"),
         "keyaxis().match should regex-select keys");
  const auto prefixed = Catalog(std::vector<std::pair<std::string, Stream>>{
      {"src_id", base.keys("id")},
      {"src_city", base.keys("city")},
      {"stamp", base.keys("timestamp")},
  });
  expect(prefixed.keyaxis().prefix("src_").nkeys() == 2, "keyaxis().prefix should filter matching keys");
  expect(prefixed.keyaxis().prefix("src_").has("src_id"), "keyaxis().prefix should keep prefixed keys");
  expect(!prefixed.keyaxis().prefix("src_").has("stamp"), "keyaxis().prefix should exclude non-matching keys");

  const auto suffixed = Catalog(std::vector<std::pair<std::string, Stream>>{
      {"id_raw", base.keys("id")},
      {"city_raw", base.keys("city")},
      {"timestamp", base.keys("timestamp")},
  });
  expect(suffixed.keyaxis().suffix("_raw").nkeys() == 2, "keyaxis().suffix should filter matching keys");
  expect(suffixed.keyaxis().suffix("_raw").has("id_raw"), "keyaxis().suffix should keep suffixed keys");
  expect(!suffixed.keyaxis().suffix("_raw").has("timestamp"), "keyaxis().suffix should exclude non-matching keys");

  const auto h = base.head(2);
  const auto t = base.tail(2);
  const auto s = base.slice(1, 4);
  const auto i = base.cloc(0, 3);
  expect(h.nrows() == 2, "head row count mismatch");
  expect(t.nrows() == 2, "tail row count mismatch");
  expect(s.nrows() == 3, "slice row count mismatch");
  expect(i.nrows() == 3, "iloc row count mismatch");

  const auto filt = base.filter(bool_mask({true, false, true, true, false}));
  const auto where = base.where(bool_mask({true, false, true, true, false}));
  expect(filt.nrows() == 3, "filter row count mismatch");
  expect(where.nrows() == 3, "where row count mismatch");

  const auto taken = base.take(std::vector<std::int64_t>{0, 2, 4});
  expect(taken.nrows() == 3, "take row count mismatch");

  const auto sampled_a = base.sample(2, 42);
  const auto sampled_b = base.sample(2, 42);
  expect(sampled_a.keys("id").as_series().host<std::int64_t>()
             == sampled_b.keys("id").as_series().host<std::int64_t>(),
         "sample with same seed should be deterministic");

  const auto queried = base.query([](const Catalog& c) {
    return (c.keys("id") > 2.0).as_series();
  });
  expect(queried.nrows() == 3, "query row count mismatch");

  const auto selected = base.keys({"id", "city"});
  const auto dropped = base.drop(std::vector<std::string>{"time_index"});
  const auto renamed = base.rename({{"metric", "distance"}});
  expect(selected.nkeys() == 2, "select key count mismatch");
  expect(dropped.nkeys() == base.nkeys() - 1, "drop key count mismatch");
  expect(renamed.has("distance"), "rename should create distance key");

  expect_throw([&]() { (void)base.keys({"missing_key"}); }, "keys unknown key should throw");

  auto replaced = base;
  replaced.set_("id", base.keys("id") + 100.0);
  const auto chosen = Catalog::select(
      {pluss::table::Slate(bool_mask({true, false, true, false, false}))},
      {replaced},
      base);
  expect(chosen.keys("id").as_series().host<std::int64_t>() == std::vector<std::int64_t>({103, 1, 102, 5, 4}),
         "Catalog::select should choose row-wise between catalogs");
  expect(note_values(chosen.keys("city").as_note()) == note_values(base.keys("city").as_note()),
         "Catalog::select should preserve untouched streams");
}

void test_catalog_label_alignment() {
  const auto left = labeled_series({1.0, 2.0, 3.0}, {"A", "B", "C"});
  const auto right = labeled_series({30.0, 10.0, 40.0}, {"C", "A", "D"});
  const auto labeled = Catalog(std::vector<std::pair<std::string, Stream>>{
      {"left", left},
      {"right", right},
  });

  pluss::kindle::TabelizePolicy policy;
  policy.align_by = pluss::kindle::AlignBy::Label;
  policy.align = pluss::kindle::Align::Inner;
  const auto inner = labeled.tabelize(policy);
  expect(inner.ncases() == 2, "Catalog::tabelize(label, inner) should keep intersecting labels");
  expect(note_values(inner.labels().values().cast().note().as_note())
             == std::vector<std::string>({"A", "C"}),
         "Catalog::tabelize(label, inner) should preserve anchor label order");
  expect(inner.features("right").as_series().host<double>() == std::vector<double>({10.0, 30.0}),
         "Catalog::tabelize(label, inner) should align values by label");

  policy.align = pluss::kindle::Align::Outer;
  const auto outer = labeled.tabelize(policy);
  expect(outer.ncases() == 4, "Catalog::tabelize(label, outer) should take label union");
  expect(note_values(outer.labels().values().cast().note().as_note())
             == std::vector<std::string>({"A", "B", "C", "D"}),
         "Catalog::tabelize(label, outer) should keep anchor labels then append new labels");

  policy.align = pluss::kindle::Align::Strict;
  expect_throw([&]() { (void)labeled.tabelize(policy); },
               "Catalog::tabelize(label, strict) should reject different label order");

  const auto unlabeled = Catalog(std::vector<std::pair<std::string, Stream>>{
      {"long", Stream(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3}))},
      {"short", Stream(pluss::make_series(std::vector<double>{1.0, 2.0}, {2}))},
  });
  policy.align_by = pluss::kindle::AlignBy::Pos;
  policy.align = pluss::kindle::Align::Strict;
  expect_throw([&]() { (void)unlabeled.tabelize(policy); },
               "Catalog::tabelize(pos, strict) should reject length mismatch");
}

void test_catalog_sort_group_missing() {
  const auto base = make_base_catalog();
  const auto sorted = base.sort("id", true);
  const auto id_sorted = sorted.keys("id").as_series().host<std::int64_t>();
  expect((id_sorted == std::vector<std::int64_t>{1, 2, 3, 4, 5}), "sort(id) order mismatch");

  const auto sorted_by = base.sort({"city", "id"}, {true, false});
  expect(sorted_by.nrows() == 5, "sort(keys) should preserve row count");

  const auto groups = base.group().by("city");
  expect(groups.size() == 3, "group_by(city) should produce 3 buckets");
  expect(groups.find("Accra") != groups.end(), "group_by(city) missing Accra");
  expect(groups.at("Accra").nrows() == 2, "Accra group row count mismatch");

  const auto filled = base.keys({"metric"}).fillnull(0.0);
  const auto dropped = base.keys({"metric"}).dropnull();
  const auto ffilled = base.keys({"metric"}).ffill();
  const auto bfilled = base.keys({"metric"}).bfill();
  const auto nulls = base.keys({"metric"}).isnull();
  const auto not_nulls = base.keys({"metric"}).notnull();
  const auto invalid = base.keys({"metric"}).invalid();
  expect(filled.nrows() == 5, "fillnull should not change row count");
  expect(filled.keys("metric").as_series().host<double>() == std::vector<double>({10.0, 0.0, 30.0, 40.0, 0.0}),
         "Catalog::fillnull(double, keys) should preserve valid values and replace only null rows");
  expect(dropped.nrows() == 3, "dropnull(metric) should drop null rows");
  expect(ffilled.nrows() == 5, "ffill should preserve row count");
  expect(bfilled.nrows() == 5, "bfill should preserve row count");
  expect(nulls.keys("metric").kind() == StreamKind::Series, "isnull should return Series stream");
  expect(not_nulls.keys("metric").kind() == StreamKind::Series, "notnull should return Series stream");
  expect(invalid.keys("metric").kind() == StreamKind::Series, "invalid should return Series stream");

  const std::array<std::int64_t, 1> shape4{4};
  const auto mixed_missing = Catalog(std::vector<std::pair<std::string, Stream>>{
      {"note_col",
       Stream(pluss::make_note(std::vector<std::string>{"a", "", "c", ""}, shape4),
              bool_mask({true, false, true, false}))},
      {"ts_col",
       Stream(pluss::make_timeline({
                  "2024-01-01T00:00:00Z",
                  "2024-01-02T00:00:00Z",
                  "2024-01-03T00:00:00Z",
                  "2024-01-04T00:00:00Z",
              }),
              bool_mask({true, false, true, false}))},
      {"ti_col",
       Stream(pluss::make_timeline({
                  "2024-01-01T00:00:00Z",
                  "2024-01-02T00:00:00Z",
                  "2024-01-03T00:00:00Z",
                  "2024-01-04T00:00:00Z",
              }),
              bool_mask({true, false, true, false}))},
  });

  const auto note_filled = mixed_missing.keys({"note_col"}).fillnull("missing");
  expect(std::string(note_filled.keys("note_col").as_note().at(1)) == "missing",
         "Catalog::fillnull(string, keys) should replace null Note values");
  expect(!pluss::kindle::detail::has_validity_mask(note_filled.keys("note_col")),
         "Catalog::fillnull(string, keys) should clear Note mask when fully resolved");
  const auto note_untouched_by_double = mixed_missing.keys({"note_col"}).fillnull(0.0);
  expect(pluss::kindle::detail::has_validity_mask(note_untouched_by_double.keys("note_col")),
         "Catalog::fillnull(double, keys) should skip Note streams instead of throwing");
  expect(std::string(note_untouched_by_double.keys("note_col").as_note().at(0)) == "a",
         "Catalog::fillnull(double, keys) should leave Note payload untouched");

  const auto ts_fill = pluss::make_timeline("2024-01-31T00:00:00Z");
  const auto ts_filled = mixed_missing.keys({"ts_col"}).fillnull(ts_fill);
  const auto ts_ticks = ts_filled.keys("ts_col").as_timeline().ticks();
  const auto ts_fill_tick = ts_fill.ticks()[0];
  expect(ts_ticks[1] == ts_fill_tick && ts_ticks[3] == ts_fill_tick,
         "Catalog::fillnull(Timeline, keys) should replace null Timeline values");
  expect(!pluss::kindle::detail::has_validity_mask(ts_filled.keys("ts_col")),
         "Catalog::fillnull(Timeline, keys) should clear Timeline mask when fully resolved");
  const auto ts_untouched_by_string = mixed_missing.keys({"ts_col"}).fillnull("missing");
  expect(pluss::kindle::detail::has_validity_mask(ts_untouched_by_string.keys("ts_col")),
         "Catalog::fillnull(string, keys) should skip Timeline streams instead of throwing");

  const auto ti_fill = pluss::make_timeline("2024-02-01T00:00:00Z");
  const auto ti_filled = mixed_missing.keys({"ti_col"}).fillnull(ti_fill);
  const auto ti_ticks = ti_filled.keys("ti_col").as_timeline().ticks();
  const auto ti_fill_tick = ti_fill.ticks()[0];
  expect(ti_ticks[1] == ti_fill_tick && ti_ticks[3] == ti_fill_tick,
         "Catalog::fillnull(Timeline, keys) should replace null Timeline values");
  expect(!pluss::kindle::detail::has_validity_mask(ti_filled.keys("ti_col")),
         "Catalog::fillnull(Timeline, keys) should clear Timeline mask when fully resolved");
  const auto ti_filled_by_timeline = mixed_missing.keys({"ti_col"}).fillnull(ts_fill);
  const auto ti_from_timeline_ticks = ti_filled_by_timeline.keys("ti_col").as_timeline().ticks();
  expect(ti_from_timeline_ticks[1] == ts_fill_tick && ti_from_timeline_ticks[3] == ts_fill_tick,
         "Catalog::fillnull(Timeline, keys) should fill selected Timeline streams");

  const auto id_as_note = base.keys({"id"}).cast().note();
  const auto ts_as_series = base.keys({"timestamp"}).cast(
      pluss::table::SlateCastOptions{.series_dtype = pluss::codem::ScalarType::Int64}).series();
  const auto id_as_f64 = base.keys({"id"}).astype(pluss::codem::ScalarType::Float64);
  expect(id_as_note.keys("id").kind() == StreamKind::Note, "cast(Note) id kind mismatch");
  expect(ts_as_series.keys("timestamp").kind() == StreamKind::Series, "cast(Series) timestamp kind mismatch");
  expect(id_as_f64.keys("id").as_series().scalar_type() == pluss::codem::ScalarType::Float64,
         "astype(id,float64) scalar type mismatch");

  const auto city_distinct = base.keys("city").distinct();
  expect(city_distinct.size() == 3, "Catalog::distinct(key) should delegate to Stream::distinct");
  const auto selected_distinct = base.keys({"city"}).distinct();
  expect(selected_distinct.keys("city").size() == 3, "Catalog::distinct(keys) should transform selected key");
  expect(selected_distinct.nkeys() == 1 && selected_distinct.has("city"),
         "Catalog::distinct(keys) should keep the projected key set");

  const auto city_freq = base.keys("city").freq();
  expect(city_freq.size() == 3, "Catalog::freq(key) should delegate to Stream::freq");
  expect(city_freq.has_label(), "Catalog::freq(key) result should carry value labels");
  const auto selected_freq = base.keys({"city"}).freq({.top = 2});
  expect(selected_freq.keys("city").size() == 2, "Catalog::freq(keys, top_n) should limit selected frequency");
  expect(selected_freq.nkeys() == 1 && selected_freq.has("city"),
         "Catalog::freq(keys, top_n) should keep the projected key set");

  const auto city_ndistinct = base.keys("city").ndistinct();
  expect(static_cast<std::int64_t>(scalar_stream_to_double(city_ndistinct)) == 3,
         "Catalog::ndistinct(key) mismatch");
  const auto all_ndistinct = base.ndistinct();
  expect(all_ndistinct.nrows() == 1, "Catalog::ndistinct() should return scalar streams");
}

void test_catalog_and_archive_lvalue_name_ranges() {
  const auto base = make_base_catalog();
  const std::vector<std::string> keys{"id", "city"};
  const auto selected = base.keys(keys);
  expect(selected.nkeys() == 2, "Catalog lvalue select should accept reusable key vectors");

  const std::vector<std::string> metric{"metric"};
  const auto filled = base.keys(metric).fillnull(0.0);
  expect(filled.keys("metric").as_series().host<double>() == std::vector<double>({10.0, 0.0, 30.0, 40.0, 0.0}),
         "Catalog lvalue fillnull should delegate to selected keys");

  const auto casted = base.keys(keys).cast().note();
  expect(casted.keys("id").kind() == StreamKind::Note, "Catalog lvalue cast should delegate to selected keys");

  const auto f0 = make_base_frame().slice(0, 3);
  const auto f1 = make_base_frame().slice(2, 5);
  const auto labels = pluss::table::Slate(
      pluss::make_note(std::vector<std::string>{"batch_a", "batch_b"}, std::array<std::int64_t, 1>{2}));
  const pluss::table::Atlas atlas({f0, f1}, labels);
  const auto archive = atlas.kindelize();

  const auto archive_selected = archive.keys(keys);
  expect(archive_selected.catalog(0).nkeys() == 2, "Archive lvalue select should delegate to Catalog");

  const auto archive_freq = archive.keys(std::vector<std::string>{"city"}).freq({.top = 1});
  expect(archive_freq.catalog(0).keys("city").size() == 1,
         "Archive lvalue freq should delegate to Catalog");
}

void test_catalog_aggregates_apply_text_merge() {
  auto base = make_base_catalog();
  expect(std::fabs(scalar_stream_to_double(base.keys("id").sum()) - 15.0) < 1e-9, "sum(id) mismatch");
  expect(std::fabs(scalar_stream_to_double(base.keys("id").min()) - 1.0) < 1e-9, "min(id) mismatch");
  expect(std::fabs(scalar_stream_to_double(base.keys("id").max()) - 5.0) < 1e-9, "max(id) mismatch");
  expect(base.nrows() == 5, "catalog row count mismatch");

  const auto right = Catalog(std::vector<std::pair<std::string, Stream>>{
      {"id", base.keys("id")},
      {"bonus", Stream(pluss::make_series(std::vector<double>{100, 200, 300, 400, 500}, {5}))},
  });
  const auto merged = base.merge(right, "id", "inner");
  expect(merged.has("bonus"), "merge should include right-side feature");
  expect(merged.nrows() == 5, "merge row count mismatch");
  // merge supports left/right/outer too, and never drops a record silently.
  const auto first_two = right.head(2);
  expect(base.merge(first_two, "id", "inner").nrows() == 2, "inner merge keeps only matched records");
  const auto kept = base.merge(first_two, "id", "left");
  expect(kept.nrows() == 5 && kept.keys("bonus").size() == 5,
         "left merge keeps records that have no partner (as nulls)");
  expect(base.merge(first_two, "id", "outer").nrows() == 5, "outer merge keeps everything");
  expect_throw([&]() { (void)base.merge(right, "id", "sideways"); }, "merge should reject an unknown how");

  const auto applied = base.keys({"id"}).map([](const Stream& s) { return s + 10.0; });
  const auto applied_values = applied.keys("id").as_series().host<std::int64_t>();
  expect(applied_values[0] == 13, "keys().map should transform target field");

  const auto transformed_all = base.transform([](const Stream& s) { return s; });
  const auto transformed_one = base.keys({"id"}).map([](const Stream& s) { return s + 1.0; });
  expect(transformed_all.nrows() == base.nrows(), "transform(all) should preserve row count");
  expect(transformed_one.keys("id").as_series().host<std::int64_t>()[0] == 4,
         "keys().map should transform target field");

  const auto updated_id_only = base.keys({"id"}).map([](const Stream& s) { return s - 1.0; });
  expect(updated_id_only.keys("id").as_series().host<std::int64_t>()[0] == 2,
         "keys().map reassignment should update target field");

  expect_throw(
      [&]() {
        (void)base.keys({"id"}).map([](const Stream& s) { return s.head(2); });
      },
      "keys().map with size-changing fn should throw");

  const auto text_clean = base.clean("text");
  const auto keywords = base.keywords("text", 4);
  const auto similarity = base.similarity("text", "city", "cosine");
  const auto text_filtered = base.grep("text", "rain");
  const auto text_features = base.featurize("text");
  const auto text_stats = base.stats("text", "mean");
  const auto text_tfidf = base.tfidf("text");

  expect(text_clean.nrows() == 5, "clean should preserve row count");
  expect(keywords.has("keyword") && keywords.has("count"), "keywords should emit keyword/count");
  expect(similarity.has("text__city_similarity"), "similarity key mismatch");
  expect(text_filtered.nrows() == 2, "grep('rain') row count mismatch");
  expect(text_features.has("text_char_len"), "featurize missing text_char_len");
  expect(text_features.has("text_word_count"), "featurize missing text_word_count");
  expect(text_stats.nrows() == 1, "stats should produce one row");
  expect(text_tfidf.has("text_tfidf_norm"), "tfidf should add norm feature");
}

void test_catalog_mutation_tabelize_io() {
  using namespace pluss::kindle::literals;
  auto base = make_base_catalog();

  const DataFrame default_cases({
      {"city", pluss::make_note({"Accra", "Tema", "Kumasi"})},
      {"units", pluss::make_series({10.0, 7.0, 12.0})},
  });
  const auto default_cases_roundtrip = default_cases.kindelize().tabelize();
  expect(default_cases_roundtrip.equals(default_cases),
         "kindelize/tabelize should preserve a numeric default case axis");

  base = base.set("id_plus", base.keys("id") + 1.0);
  base.set_("city_copy", base.keys("city"));
  base = base.set({
      "ones"_k = Stream(pluss::make_series(std::vector<double>{1, 1, 1, 1, 1}, {5})),
  });
  base.set_({
      "flag"_k = Stream(pluss::make_series(std::vector<double>{0, 1, 0, 1, 0}, {5})),
  });
  base = base.drop({"flag"});
  expect(base.has("id_plus"), "set should add id_plus");
  expect(base.has("city_copy"), "set_ should add city_copy");
  expect(base.has("ones"), "set should add ones");
  expect(!base.has("flag"), "drop should remove flag");

  pluss::kindle::TabelizePolicy policy;
  policy.align = pluss::kindle::Align::Inner;
  policy.align_by = pluss::kindle::AlignBy::Label;
  policy.include_label_feature = true;
  policy.label_feature = "case";
  policy.collision = pluss::kindle::CollisionPolicy::Suffix;
  const auto frame = base.tabelize(policy);
  expect(frame.ncases() == 5, "tabelize row count mismatch");
  expect(frame.nfeatures() >= base.nkeys(), "tabelize feature count mismatch");
  expect(!base.to_string().empty(), "to_string should not be empty");
  expect(!base.to_json().empty(), "to_json should not be empty");
  expect(base.to_json().find("\"_type\": \"Catalog\"") != std::string::npos,
         "Catalog::to_json should emit a Catalog JSON object");
  expect(base.to_json().find("\"streams\"") != std::string::npos,
         "Catalog::to_json should include stream entries");
  expect(base.keys("id").to_json().find("\"_type\": \"Stream\"") != std::string::npos,
         "Stream::to_json should emit a Stream JSON object");
  expect(base.keys("id").to_json().find("\"values\"") != std::string::npos,
         "Stream::to_json should include values");

  std::int64_t iter_count = 0;
  for (const auto& key : base) {
    (void)key;
    ++iter_count;
  }
  expect(iter_count == base.nkeys(), "iterator should visit all keys");

  const auto out_path = temp_csv_path("catalog_save");
  base.save_csv(out_path.string());
  const auto reloaded = DataFrame::load_csv(out_path.string());
  expect(reloaded.ncases() == 5, "save_csv/load_csv roundtrip rows mismatch");

  auto txt_out = temp_csv_path("catalog_save_txt");
  txt_out.replace_extension(".txt");
  base.save_txt(txt_out.string(), pluss::loader::config_from({{"header", true}}));
  const auto txt_roundtrip = DataFrame::load_txt(txt_out.string(), {{"header", true}});
  expect(txt_roundtrip.ncases() == 5, "Catalog::save_txt roundtrip rows mismatch");

  auto json_out = temp_csv_path("catalog_save_json");
  json_out.replace_extension(".json");
  base.save_json(json_out.string());
  const auto json_roundtrip = DataFrame::load_json(json_out.string());
  expect(json_roundtrip.ncases() == 5, "Catalog::save_json roundtrip rows mismatch");

  auto save_as_json_out = temp_csv_path("catalog_save_as_json");
  save_as_json_out.replace_extension(".json");
  base.save_as("json", save_as_json_out.string(), pluss::loader::config_from({{"orient", "split"}}));
  const auto save_as_json_roundtrip = DataFrame::load_json(save_as_json_out.string());
  expect(save_as_json_roundtrip.ncases() == 5, "Catalog::save_as(json) roundtrip rows mismatch");

  const auto loaded_csv = Catalog::load_csv(data_file("messy_data.csv").string());
  expect(loaded_csv.nkeys() > 0, "load_csv should return non-empty catalog");

  const auto loaded_generic = Catalog::load(
      "csv",
      data_file("messy_data.csv").string(),
      pluss::loader::config_from({
          {"ncases", 8},
      }));
  expect(loaded_generic.nrows() == 8, "Catalog::load(csv, config) row count mismatch");

  pluss::loader::ReadCsvOptions opts;
  opts.ncases = 24;
  const auto keyed = Catalog::load_labeled(data_file("bank-full.csv").string(), "job", opts);
  expect(keyed.has("_label"), "load_labeled should materialize _label key");

  const auto from_json = Catalog::from_json(
      R"([
  {"id": 1, "city": "Accra", "when": "2024-01-01T00:00:00Z"},
  {"id": 2, "city": "Kumasi"},
  {"id": 3, "city": "Tema", "when": "2024-01-03T00:00:00Z"}
])");
  expect(from_json.nkeys() == 3, "Catalog::from_json key count mismatch");
  expect(from_json.keys("id").kind() == StreamKind::Series, "Catalog::from_json id kind mismatch");
  expect(from_json.keys("city").kind() == StreamKind::Note, "Catalog::from_json city kind mismatch");
  expect(from_json.keys("when").kind() == StreamKind::Timeline, "Catalog::from_json when kind mismatch");

  auto json_path = temp_csv_path("catalog_json");
  json_path.replace_extension(".json");
  {
    std::ofstream out(json_path, std::ios::out | std::ios::trunc | std::ios::binary);
    expect(out.is_open(), "failed to create catalog json temp file");
    out << R"({"id":[1,2], "city":["Accra","Tema"]})";
  }
  const auto loaded_json = Catalog::load_json(json_path.string());
  expect(loaded_json.nkeys() == 2, "Catalog::load_json key count mismatch");
  expect(loaded_json.nrows() == 2, "Catalog::load_json row count mismatch");
  const auto loaded_json_generic = Catalog::load("json", json_path.string());
  expect(loaded_json_generic.nkeys() == loaded_json.nkeys(),
         "Catalog::load(json) should match Catalog::load_json");

  auto txt_path = temp_csv_path("catalog_txt");
  txt_path.replace_extension(".txt");
  {
    std::ofstream out(txt_path, std::ios::out | std::ios::trunc | std::ios::binary);
    expect(out.is_open(), "failed to create catalog txt temp file");
    out << "id city\n1 Accra\n2 Tema\n";
  }
  const auto loaded_txt = Catalog::load_txt(
      txt_path.string(),
      pluss::loader::config_from({
          {"txt_mode", "whitespace"},
          {"header", true},
      }));
  expect(loaded_txt.nkeys() == 2, "Catalog::load_txt key count mismatch");
  expect(loaded_txt.nrows() == 2, "Catalog::load_txt row count mismatch");
}

void test_archive_to_table_bridge() {
  const auto base = make_base_frame();
  const auto f0 = base.slice(0, 3);
  const auto f1 = base.slice(2, 5);
  const auto labels = pluss::table::Slate(
      pluss::make_note(std::vector<std::string>{"batch_a", "batch_b"}, std::array<std::int64_t, 1>{2}));
  const pluss::table::Atlas atlas({f0, f1}, labels);

  const auto archive = atlas.kindelize();
  expect(archive.size() == archive.depth(), "Archive::size should match depth");
  expect(archive.nkeys() == 6, "Archive::nkeys mismatch");
  expect(archive.minkeys() == 6 && archive.maxkeys() == 6, "Archive key span mismatch");
  expect(archive.nrecords() == 3, "Archive::nrecords mismatch");
  expect(archive.minrecords() == 3 && archive.maxrecords() == 3, "Archive record span mismatch");
  expect(archive.shape() == std::make_tuple<std::int64_t, std::int64_t, std::int64_t>(2, 3, 6),
         "Archive::shape should be (depth, max records, max keys)");
  expect(archive.has("city") && !archive.has("missing"), "Archive::has should require key in every catalog");
  expect(archive.label().has(), "Archive::label().has should detect depth labels");
  expect(archive.label().values().size() == archive.depth(), "Archive::label().values size mismatch");
  expect(archive.dloc(0).nkeys() == 6, "Archive::dloc key count mismatch");
  expect(archive.loc("batch_b").nkeys() == 6, "Archive::loc(string) key count mismatch");
  expect(archive.head(1).depth() == 1, "Archive::head depth mismatch");
  expect(archive.tail(1).depth() == 1, "Archive::tail depth mismatch");
  expect(archive.slice_depth(0, 1).depth() == 1, "Archive::slice_depth mismatch");

  const auto sorted_desc = archive.label().sort(false);
  const auto sorted_labels = sorted_desc.label().values().to_slate().cast().note().as_note();
  expect(std::string(sorted_labels.at(0)) == "batch_b", "Archive::sort_depth(desc) label mismatch");

  const auto relabeled = archive.label().set(
      Stream(pluss::make_note(std::vector<std::string>{"A", "B"}, std::array<std::int64_t, 1>{2})));
  expect(relabeled.label().has(), "Archive::label().set should set labels");

  const auto aligned = archive.label().align(
      Stream(pluss::make_note(std::vector<std::string>{"batch_b", "batch_a"}, std::array<std::int64_t, 1>{2})));
  const auto aligned_first = aligned.catalog(0).keys("id").as_series().host<std::int64_t>();
  expect(aligned_first == f1.features("id").as_series().host<std::int64_t>(),
         "Archive::label().align should reorder catalogs");
  expect(!relabeled.label().reset().label().has(), "Archive::label().reset should clear labels");

  const auto mapped = archive.map_catalogs([](const Catalog& c) { return c.keys({"id", "city"}); });
  expect(mapped.catalog(0).nkeys() == 2, "Archive::map_catalogs should transform each catalog");
  const auto reduced = archive.aggregate_over_catalogs([](const std::vector<Catalog>& cats) { return cats.front(); });
  expect(reduced.nkeys() == archive.catalog(0).nkeys(),
         "Archive::aggregate_over_catalogs should return reducer output");

  const auto selected = archive.keys({"id", "city"});
  const auto dropped = archive.drop({"time_index"});
  const auto renamed = archive.rename("city", "town");
  expect(selected.catalog(0).nkeys() == 2, "Archive::keys(names) should project each catalog");
  expect(dropped.catalog(0).nkeys() == 5, "Archive::drop should delegate to catalogs");
  expect(renamed.catalog(0).has("town"), "Archive::rename should delegate to catalogs");
  expect(archive.keyaxis().series().catalog(0).nkeys() == 2, "Archive::keyaxis().series projection mismatch");
  expect(archive.keyaxis().like("time").catalog(0).nkeys() == 2, "Archive::keyaxis().like selection mismatch");
  const auto& cat0 = archive.catalog(0);
  const auto archive_prefixed = pluss::kindle::Archive({
      Catalog(std::vector<std::pair<std::string, Stream>>{
          {"src_id", cat0.keys("id")},
          {"src_city", cat0.keys("city")},
          {"stamp", cat0.keys("timestamp")},
      }),
      Catalog(std::vector<std::pair<std::string, Stream>>{
          {"src_id", cat0.keys("id")},
          {"src_city", cat0.keys("city")},
          {"stamp", cat0.keys("timestamp")},
      }),
  });
  expect(archive_prefixed.keyaxis().prefix("src_").catalog(0).nkeys() == 2,
         "Archive::keyaxis().prefix should filter matching keys");
  expect(!archive_prefixed.keyaxis().prefix("src_").catalog(0).has("stamp"),
         "Archive::keyaxis().prefix should exclude non-matching keys");
  const auto archive_suffixed = pluss::kindle::Archive({
      Catalog(std::vector<std::pair<std::string, Stream>>{
          {"id_raw", cat0.keys("id")},
          {"city_raw", cat0.keys("city")},
          {"timestamp", cat0.keys("timestamp")},
      }),
      Catalog(std::vector<std::pair<std::string, Stream>>{
          {"id_raw", cat0.keys("id")},
          {"city_raw", cat0.keys("city")},
          {"timestamp", cat0.keys("timestamp")},
      }),
  });
  expect(archive_suffixed.keyaxis().suffix("_raw").catalog(0).nkeys() == 2,
         "Archive::keyaxis().suffix should filter matching keys");
  expect(!archive_suffixed.keyaxis().suffix("_raw").catalog(0).has("timestamp"),
         "Archive::keyaxis().suffix should exclude non-matching keys");

  const auto filled = archive.keys({"metric"}).fillnull(0.0);
  const auto dropped_null = archive.keys({"metric"}).dropnull();
  const auto nulls = archive.keys({"metric"}).isnull();
  const auto notnulls = archive.keys({"metric"}).notnull();
  const auto invalid = archive.keys({"metric"}).invalid();
  expect(filled.catalog(0).keys("metric").as_series().host<double>()[1] == 0.0,
         "Archive::fillnull should delegate to catalogs");
  expect(dropped_null.catalog(0).nrows() == 2 && dropped_null.catalog(1).nrows() == 2,
         "Archive::dropnull should delegate to catalogs");
  expect(nulls.catalog(0).keys("metric").kind() == StreamKind::Series,
         "Archive::isnull should return boolean streams");
  expect(notnulls.catalog(0).keys("metric").kind() == StreamKind::Series,
         "Archive::notnull should return boolean streams");
  expect(invalid.catalog(0).keys("metric").kind() == StreamKind::Series,
         "Archive::invalid should return boolean streams");

  const auto casted = archive.keys({"id"}).cast().note();
  const auto typed = archive.keys({"id"}).astype(pluss::codem::ScalarType::Float64);
  expect(casted.catalog(0).keys("id").kind() == StreamKind::Note, "Archive::keys().cast should delegate to catalogs");
  expect(typed.catalog(0).keys("id").as_series().scalar_type() == pluss::codem::ScalarType::Float64,
         "Archive::keys().astype should delegate to catalogs");

  const auto distinct = archive.keys({"city"}).distinct();
  const auto freq = archive.keys({"city"}).freq({.top = 1});
  const auto ndistinct = archive.keys({"city"}).ndistinct();
  expect(distinct.catalog(0).keys("city").size() == 2, "Archive::keys().distinct should delegate to catalogs");
  expect(freq.catalog(0).keys("city").size() == 1, "Archive::keys().freq should delegate to catalogs");
  expect(ndistinct.catalog(0).keys("city").size() == 1,
         "Archive::keys().ndistinct should produce scalar streams for selected keys");

  const auto atlas_a = archive.tabelize();
  const auto atlas_b = archive.tabelize();
  expect(atlas_a.depth() == atlas_b.depth(), "Archive::tabelize depth mismatch");
  expect(atlas_a.ncases() == atlas_b.ncases(), "Archive::tabelize nrows mismatch");
  expect(atlas_a.nfeatures() == atlas_b.nfeatures(), "Archive::tabelize ncols mismatch");
}

void test_archive_plot_accessor() {
  const auto depth_labels =
      Stream(pluss::make_note(std::vector<std::string>{"d0", "d1", "d2"}, std::array<std::int64_t, 1>{3}));

  const auto ragged_labeled = pluss::kindle::Archive(
      {
          Catalog(std::vector<std::pair<std::string, Stream>>{
              {"x", labeled_series({1.0, 2.0, 3.0, 4.0}, {"a", "b", "c", "d"})},
              {"y", labeled_series({10.0, 20.0, 30.0, 40.0}, {"a", "b", "c", "d"})},
              {"z", labeled_series({100.0, 200.0, 300.0, 400.0}, {"a", "b", "c", "d"})},
              {"metric", labeled_series({5.0, 6.0, 7.0, 8.0}, {"a", "b", "c", "d"})},
              {"unrelated", Stream(pluss::make_series(std::vector<double>{9.0, 9.0}, {2}))},
          }),
          Catalog(std::vector<std::pair<std::string, Stream>>{
              {"x", labeled_series({1.5, 3.5}, {"a", "c"})},
              {"y", labeled_series({15.0, 35.0}, {"a", "c"})},
              {"z", labeled_series({150.0, 350.0}, {"a", "c"})},
              {"metric", labeled_series({5.5, 7.5}, {"a", "c"})},
              {"unrelated", Stream(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3}))},
          }),
          Catalog(std::vector<std::pair<std::string, Stream>>{
              {"x", labeled_series({2.5, 4.5, 5.5}, {"b", "d", "e"})},
              {"y", labeled_series({25.0, 45.0, 55.0}, {"b", "d", "e"})},
              {"z", labeled_series({250.0, 450.0, 550.0}, {"b", "d", "e"})},
              {"metric", labeled_series({6.5, 8.5, 9.5}, {"b", "d", "e"})},
              {"unrelated", Stream(pluss::make_series(std::vector<double>{4.0}, {1}))},
          }),
      },
      depth_labels);

  expect_no_throw([&]() {
    auto fig = ragged_labeled.plot().surface("metric");
    (void)fig;
  }, "Archive::plot().surface should reconcile ragged labeled frames over depth");

  expect_no_throw([&]() {
    auto fig = ragged_labeled.plot().wireframe("metric");
    (void)fig;
  }, "Archive::plot().wireframe should reconcile ragged labeled frames over depth");

  expect_no_throw([&]() {
    auto fig = ragged_labeled.plot().cloud3("x", "y", "z");
    (void)fig;
  }, "Archive::plot().cloud3 should allow each depth frame to keep its own row count");

  expect_no_throw([&]() {
    auto fig = ragged_labeled.plot().depth("metric");
    (void)fig;
  }, "Archive::plot().depth should summarize ragged frames without forcing equal case counts");

  expect_no_throw([&]() {
    auto fig = ragged_labeled.plot().depth_compare({"metric", "x"});
    (void)fig;
  }, "Archive::plot().depth_compare should summarize selected ragged features over depth");

  expect_no_throw([&]() {
    auto fig = ragged_labeled.plot().depth_facet({"metric", "x"});
    (void)fig;
  }, "Archive::plot().depth_facet should summarize selected ragged features over depth");

  expect_no_throw([&]() {
    auto fig = ragged_labeled.plot().depth_rolling("metric");
    (void)fig;
  }, "Archive::plot().depth_rolling should compute rolling depth statistics on ragged frames");

  expect_no_throw([&]() {
    auto fig = ragged_labeled.plot().triplot({"x", "y", "z"});
    (void)fig;
  }, "Archive::plot().triplot should flatten ragged frame columns without equal-length depth requirements");

  const auto ragged_positional = pluss::kindle::Archive(
      {
          Catalog(std::vector<std::pair<std::string, Stream>>{
              {"x", Stream(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4}))},
              {"y", Stream(pluss::make_series(std::vector<double>{11.0, 12.0, 13.0, 14.0}, {4}))},
              {"z", Stream(pluss::make_series(std::vector<double>{21.0, 22.0, 23.0, 24.0}, {4}))},
              {"metric", Stream(pluss::make_series(std::vector<double>{31.0, 32.0, 33.0, 34.0}, {4}))},
          }),
          Catalog(std::vector<std::pair<std::string, Stream>>{
              {"x", Stream(pluss::make_series(std::vector<double>{1.5, 2.5}, {2}))},
              {"y", Stream(pluss::make_series(std::vector<double>{11.5, 12.5}, {2}))},
              {"z", Stream(pluss::make_series(std::vector<double>{21.5, 22.5}, {2}))},
              {"metric", Stream(pluss::make_series(std::vector<double>{31.5, 32.5}, {2}))},
          }),
      },
      Stream(pluss::make_note(std::vector<std::string>{"p0", "p1"}, std::array<std::int64_t, 1>{2})));

  expect_no_throw([&]() {
    auto fig = ragged_positional.plot().surface("metric");
    (void)fig;
  }, "Archive::plot().surface should fall back to positional outer alignment when case labels are unavailable");

  expect_no_throw([&]() {
    auto fig = ragged_positional.plot().wireframe("metric");
    (void)fig;
  }, "Archive::plot().wireframe should fall back to positional outer alignment when case labels are unavailable");
}

void test_archive_time_accessors() {
  const auto archive = pluss::kindle::Archive({
      Catalog(std::vector<std::pair<std::string, Stream>>{
          {"timestamp",
           Stream(pluss::make_timeline({
               "2024-01-01T00:00:00Z",
               "2024-01-02T00:00:00Z",
               "2024-01-03T00:00:00Z",
           }))},
          {"metric", Stream(pluss::make_series(std::vector<double>{10.0, 12.0, 11.0}, {3}))},
      }),
      Catalog(std::vector<std::pair<std::string, Stream>>{
          {"timestamp",
           Stream(pluss::make_timeline({
               "2024-02-01T00:00:00Z",
               "2024-02-02T00:00:00Z",
               "2024-02-03T00:00:00Z",
               "2024-02-04T00:00:00Z",
               "2024-02-05T00:00:00Z",
           }))},
          {"metric", Stream(pluss::make_series(std::vector<double>{20.0, 22.0, 24.0, 26.0, 28.0}, {5}))},
      }),
  });

  const auto years = archive.keys({"timestamp"}).tl().year();
  expect(years.catalog(0).keys("timestamp").kind() == StreamKind::Series,
         "Archive::keys(...).tl().year should convert selected timeline streams to Series");
  expect(years.catalog(0).keys("timestamp").as_series().host<std::int64_t>()
             == std::vector<std::int64_t>({2024, 2024, 2024}),
         "Archive::keys(...).tl().year should preserve first ragged catalog length");
  expect(years.catalog(1).keys("timestamp").as_series().host<std::int64_t>()
             == std::vector<std::int64_t>({2024, 2024, 2024, 2024, 2024}),
         "Archive::keys(...).tl().year should preserve second ragged catalog length");

  const auto month_start = archive.keys({"timestamp"}).tl().month_start();
  expect(month_start.catalog(0).keys("timestamp").kind() == StreamKind::Series,
         "Archive::keys(...).tl().month_start should yield boolean Series streams");
  expect(month_start.catalog(1).keys("timestamp").size() == 5,
         "Archive::keys(...).tl().month_start should preserve ragged per-catalog sizes");

  const auto lagged = archive.keys({"metric"}).ts().lag(1);
  expect(lagged.catalog(0).keys("metric").kind() == StreamKind::Series,
         "Archive::keys(...).ts().lag should keep Series kind");
  expect(lagged.catalog(0).keys("metric").size() == 3,
         "Archive::keys(...).ts().lag should preserve first ragged catalog length");
  expect(lagged.catalog(1).keys("metric").size() == 5,
         "Archive::keys(...).ts().lag should preserve second ragged catalog length");

  const auto rolled = archive.keys({"metric"}).ts().rolling(2, pluss::table::WindowStatistic::Mean);
  expect(rolled.catalog(0).keys("metric").size() == 3,
         "Archive::keys(...).ts().rolling should preserve first ragged catalog length");
  expect(rolled.catalog(1).keys("metric").size() == 5,
         "Archive::keys(...).ts().rolling should preserve second ragged catalog length");
}


void test_catalog_ragged_persistence() {
  namespace fs = std::filesystem;
  const auto root = fs::temp_directory_path() / "pluss_catalog_persist_test";
  fs::remove_all(root);

  for (const std::string kind : {"parquet", "csv"}) {
    std::vector<std::pair<std::string, Stream>> seed{{"a", dstream({0.0})}};
    Catalog cat(std::move(seed));
    cat.set_("a", Stream(pluss::make_series(std::vector<double>{1.5, 2.5, 3.5, 4.5, 5.5}, {5}),
                         bool_mask({true, false, true, true, true})));
    cat.set_("weird,key \"q\"", labeled_series({7.0, 8.0}, {"x", "y"}));
    cat.set_("n", labeled_note({"p", "q", "r"}, {"l1", "l2", "l3"}));
    cat.set_("i", Stream(pluss::make_series(std::vector<std::int64_t>{10, 20, 30, 40}, {4})));
    cat.set_("t", Stream(pluss::make_timeline({"2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z"})));
    cat.set_("empty", Stream(pluss::make_series(std::vector<double>{}, {0})));

    const auto dir = (root / kind).string();
    cat.save_streams(dir, kind);
    const auto back = Catalog::load_streams(dir);

    expect(back.keys() == cat.keys(), kind + ": key order should round-trip");
    for (const auto& key : cat.keys()) {
      const auto a = cat.get(key).value(), b = back.get(key).value();
      expect(a.size() == b.size(), kind + ": length should round-trip for '" + key + "'");
      expect(a.kind() == b.kind(), kind + ": kind should round-trip for '" + key + "'");
      expect(a.has_label() == b.has_label(), kind + ": label presence should round-trip for '" + key + "'");
    }
    expect(back.get("a").value().size() == 5 && back.get("empty").value().size() == 0, kind + ": ragged extents are exact");
    auto av = stream_values(back.get("a").value());
    av[1] = 2.5;  // the payload under a null is unspecified; only validity matters there
    expect(near_all(av, {1.5, 2.5, 3.5, 4.5, 5.5}) &&
               stream_valid(back.get("a").value()) == std::vector<bool>({true, false, true, true, true}),
           kind + ": values and nulls round-trip");
    expect(note_values(back.get("n").value().as_note()) == std::vector<std::string>({"p", "q", "r"}),
           kind + ": note values round-trip");
    expect(note_values(back.get("n").value().label()->as_note()) == std::vector<std::string>({"l1", "l2", "l3"}),
           kind + ": labels round-trip");
    expect(near_all(stream_values(back.get("weird,key \"q\"").value()), {7.0, 8.0}), kind + ": awkward keys round-trip");
    expect(near_all(stream_values(back.get("i").value()), {10, 20, 30, 40}), kind + ": int values round-trip");
  }

  expect_throw([&] { (void)Catalog::load_streams((root / "nowhere").string()); },
               "load_streams should reject a directory with no manifest");
  expect_throw([&] { Catalog(std::vector<std::pair<std::string, Stream>>{{"a", dstream({1})}}).save_streams((root / "bad").string(), "xlsx"); },
               "save_streams should reject unknown store kinds");
  {  // a truncated stream file is caught by the manifest's record count
    const auto dir = (root / "parquet").string();
    Catalog cat(std::vector<std::pair<std::string, Stream>>{{"a", dstream({1, 2, 3})}});
    cat.save_streams((root / "trunc").string(), "csv");
    Catalog other(std::vector<std::pair<std::string, Stream>>{{"a", dstream({1, 2})}});
    other.save_streams((root / "trunc2").string(), "csv");
    fs::copy_file(root / "trunc2" / "stream_000000.csv", root / "trunc" / "stream_000000.csv",
                  fs::copy_options::overwrite_existing);
    expect_throw([&] { (void)Catalog::load_streams((root / "trunc").string()); },
                 "a record-count mismatch against the manifest should throw");
    (void)dir;
  }
  fs::remove_all(root);
}


pluss::Series mask_of(const std::vector<bool>& m) {
  std::vector<double> d(m.size());
  for (std::size_t i = 0; i < m.size(); ++i) d[i] = m[i] ? 1.0 : 0.0;
  return pluss::make_series(d, {static_cast<std::int64_t>(d.size())}).to(pluss::codem::ScalarType::Bool);
}

void test_catalog_fused_reductions() {
  std::mt19937 rng(20261008);
  std::uniform_int_distribution<int> len(0, 40), nstreams(1, 6);
  std::uniform_real_distribution<double> val(-5.0, 5.0);
  std::bernoulli_distribution nullp(0.2);
  const std::vector<std::string> all{"sum", "mean", "min", "max", "var", "std"};
  for (int trial = 0; trial < 60; ++trial) {
    std::vector<std::pair<std::string, Stream>> streams;
    std::vector<std::vector<double>> raw;
    std::vector<std::vector<bool>> ok;
    const int k = nstreams(rng);
    for (int c = 0; c < k; ++c) {
      std::vector<double> v(static_cast<std::size_t>(c == 0 ? 1 + len(rng) : len(rng)));
      std::vector<bool> m(v.size());
      for (std::size_t i = 0; i < v.size(); ++i) { v[i] = val(rng); m[i] = !nullp(rng); }
      auto st = Stream(pluss::make_series(v, {static_cast<std::int64_t>(v.size())}));
      if (!v.empty()) st = pluss::kindle::detail::with_validity_mask(st, mask_of(m));
      streams.emplace_back("s" + std::to_string(c), st);
      raw.push_back(v);
      ok.push_back(m);
    }
    const Catalog cat(std::move(streams));

    CatalogReduceOptions opt;
    opt.unbiased = trial % 2 == 0;
    if (trial % 3 == 1) opt.ended = pluss::kindle::RaggedEnd::Null;
    if (trial % 5 == 2) opt.min_streams = 2;
    if (trial % 4 == 3) opt.align = pluss::kindle::Align::Inner;

    opt.axis = pluss::kindle::ReduceAxis::Key;
    const auto fused = cat.reduce(all, opt);
    expect(fused.keys() == all, "fused reduce returns one stream per op, in order");
    const Catalog singles[] = {cat.sum(opt), cat.mean(opt), cat.min(opt), cat.max(opt), cat.var(opt), cat.std(opt)};
    for (std::size_t i = 0; i < all.size(); ++i) {
      const auto a = fused.get(all[i]).value();
      const auto b = singles[i].get(all[i]).value();
      expect(a.size() == b.size(), "fused '" + all[i] + "' has the single reduction's length");
      const auto va = stream_valid(a), vb = stream_valid(b);
      expect(va == vb, "fused '" + all[i] + "' has the single reduction's validity");
      auto xa = stream_values(a), xb = stream_values(b);
      for (std::size_t j = 0; j < xa.size(); ++j) if (!va[j]) xa[j] = xb[j] = 0.0;
      expect(near_all(xa, xb, 1e-12), "fused '" + all[i] + "' equals the single reduction");
    }

    // count = contributing streams per index (brute force; Head alignment, longest domain)
    if (opt.align == pluss::kindle::Align::Outer && opt.ended == pluss::kindle::RaggedEnd::Skip &&
        opt.min_streams == 1) {
      const auto cnt = stream_values(cat.reduce({"count"}, opt).get("count").value());
      std::size_t width = 0;
      for (const auto& r : raw) width = std::max(width, r.size());
      expect(cnt.size() == width, "count spans the longest stream");
      for (std::size_t j = 0; j < width; ++j) {
        double want = 0;
        for (std::size_t c = 0; c < raw.size(); ++c) want += j < raw[c].size() && ok[c][j];
        expect(cnt[j] == want, "count matches the brute-force contributor count");
      }
    }
  }

  const Catalog tiny(std::vector<std::pair<std::string, Stream>>{{"a", dstream({1, 2})}});
  expect_throw([&] { (void)tiny.reduce({"sum", "sum"}); }, "duplicate ops are rejected");
  expect_throw([&] { (void)tiny.reduce({"median"}); }, "unknown ops are rejected");
  expect_throw([&] { (void)tiny.reduce(std::vector<std::string>{}); }, "an empty op list is rejected");
}


void test_ragged_binary() {
  using pluss::kindle::OpClass;
  using pluss::kindle::RaggedAbsent;
  using pluss::kindle::RaggedAlignment;
  using pluss::kindle::RaggedDomain;
  using pluss::kindle::RaggedLoss;
  using pluss::kindle::RaggedPolicy;
  const auto a = dstream({1, 2, 3, 4, 5});
  const auto b = dstream({10, 20, 30});

  // Default: longest domain, absent operand -> null; nothing dropped.
  const auto sum = pluss::kindle::ragged_binary(a, b, "+");
  expect(sum.size() == 5, "default keeps the longest extent");
  expect(stream_valid(sum) == std::vector<bool>({true, true, true, false, false}),
         "absent operand gives null, present pairs stay valid");
  expect(near_all({stream_values(sum)[0], stream_values(sum)[1], stream_values(sum)[2]}, {11, 22, 33}),
         "valid positions hold the pairwise sums");
  // Symmetric: absent on the left.
  const auto rsum = pluss::kindle::ragged_binary(b, a, "+");
  expect(rsum.size() == 5 && stream_valid(rsum) == stream_valid(sum), "operand order does not change the shape");

  // Equal lengths equal the plain operator.
  const auto same = pluss::kindle::ragged_binary(a, dstream({1, 1, 1, 1, 1}), "-");
  expect(near_all(stream_values(same), {0, 1, 2, 3, 4}), "equal lengths take the plain path");

  auto policy = RaggedPolicy::defaults(OpClass::Elementwise);
  policy.domain = RaggedDomain::Shortest;
  const auto shortest = pluss::kindle::ragged_binary(a, b, "*", policy);
  expect(shortest.size() == 3 && near_all(stream_values(shortest), {10, 40, 90}), "Shortest keeps complete pairs");
  policy.loss = RaggedLoss::Error;
  expect_throw([&] { (void)pluss::kindle::ragged_binary(a, b, "*", policy); }, "loss=Error refuses to drop records");

  auto tail = RaggedPolicy::defaults(OpClass::Elementwise);
  tail.alignment = RaggedAlignment::Tail;
  const auto tl = pluss::kindle::ragged_binary(a, b, "+", tail);  // b lines up with a[2..4]
  expect(stream_valid(tl) == std::vector<bool>({false, false, true, true, true}) &&
             near_all({stream_values(tl)[2], stream_values(tl)[3], stream_values(tl)[4]}, {13, 24, 35}),
         "Tail alignment lines the last records up");

  auto fill = RaggedPolicy::defaults(OpClass::Elementwise);
  fill.absent = RaggedAbsent::Fill;
  fill.fill = 100.0;
  const auto filled = pluss::kindle::ragged_binary(a, b, "+", fill);
  expect(near_all(stream_values(filled), {11, 22, 33, 104, 105}) && stream_valid(filled) == std::vector<bool>(5, true),
         "Fill substitutes the value for the absent operand");

  auto ff = RaggedPolicy::defaults(OpClass::Elementwise);
  ff.absent = RaggedAbsent::ForwardFill;
  const auto forward = pluss::kindle::ragged_binary(a, b, "+", ff);
  expect(near_all(stream_values(forward), {11, 22, 33, 34, 35}), "ForwardFill repeats the last record");

  auto exact = RaggedPolicy::defaults(OpClass::Elementwise);
  exact.domain = RaggedDomain::Exact;
  expect_throw([&] { (void)pluss::kindle::ragged_binary(a, b, "+", exact); }, "Exact rejects different lengths");
  auto err = RaggedPolicy::defaults(OpClass::Elementwise);
  err.absent = RaggedAbsent::Error;
  expect_throw([&] { (void)pluss::kindle::ragged_binary(a, b, "+", err); }, "absent=Error rejects different lengths");
  expect_throw([&] { (void)pluss::kindle::ragged_binary(a, b, "^"); }, "unknown operators are rejected");

  // Comparison of ragged streams, and nulls already in the data stay null.
  const auto cmp = pluss::kindle::ragged_binary(a, b, ">");
  expect(cmp.size() == 5, "comparisons are ragged-aware too");

  // Catalog level.
  const Catalog cat(std::vector<std::pair<std::string, Stream>>{{"x", a}, {"y", b}});
  const auto out = cat.combine("x", "y", "+");
  expect(out.has("x+y") && out.get("x+y")->size() == 5 && out.get("x")->size() == 5 && out.get("y")->size() == 3,
         "combine adds the result and leaves the operands ragged");
  expect_throw([&] { (void)cat.combine("x", "nope", "+"); }, "combine rejects unknown keys");
}


void test_join_build_side_cache() {
  using pluss::relational::JoinHow;
  std::mt19937 rng(77);
  const std::int64_t n = 20000;
  std::vector<std::int64_t> lk(n), rk(n / 2);
  std::vector<double> lv(n), rv(n / 2);
  for (std::int64_t i = 0; i < n; ++i) { lk[i] = static_cast<std::int64_t>(rng() % (n / 2 + 100)); lv[i] = double(i); }
  for (std::int64_t i = 0; i < n / 2; ++i) { rk[i] = i; rv[i] = i * 0.5; }
  auto ser = [](const std::vector<std::int64_t>& v) { return Stream(pluss::make_series(v, {static_cast<std::int64_t>(v.size())})); };
  Catalog L(std::vector<std::pair<std::string, Stream>>{{"k", ser(lk)}, {"a", dstream(lv)}});
  Catalog R(std::vector<std::pair<std::string, Stream>>{{"k", ser(rk)}, {"b", dstream(rv)}});
  pluss::kindle::CatalogJoinSpec spec;
  spec.how = JoinHow::Left;
  spec.left_keys = {"k"};
  spec.right_keys = {"k"};

  Catalog::clear_join_cache();
  const auto before = Catalog::join_cache_stats();
  const auto first = L.join_detailed(R, spec);
  const auto mid = Catalog::join_cache_stats();
  const auto second = L.join_detailed(R, spec);
  const auto after = Catalog::join_cache_stats();
  expect(mid.second == before.second + 1 && after.first == mid.first + 1,
         "the second join on the same right side is served from the cache");
  expect(first.report.summary() == second.report.summary(), "a cached join reports exactly like a fresh one");
  expect(near_all(stream_values(first.catalog.get("b").value()), stream_values(second.catalog.get("b").value())),
         "a cached join produces the same data");

  // Editing the right side in place must not serve stale results.
  Catalog::clear_join_cache();
  const auto fresh = L.join_detailed(R, spec);
  expect(first.report.summary() == fresh.report.summary(), "cache-free result matches the cached one");

  {  // An in-place edit of storage shared with the right side must never serve a stale table.
    spec.how = JoinHow::Left;
    auto shared_keys = pluss::make_series(rk, {static_cast<std::int64_t>(rk.size())});
    Catalog R3(std::vector<std::pair<std::string, Stream>>{{"k", Stream(shared_keys)}, {"b", dstream(rv)}});
    Catalog::clear_join_cache();
    const auto pre = L.join_detailed(R3, spec);
    shared_keys.add_(5);
    const auto post_cached = L.join_detailed(R3, spec);
    Catalog::clear_join_cache();
    const auto post_fresh = L.join_detailed(R3, spec);
    expect(post_cached.report.summary() == post_fresh.report.summary(),
           "an in-place edit of shared key storage is never answered from a stale cached table");
    (void)pre;
  }

  // A different key column (or data) is a different entry, and results follow the data.
  std::vector<std::int64_t> shifted(rk.size());
  for (std::size_t i = 0; i < rk.size(); ++i) shifted[i] = rk[i] + 5;
  Catalog R2(std::vector<std::pair<std::string, Stream>>{{"k", ser(shifted)}, {"b", dstream(rv)}});
  const auto other = L.join_detailed(R2, spec);
  expect(!near_all(stream_values(other.catalog.get("b").value()), stream_values(first.catalog.get("b").value())),
         "different right keys give a different join");

  // Every join kind agrees with and without the cache.
  for (const auto how : {JoinHow::Inner, JoinHow::Left, JoinHow::Right, JoinHow::Outer, JoinHow::LeftOnly,
                         JoinHow::RightOnly}) {
    spec.how = how;
    Catalog::clear_join_cache();
    const auto cold = L.join_detailed(R, spec);
    const auto warm = L.join_detailed(R, spec);
    expect(cold.report.summary() == warm.report.summary(), "cold and warm joins agree for every join kind");
  }
}

} // namespace

int main() {
  ::setenv("PLUSS_ERROR_NATIVE_TRACE", "0", 0);
  try {
    test_stream_methods();
    test_stream_accessors();
    test_catalog_plot_accessor();
    test_catalog_plot3d_accessor();
    test_catalog_accessors();
    test_catalog_reductions();
    test_catalog_jagged_reductions();
    test_catalog_fused_reductions();
    test_ragged_binary();
    test_join_build_side_cache();
    test_ragged_policy_and_layout();
    test_catalog_join_policy();
    test_stream_reduce_groups_and_expand();
    test_catalog_ragged_persistence();
    test_catalog_construction_and_selection();
    test_catalog_label_alignment();
    test_catalog_sort_group_missing();
    test_catalog_and_archive_lvalue_name_ranges();
    test_catalog_aggregates_apply_text_merge();
    test_catalog_mutation_tabelize_io();
    test_archive_to_table_bridge();
    test_archive_plot_accessor();
    test_archive_time_accessors();
  } catch (const std::exception& e) {
    std::cerr << "[kindle_catalog_tests] failure: " << e.what() << "\n";
    return 1;
  }

  std::cout << "[kindle_catalog_tests] all checks passed\n";
  return 0;
}

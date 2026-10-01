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
  const auto values = s.as_series().to(pluss::codem::ScalarType::Float64).to_vec<double>();
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
  const auto cases = pluss::table::Thread(
      pluss::make_note(std::vector<std::string>{"cs_0", "cs_1", "cs_2", "cs_3", "cs_4"}, shape));

  return DataFrame({
                       {"id", pluss::table::Thread(ids)},
                       {"metric", pluss::table::Thread(metric, metric_mask)},
                       {"city", pluss::table::Thread(city)},
                       {"text", pluss::table::Thread(text)},
                       {"timestamp", pluss::table::Thread(timeline)},
                       {"time_index", pluss::table::Thread(indexed_timeline)},
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
  auto label = pluss::table::Thread(pluss::make_note(std::move(labels), shape));
  return Stream(pluss::make_series(std::move(values), {n}), std::nullopt, std::move(label));
}

Stream labeled_note(std::vector<std::string> values, std::vector<std::string> labels) {
  const auto n = static_cast<std::int64_t>(values.size());
  expect(static_cast<std::int64_t>(labels.size()) == n, "labeled_note label size mismatch");
  const std::array<std::int64_t, 1> shape{n};
  auto label = pluss::table::Thread(pluss::make_note(std::move(labels), shape));
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
  expect(filled.as_series().to_vec<double>() == std::vector<double>({10.0, 0.0, 30.0, 40.0, 0.0}),
         "Stream::fillnull(double) should preserve valid values and replace only null rows");
  const auto dropped = metric.dropnull();
  expect(dropped.size() == 3, "dropnull should keep only valid rows");
  const auto taken = metric.take(std::vector<std::int64_t>{0, 2, 4});
  expect(taken.size() == 3, "take size mismatch");
  const auto picked = Stream::select(
      {pluss::table::Thread(bool_mask({true, false, true}))},
      {labeled_series({10.0, 20.0, 30.0}, {"r0", "r1", "r2"})},
      labeled_series({1.0, 2.0, 3.0}, {"l0", "l1", "l2"}));
  expect(picked.as_series().to_vec<double>() == std::vector<double>({10.0, 2.0, 30.0}),
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
  expect(year.as_series().to_vec<std::int64_t>() == std::vector<std::int64_t>({2024, 2024, 2024, 2024, 2024}),
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
  expect(year.keys("timestamp").as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({2024, 2024, 2024, 2024, 2024}),
         "Catalog::keys().tl().year values mismatch");

  const auto filled = base.keys({"metric"}).fillnull(0.0);
  expect(filled.keys("metric").as_series().to_vec<double>() == std::vector<double>({10.0, 0.0, 30.0, 40.0, 0.0}),
         "Catalog::keys().fillnull should preserve valid values and replace only null rows");

  const auto dropped = base.keys({"metric"}).dropnull();
  expect(dropped.nrows() == 3, "Catalog::keys().dropnull should drop rows by selected key mask");
  expect(dropped.nkeys() == 1 && dropped.has("metric"),
         "Catalog::keys().dropnull should keep the projected key set");
  expect(dropped.keys("metric").as_series().to_vec<double>() == std::vector<double>({10.0, 30.0, 40.0}),
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
      pluss::table::ThreadCastTarget::Timeline,
      pluss::table::ThreadCastOptions{.format = "%Y-%m-%d", .invalid = pluss::table::ParseInvalid::Null});
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
  expect(key_sum.keys("sum").as_series().to_vec<double>() ==
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
  expect(inner.keys("sum").as_series().to_vec<double>() == std::vector<double>({11.0, 22.0, 33.0}),
         "Catalog::sum(Key, Inner) should trim to common length");

  const auto outer = ragged.sum(CatalogReduceOptions{.axis = ReduceAxis::Key, .align = pluss::kindle::Align::Outer});
  expect(outer.keys("sum").as_series().to_vec<double>() == std::vector<double>({11.0, 22.0, 33.0, 4.0, 5.0}),
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
  expect(sampled_a.keys("id").as_series().to_vec<std::int64_t>()
             == sampled_b.keys("id").as_series().to_vec<std::int64_t>(),
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
      {pluss::table::Thread(bool_mask({true, false, true, false, false}))},
      {replaced},
      base);
  expect(chosen.keys("id").as_series().to_vec<std::int64_t>() == std::vector<std::int64_t>({103, 1, 102, 5, 4}),
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
  expect(note_values(inner.label().values().cast().note().as_note())
             == std::vector<std::string>({"A", "C"}),
         "Catalog::tabelize(label, inner) should preserve anchor label order");
  expect(inner.features("right").as_series().to_vec<double>() == std::vector<double>({10.0, 30.0}),
         "Catalog::tabelize(label, inner) should align values by label");

  policy.align = pluss::kindle::Align::Outer;
  const auto outer = labeled.tabelize(policy);
  expect(outer.ncases() == 4, "Catalog::tabelize(label, outer) should take label union");
  expect(note_values(outer.label().values().cast().note().as_note())
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
  const auto id_sorted = sorted.keys("id").as_series().to_vec<std::int64_t>();
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
  expect(filled.keys("metric").as_series().to_vec<double>() == std::vector<double>({10.0, 0.0, 30.0, 40.0, 0.0}),
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
      pluss::table::ThreadCastOptions{.series_dtype = pluss::codem::ScalarType::Int64}).series();
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
  expect(filled.keys("metric").as_series().to_vec<double>() == std::vector<double>({10.0, 0.0, 30.0, 40.0, 0.0}),
         "Catalog lvalue fillnull should delegate to selected keys");

  const auto casted = base.keys(keys).cast().note();
  expect(casted.keys("id").kind() == StreamKind::Note, "Catalog lvalue cast should delegate to selected keys");

  const auto f0 = make_base_frame().slice(0, 3);
  const auto f1 = make_base_frame().slice(2, 5);
  const auto labels = pluss::table::Thread(
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
  expect_throw([&]() { (void)base.merge(right, "id", "left"); }, "merge(how!=inner) should throw");

  const auto applied = base.keys({"id"}).map([](const Stream& s) { return s + 10.0; });
  const auto applied_values = applied.keys("id").as_series().to_vec<std::int64_t>();
  expect(applied_values[0] == 13, "keys().map should transform target field");

  const auto transformed_all = base.transform([](const Stream& s) { return s; });
  const auto transformed_one = base.keys({"id"}).map([](const Stream& s) { return s + 1.0; });
  expect(transformed_all.nrows() == base.nrows(), "transform(all) should preserve row count");
  expect(transformed_one.keys("id").as_series().to_vec<std::int64_t>()[0] == 4,
         "keys().map should transform target field");

  const auto updated_id_only = base.keys({"id"}).map([](const Stream& s) { return s - 1.0; });
  expect(updated_id_only.keys("id").as_series().to_vec<std::int64_t>()[0] == 2,
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
  const auto labels = pluss::table::Thread(
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
  const auto sorted_labels = sorted_desc.label().values().to_thread().cast().note().as_note();
  expect(std::string(sorted_labels.at(0)) == "batch_b", "Archive::sort_depth(desc) label mismatch");

  const auto relabeled = archive.label().set(
      Stream(pluss::make_note(std::vector<std::string>{"A", "B"}, std::array<std::int64_t, 1>{2})));
  expect(relabeled.label().has(), "Archive::label().set should set labels");

  const auto aligned = archive.label().align(
      Stream(pluss::make_note(std::vector<std::string>{"batch_b", "batch_a"}, std::array<std::int64_t, 1>{2})));
  const auto aligned_first = aligned.catalog(0).keys("id").as_series().to_vec<std::int64_t>();
  expect(aligned_first == f1.features("id").as_series().to_vec<std::int64_t>(),
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
  expect(filled.catalog(0).keys("metric").as_series().to_vec<double>()[1] == 0.0,
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
  expect(years.catalog(0).keys("timestamp").as_series().to_vec<std::int64_t>()
             == std::vector<std::int64_t>({2024, 2024, 2024}),
         "Archive::keys(...).tl().year should preserve first ragged catalog length");
  expect(years.catalog(1).keys("timestamp").as_series().to_vec<std::int64_t>()
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

#include <pluss/pluss.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <string>
#include <vector>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

namespace {

using namespace pluss::pl;
using Catch::Approx;
using pluss::ScalarType;
using pluss::Series;

std::vector<double> values(const Series& series) {
  return series.to(ScalarType::Float64).host<double>().array().data;
}

std::vector<double> ramp(std::size_t n) {
  std::vector<double> x(n);
  std::iota(x.begin(), x.end(), 1.0);
  return x;
}

Studio lines_figure() {
  Studio fig("lines");
  auto& pane = fig.pane2();
  const auto x = ramp(40);
  std::vector<double> a(40), b(40), c(40);
  for (std::size_t i = 0; i < 40; ++i) {
    a[i] = 100.0 + static_cast<double>(i) + std::sin(static_cast<double>(i));
    b[i] = 50.0 + 2.0 * static_cast<double>(i);
    c[i] = 200.0 - static_cast<double>(i);
  }
  pane.trend(x, a).label("alpha");
  pane.trend(x, b).label("beta");
  pane.trend(x, c).label("gamma");
  pane.x_label("t").y_label("price");
  return fig;
}

Studio bars_figure() {
  Studio fig("bars");
  auto& pane = fig.pane2();
  pane.columns(std::vector<double>{1, 2, 3, 4}, std::vector<double>{30, 10, 40, 20}).label("sales");
  pane.x_categories({"a", "b", "c", "d"});
  return fig;
}

Studio hist_figure() {
  Studio fig("hist");
  auto& pane = fig.pane2();
  std::vector<double> v(200);
  for (std::size_t i = 0; i < v.size(); ++i) {
    v[i] = std::sin(static_cast<double>(i) * 0.37) * 10.0 + static_cast<double>(i % 17);
  }
  pane.bucket(v, 10);
  return fig;
}

Studio scatter_figure() {
  Studio fig("scatter");
  auto& pane = fig.pane2();
  std::vector<double> x(120), y(120);
  for (std::size_t i = 0; i < x.size(); ++i) {
    x[i] = static_cast<double>(i) / 10.0;
    y[i] = 2.0 * x[i] + std::sin(static_cast<double>(i));
  }
  y[7] = 90.0;
  pane.cloud(x, y).label("points");
  return fig;
}

bool has_group(const std::vector<views::Group>& groups, const std::string& id) {
  return std::any_of(groups.begin(), groups.end(), [&](const views::Group& g) { return g.id == id; });
}

const views::Group& group(const std::vector<views::Group>& groups, const std::string& id) {
  return *std::find_if(groups.begin(), groups.end(), [&](const views::Group& g) { return g.id == id; });
}

const Trace& trace_at(const Studio& fig, std::size_t pane, std::size_t index) {
  return fig.panes()[pane].traces()[index];
}

const TrendData& trend_of(const Trace& trace) { return std::get<TrendData>(trace.payload()); }

} // namespace

TEST_CASE("views: only the groups that make sense for a figure are offered", "[plusspl][views]") {
  const auto lines = views::groups(lines_figure());
  for (const char* id : {"theme", "colors", "focus", "isolate", "layout", "transform", "smooth", "draw", "guides",
                         "marks", "stats", "scale", "range", "window", "legend", "grid"}) {
    INFO(id);
    CHECK(has_group(lines, id));
  }
  for (const char* id : {"bins", "counts", "curve", "fit", "sort", "orient", "outliers", "shade", "dots"}) {
    INFO(id);
    CHECK_FALSE(has_group(lines, id));
  }

  const auto bars = views::groups(bars_figure());
  CHECK(has_group(bars, "sort"));
  CHECK(has_group(bars, "orient"));
  CHECK_FALSE(has_group(bars, "focus"));   // one series
  CHECK_FALSE(has_group(bars, "smooth"));  // bars are not a time series
  CHECK_FALSE(has_group(bars, "scale"));   // a bar's base sits at zero: no log axis

  const auto hist = views::groups(hist_figure());
  CHECK(has_group(hist, "bins"));
  CHECK(has_group(hist, "counts"));
  CHECK(has_group(hist, "curve"));
  CHECK_FALSE(has_group(hist, "smooth"));
  CHECK_FALSE(has_group(hist, "transform"));

  const auto scatter = views::groups(scatter_figure());
  CHECK(has_group(scatter, "fit"));
  CHECK(has_group(scatter, "outliers"));
  CHECK(has_group(scatter, "shade"));
  CHECK(has_group(scatter, "dots"));
  CHECK_FALSE(has_group(scatter, "smooth"));  // a scatter is not an ordered series
}

TEST_CASE("views: every group lists its default option first and offers at least two", "[plusspl][views]") {
  for (const Studio& fig : {lines_figure(), bars_figure(), hist_figure(), scatter_figure()}) {
    for (const auto& g : views::groups(fig)) {
      INFO(g.id);
      REQUIRE(g.options.size() >= 2);
      CHECK_FALSE(g.options.front().id.empty());
      CHECK_FALSE(g.title.empty());
    }
  }
}

TEST_CASE("views: no choices, a default choice, or an unknown one leave the figure as drawn", "[plusspl][views]") {
  const Studio fig = lines_figure();
  const std::string original = fig.to_svg();
  CHECK(views::apply(fig, {}).to_svg() == original);
  CHECK(views::apply(fig, {{"smooth", "raw"}}).to_svg() == original);
  CHECK(views::apply(fig, {{"nonsense", "x"}, {"smooth", "no-such-option"}}).to_svg() == original);
  CHECK(views::apply(fig, {{"bins", "b5"}}).to_svg() == original);  // does not apply to a line figure
}

TEST_CASE("views: every option of every group renders", "[plusspl][views]") {
  for (const Studio& fig : {lines_figure(), bars_figure(), hist_figure(), scatter_figure()}) {
    for (const auto& g : views::groups(fig)) {
      for (const auto& option : g.options) {
        INFO(fig.name() << " / " << g.id << " / " << option.id);
        const Studio shown = views::apply(fig, {{g.id, option.id}});
        CHECK_FALSE(shown.to_svg().empty());
      }
    }
  }
}

TEST_CASE("views: transforms rewrite the values", "[plusspl][views]") {
  const Studio fig = lines_figure();
  const auto original = values(trend_of(trace_at(fig, 0, 1)).y);

  const auto index = views::apply(fig, {{"transform", "index"}});
  const auto idx = values(trend_of(trace_at(index, 0, 1)).y);
  REQUIRE(idx.size() == original.size());
  CHECK(idx.front() == Approx(100.0));
  CHECK(idx[10] == Approx(original[10] / original[0] * 100.0));
  CHECK(index.panes()[0].y_axis().label.find("index") != std::string::npos);

  const auto z = values(trend_of(trace_at(views::apply(fig, {{"transform", "zscore"}}), 0, 0)).y);
  const double mean = std::accumulate(z.begin(), z.end(), 0.0) / static_cast<double>(z.size());
  double ss = 0.0;
  for (const double v : z) ss += (v - mean) * (v - mean);
  CHECK(mean == Approx(0.0).margin(1e-9));
  CHECK(std::sqrt(ss / static_cast<double>(z.size() - 1)) == Approx(1.0));

  const auto diff = views::apply(fig, {{"transform", "diff"}});
  const auto d = values(trend_of(trace_at(diff, 0, 1)).y);
  CHECK(d.size() == original.size() - 1);  // a difference has one value fewer
  CHECK(d.front() == Approx(2.0));
  CHECK(values(trend_of(trace_at(diff, 0, 1)).x).front() == Approx(2.0));

  const auto cum = values(trend_of(trace_at(views::apply(fig, {{"transform", "cumsum"}}), 0, 1)).y);
  CHECK(cum.back() == Approx(std::accumulate(original.begin(), original.end(), 0.0)));

  const auto mm = values(trend_of(trace_at(views::apply(fig, {{"transform", "minmax"}}), 0, 1)).y);
  CHECK(*std::min_element(mm.begin(), mm.end()) == Approx(0.0));
  CHECK(*std::max_element(mm.begin(), mm.end()) == Approx(1.0));
}

TEST_CASE("views: smoothing averages neighbours and keeps the length", "[plusspl][views]") {
  const Studio fig = lines_figure();
  const auto raw = values(trend_of(trace_at(fig, 0, 0)).y);
  const auto ma = values(trend_of(trace_at(views::apply(fig, {{"smooth", "ma3"}}), 0, 0)).y);
  REQUIRE(ma.size() == raw.size());
  CHECK(ma[5] == Approx((raw[4] + raw[5] + raw[6]) / 3.0));
  CHECK(ma.front() == Approx((raw[0] + raw[1]) / 2.0));  // an edge averages the neighbours it has

  const auto ema = values(trend_of(trace_at(views::apply(fig, {{"smooth", "ema_fast"}}), 0, 1)).y);
  const auto beta = values(trend_of(trace_at(fig, 0, 1)).y);
  CHECK(ema.front() == Approx(beta.front()));
  CHECK(ema[1] == Approx(0.4 * beta[1] + 0.6 * beta[0]));

  // The raw line can be kept faintly behind the smoothed one; it belongs to the view, not to the figure.
  const auto ghost = views::apply(fig, {{"smooth", "ma7"}, {"ghost", "faint"}});
  CHECK(ghost.panes()[0].trace_count() == 6);
  CHECK(ghost.panes()[0].traces()[0].label_text().empty());
}

TEST_CASE("views: window keeps the tail or the head of the series", "[plusspl][views]") {
  const Studio fig = lines_figure();
  const auto last = views::apply(fig, {{"window", "last25"}});
  const auto x = values(trend_of(trace_at(last, 0, 0)).x);
  CHECK(x.back() == Approx(40.0));
  CHECK(x.front() >= 30.0);
  CHECK(x.size() <= 12);
  const auto first = values(trend_of(trace_at(views::apply(fig, {{"window", "first50"}}), 0, 0)).x);
  CHECK(first.front() == Approx(1.0));
  CHECK(first.back() <= 21.0);
}

TEST_CASE("views: isolate and focus act on the series of a pane", "[plusspl][views]") {
  const Studio fig = lines_figure();
  const auto only = views::apply(fig, {{"isolate", "t1"}});
  REQUIRE(only.panes()[0].trace_count() == 1);
  CHECK(only.panes()[0].traces()[0].label_text() == "beta");

  const auto focused = views::apply(fig, {{"focus", "t2"}});
  REQUIRE(focused.panes()[0].trace_count() == 3);
  CHECK(trace_at(focused, 0, 2).stroke_color().alpha == Approx(1.0));
  CHECK(trace_at(focused, 0, 0).stroke_color().alpha < 0.5);
  CHECK(trace_at(focused, 0, 2).stroke_width() > trace_at(focused, 0, 0).stroke_width());

  const auto groups = views::groups(fig);
  CHECK(group(groups, "isolate").options.size() == 4);  // all + one per series
  CHECK(group(groups, "focus").options[1].label == "alpha");
}

TEST_CASE("views: layout splits one pane into a pane per series", "[plusspl][views]") {
  const Studio fig = lines_figure();
  const auto stacked = views::apply(fig, {{"layout", "stack"}});
  REQUIRE(stacked.pane_count() == 3);
  for (std::size_t i = 0; i < 3; ++i) {
    CHECK(stacked.panes()[i].trace_count() == 1);
  }
  CHECK(stacked.panes()[1].title_text() == "beta");
  CHECK(stacked.mosaic_layout().rows == 3);
  CHECK(stacked.mosaic_layout().cols == 1);
  CHECK(views::apply(fig, {{"layout", "grid"}}).mosaic_layout().cols == 2);

  // A figure that already has several panes is left alone.
  CHECK_FALSE(has_group(views::groups(stacked), "layout"));
}

TEST_CASE("views: sort and orientation keep each bar with its label", "[plusspl][views]") {
  const Studio fig = bars_figure();
  const auto desc = views::apply(fig, {{"sort", "desc"}});
  const auto& bars = std::get<ColumnsData>(trace_at(desc, 0, 0).payload());
  CHECK(values(bars.y) == std::vector<double>{40, 30, 20, 10});
  const auto& labels = desc.panes()[0].x_axis().ticks->labels;
  CHECK(labels == std::vector<std::string>{"c", "a", "d", "b"});

  const auto az = views::apply(fig, {{"sort", "za"}});
  CHECK(az.panes()[0].x_axis().ticks->labels == std::vector<std::string>{"d", "c", "b", "a"});
  CHECK(values(std::get<ColumnsData>(trace_at(az, 0, 0).payload()).y) == std::vector<double>{20, 40, 10, 30});

  const auto flipped = views::apply(fig, {{"orient", "flip"}});
  const auto& h = std::get<ColumnsData>(trace_at(flipped, 0, 0).payload());
  CHECK(h.horizontal);
  CHECK(values(h.x) == std::vector<double>{30, 10, 40, 20});  // the values now run along x
  REQUIRE(flipped.panes()[0].y_axis().ticks.has_value());     // the category labels moved with them
  CHECK_FALSE(flipped.panes()[0].x_axis().ticks.has_value());
}

TEST_CASE("views: histograms change bins, what the bars measure, and gain a curve", "[plusspl][views]") {
  const Studio fig = hist_figure();
  const auto five = views::apply(fig, {{"bins", "b5"}});
  CHECK(std::get<BucketData>(trace_at(five, 0, 0).payload()).bins == 5);
  const auto automatic = std::get<BucketData>(trace_at(views::apply(fig, {{"bins", "auto"}}), 0, 0).payload()).bins;
  CHECK(automatic >= 5);
  CHECK(automatic <= 80);

  CHECK(std::get<BucketData>(trace_at(views::apply(fig, {{"counts", "density"}}), 0, 0).payload()).scale == BucketScale::Density);
  CHECK(std::get<BucketData>(trace_at(views::apply(fig, {{"counts", "cdf"}}), 0, 0).payload()).scale ==
        BucketScale::CumulativeDensity);

  const auto curved = views::apply(fig, {{"curve", "kde"}, {"counts", "density"}});
  REQUIRE(curved.panes()[0].trace_count() == 2);
  const auto& curve = trend_of(trace_at(curved, 0, 1));
  const auto cx = values(curve.x);
  const auto cy = values(curve.y);
  double area = 0.0;
  for (std::size_t i = 1; i < cx.size(); ++i) {
    area += 0.5 * (cy[i] + cy[i - 1]) * (cx[i] - cx[i - 1]);
  }
  CHECK(area > 0.8);   // a density curve encloses about unit area (the data's own range clips its tails)
  CHECK(area < 1.05);
  CHECK(trace_at(curved, 0, 1).label_text() == "density curve");

  // The same figure's count curve sits at the height of the bars: n * bin width * density.
  const auto counted = views::apply(fig, {{"curve", "normal"}});
  const auto top = values(trend_of(trace_at(counted, 0, 1)).y);
  CHECK(*std::max_element(top.begin(), top.end()) > 1.0);
}

TEST_CASE("views: a histogram's scale changes what the renderer draws", "[plusspl][views]") {
  const Studio fig = hist_figure();
  CHECK(views::apply(fig, {{"counts", "density"}}).to_svg() != fig.to_svg());
  CHECK(views::apply(fig, {{"counts", "cumulative"}}).to_svg() != fig.to_svg());
}

TEST_CASE("views: scatter fits, outliers and shading", "[plusspl][views]") {
  const Studio fig = scatter_figure();
  const auto fitted = views::apply(fig, {{"fit", "linear"}});
  REQUIRE(fitted.panes()[0].trace_count() == 2);
  CHECK(fitted.panes()[0].traces()[1].kind() == TraceKind::Trend);
  const auto fit = trend_of(trace_at(fitted, 0, 1));
  const auto fy = values(fit.y);
  const auto fx = values(fit.x);
  CHECK(fx.size() == 120);
  CHECK(fy.back() > fy.front());

  const auto flagged = views::apply(fig, {{"outliers", "highlight"}});
  REQUIRE(flagged.panes()[0].trace_count() == 2);
  CHECK(trace_at(flagged, 0, 1).label_text().find("outliers") == 0);
  const auto& kept = std::get<CloudData>(trace_at(flagged, 0, 0).payload());
  const auto& found = std::get<CloudData>(trace_at(flagged, 0, 1).payload());
  CHECK(kept.x.numel() + found.x.numel() == 120);
  const auto fy_out = values(found.y);
  CHECK(std::find(fy_out.begin(), fy_out.end(), 90.0) != fy_out.end());

  const auto hidden = views::apply(fig, {{"outliers", "remove"}});
  CHECK(hidden.panes()[0].trace_count() == 1);
  CHECK(std::get<CloudData>(trace_at(hidden, 0, 0).payload()).x.numel() < 120);

  // A fit made after the outliers were set aside is not pulled towards them.
  const auto robust = views::apply(fig, {{"outliers", "remove"}, {"fit", "linear"}});
  const auto robust_y = values(trend_of(trace_at(robust, 0, 1)).y);
  CHECK(robust_y.back() == Approx(2.0 * 11.9).margin(1.5));

  const auto shaded = views::apply(fig, {{"shade", "on"}});
  CHECK(std::get<CloudData>(trace_at(shaded, 0, 0).payload()).color.has_value());
}

TEST_CASE("views: guides, marks and statistics annotate the pane", "[plusspl][views]") {
  const Studio fig = lines_figure();
  const auto mean = views::apply(fig, {{"guides", "mean"}});
  CHECK(mean.panes()[0].rules().size() == 3);  // one per series
  const auto band = views::apply(fig, {{"guides", "sd2"}});
  CHECK(band.panes()[0].bands().size() == 3);
  CHECK(band.panes()[0].rules().size() == 3);

  const auto extremes = views::apply(fig, {{"marks", "extremes"}});
  CHECK(extremes.panes()[0].pins().size() == 6);
  CHECK(views::apply(fig, {{"marks", "last"}}).panes()[0].pins().size() == 3);
  CHECK(views::apply(fig, {{"marks", "ends"}}).panes()[0].pins().size() == 6);

  const auto stats = views::apply(fig, {{"stats", "summary"}});
  CHECK(stats.panes()[0].caption_text().find("mean") != std::string::npos);

  // Focus passes the annotation to the one series in focus.
  const auto one = views::apply(fig, {{"focus", "t0"}, {"guides", "mean"}, {"marks", "last"}});
  CHECK(one.panes()[0].rules().size() == 1);
  CHECK(one.panes()[0].pins().size() == 1);
}

TEST_CASE("views: draw converts the kind of trace", "[plusspl][views]") {
  const Studio fig = lines_figure();
  CHECK(trace_at(views::apply(fig, {{"draw", "area"}}), 0, 0).kind() == TraceKind::Ribbon);
  CHECK(trace_at(views::apply(fig, {{"draw", "stems"}}), 0, 0).kind() == TraceKind::Spike);
  CHECK(trace_at(views::apply(fig, {{"draw", "points"}}), 0, 0).kind() == TraceKind::Cloud);
  const auto step = views::apply(fig, {{"draw", "step"}});
  CHECK(trend_of(trace_at(step, 0, 0)).x.numel() == 2 * 40 - 1);
  const auto both = views::apply(fig, {{"draw", "linepoints"}});
  CHECK(both.panes()[0].trace_count() == 6);
  CHECK(both.panes()[0].traces()[1].kind() == TraceKind::Cloud);
}

TEST_CASE("views: scale offers log axes only where the data allow them", "[plusspl][views]") {
  const auto groups = views::groups(lines_figure());
  const auto& scale = group(groups, "scale");
  const auto has = [&](const char* id) {
    return std::any_of(scale.options.begin(), scale.options.end(), [&](const views::Option& o) { return o.id == id; });
  };
  CHECK(has("ylog"));
  CHECK(has("loglog"));
  CHECK(has("symlog"));

  Studio negative("neg");
  negative.pane2().trend(std::vector<double>{1, 2, 3, 4}, std::vector<double>{-1, 2, -3, 4});
  const auto& neg = group(views::groups(negative), "scale");
  CHECK(std::none_of(neg.options.begin(), neg.options.end(), [](const views::Option& o) { return o.id == "ylog"; }));

  const Studio logged = views::apply(lines_figure(), {{"scale", "ylog"}});
  CHECK(logged.panes()[0].y_axis().scale.kind == ScaleKind::Log10);

  // A transform that makes the values negative falls back to the symmetric log rather than drawing nothing.
  const Studio fallback = views::apply(lines_figure(), {{"transform", "zscore"}, {"scale", "ylog"}});
  CHECK(fallback.panes()[0].y_axis().scale.kind == ScaleKind::SymLog);
}

TEST_CASE("views: choices from different groups compose", "[plusspl][views]") {
  const Studio fig = lines_figure();
  const auto all = views::apply(fig, {{"smooth", "ma3"}, {"transform", "index"}, {"scale", "ylog"}, {"colors", "pastel"},
                                      {"guides", "mean"}, {"legend", "off"}});
  const auto idx = values(trend_of(trace_at(all, 0, 1)).y);
  const auto raw = values(trend_of(trace_at(fig, 0, 1)).y);
  CHECK(idx.front() == Approx((100.0 + raw[1] / raw[0] * 100.0) / 2.0));  // transformed ...
  CHECK(idx[5] == Approx((raw[4] + raw[5] + raw[6]) / 3.0 / raw[0] * 100.0));  // ... and smoothed
  CHECK(all.panes()[0].y_axis().scale.kind == ScaleKind::Log10);  // ... on a log axis
  CHECK(all.panes()[0].rules().size() == 3);                     // ... with the mean marked
  CHECK(all.panes()[0].legend_place() == KeyPlace::None);        // ... and no legend
  CHECK(trace_at(all, 0, 0).stroke_color().red != trace_at(fig, 0, 0).stroke_color().red);  // ... in other colours

  // Order of the choices in the map does not matter: the groups run in a fixed order.
  const auto reversed = views::apply(fig, {{"legend", "off"}, {"guides", "mean"}, {"colors", "pastel"}, {"scale", "ylog"},
                                           {"transform", "index"}, {"smooth", "ma3"}});
  CHECK(reversed.to_svg() == all.to_svg());
}

TEST_CASE("views: looks change theme, legend, grid, shape and text", "[plusspl][views]") {
  const Studio fig = lines_figure();
  const auto midnight = views::apply(fig, {{"theme", "midnight"}});
  CHECK(midnight.panes()[0].theme_tokens().background().red < 0.2);
  CHECK(midnight.theme_tokens().name() == "midnight");

  CHECK(views::apply(fig, {{"legend", "outside"}}).panes()[0].legend_place() == KeyPlace::OutsideEast);
  CHECK(views::apply(fig, {{"grid", "major"}}).panes()[0].grid_style() == GridStyle::Major);

  const auto wide = views::apply(fig, {{"aspect", "wide"}});
  CHECK(wide.extent().width / wide.extent().height == Approx(16.0 / 9.0).epsilon(0.01));
  const auto square = views::apply(fig, {{"aspect", "square"}});
  CHECK(square.extent().width == Approx(square.extent().height).margin(1.0));
  CHECK(views::apply(fig, {{"text", "large"}}).panes()[0].tick_font_scale() > fig.panes()[0].tick_font_scale());

  // A named fit or curve gets a legend when the figure had none.
  const auto fitted = views::apply(scatter_figure(), {{"fit", "linear"}});
  CHECK(fitted.panes()[0].legend_place() != KeyPlace::None);
  const auto quiet = views::apply(scatter_figure(), {{"fit", "linear"}, {"legend", "off"}});
  CHECK(quiet.panes()[0].legend_place() == KeyPlace::None);
}

TEST_CASE("views: the original figure is never changed", "[plusspl][views]") {
  const Studio fig = lines_figure();
  const std::string before = fig.to_svg();
  (void)views::apply(fig, {{"layout", "stack"}, {"isolate", "t0"}, {"transform", "diff"}, {"theme", "midnight"}});
  CHECK(fig.to_svg() == before);
  CHECK(fig.pane_count() == 1);
  CHECK(fig.panes()[0].trace_count() == 3);
}

TEST_CASE("views: odd figures never throw and offer only what applies", "[plusspl][views]") {
  std::vector<Studio> figures;

  figures.emplace_back("empty");  // no panes at all

  Studio blank("blank");
  blank.pane2("nothing here");
  figures.push_back(blank);

  Studio single("single");
  single.pane2().trend(std::vector<double>{1.0}, std::vector<double>{5.0}).label("one point");
  figures.push_back(single);

  Studio gaps("gaps");
  gaps.pane2().trend(ramp(30), [] {
    std::vector<double> y(30, 3.0);  // constant, with holes
    y[4] = std::numeric_limits<double>::quiet_NaN();
    y[17] = std::numeric_limits<double>::infinity();
    return y;
  }()).label("flat");
  figures.push_back(gaps);

  Studio same("same");
  same.pane2().bucket(std::vector<double>(50, 2.5), 6);  // every value equal
  figures.push_back(same);

  Studio pie("pie");
  pie.pane2().wedge(std::vector<double>{3.0, 1.0, 2.0});
  figures.push_back(pie);

  Studio cube("cube");
  cube.pane3().cloud3(std::vector<double>{1, 2, 3}, std::vector<double>{3, 2, 1}, std::vector<double>{1, 3, 2});
  figures.push_back(cube);

  for (const Studio& fig : figures) {
    for (const auto& g : views::groups(fig)) {
      for (const auto& option : g.options) {
        INFO(fig.name() << " / " << g.id << " / " << option.id);
        CHECK_NOTHROW(views::apply(fig, {{g.id, option.id}}).to_svg());
      }
    }
  }

  // What is offered follows the figure: a pie only has the looks, a cube too.
  for (const Studio* fig : {&figures[5], &figures[6]}) {
    for (const auto& g : views::groups(*fig)) {
      INFO(g.id);
      CHECK((g.id == "theme" || g.id == "colors" || g.id == "aspect" || g.id == "text"));
    }
  }
  CHECK(views::groups(figures[0]).empty());
}

#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdint>
#include <string>
#include <vector>

#include <pluss/pluss.h>

namespace {

using pluss::table::DataFrame;
using pluss::table::Thread;

void section(const std::string& name) {
  std::cout << "\n=== " << name << " ===\n";
}

DataFrame sales_frame() {
  return DataFrame({
      {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
      {"region", pluss::make_note({"south", "south", "middle", "north"})},
      {"units", pluss::make_series({10.0, 7.0, 12.0, 5.0})},
      {"price", pluss::make_series({3.0, 4.0, 2.5, 5.0})},
  }).title("sales");
}

pluss::Series bool_series(std::vector<bool> values) {
  auto out = pluss::Series::zeros({static_cast<std::int64_t>(values.size())},
                                  pluss::codem::ScalarType::Bool);
  auto* ptr = reinterpret_cast<bool*>(out.raw_data());
  for (std::size_t i = 0; i < values.size(); ++i) {
    ptr[i] = values[i];
  }
  return out;
}

std::string trace_kind_name(pluss::pl::TraceKind kind) {
  switch (kind) {
    case pluss::pl::TraceKind::Trend: return "Trend";
    case pluss::pl::TraceKind::Cloud: return "Cloud";
    case pluss::pl::TraceKind::Cloud3: return "Cloud3";
    case pluss::pl::TraceKind::Fieldmap: return "Fieldmap";
    case pluss::pl::TraceKind::Ribbon: return "Ribbon";
    case pluss::pl::TraceKind::Spike: return "Spike";
    case pluss::pl::TraceKind::Wedge: return "Wedge";
    case pluss::pl::TraceKind::Whisk: return "Whisk";
    case pluss::pl::TraceKind::Band: return "Band";
    case pluss::pl::TraceKind::Columns: return "Columns";
    case pluss::pl::TraceKind::Bucket: return "Bucket";
    case pluss::pl::TraceKind::Quartet: return "Quartet";
    case pluss::pl::TraceKind::Violin: return "Violin";
    case pluss::pl::TraceKind::Radial: return "Radial";
    case pluss::pl::TraceKind::Contour: return "Contour";
    case pluss::pl::TraceKind::Isoband: return "Isoband";
    case pluss::pl::TraceKind::Ridge3: return "Ridge3";
    case pluss::pl::TraceKind::IsoShell: return "IsoShell";
    case pluss::pl::TraceKind::Path3: return "Path3";
    case pluss::pl::TraceKind::Spire3: return "Spire3";
    case pluss::pl::TraceKind::Sheet: return "Sheet";
    case pluss::pl::TraceKind::Lattice: return "Lattice";
  }
  return "Unknown";
}

void print_figure_summary(const std::string& label, const pluss::pl::Studio& figure) {
  std::cout << label << ": panes=" << figure.pane_count();
  if (!figure.panes().empty()) {
    const auto& pane = figure.panes().front();
    std::cout << ", first pane traces=" << pane.trace_count();
    if (!pane.traces().empty()) {
      std::cout << ", first trace=" << trace_kind_name(pane.traces().front().kind());
    }
  }
  std::cout << '\n';
}

void example_01_construct_and_inspect() {
  section("01 construct and inspect");
  auto sales = sales_frame();

  std::cout << sales.to_string() << '\n';
  std::cout << "ncases=" << sales.ncases() << ", nfeatures=" << sales.nfeatures() << '\n';
  std::cout << "features:";
  for (const auto& name : sales.features()) {
    std::cout << ' ' << name;
  }
  std::cout << '\n';
}

void example_02_select_and_reduce() {
  section("02 select and reduce");
  auto sales = sales_frame();
  auto numeric = sales.series_features();

  std::cout << numeric.to_string() << '\n';
  std::cout << "units mean=" << sales.features("units").mean().item<double>() << '\n';
  std::cout << "price max=" << sales.features("price").max().item<double>() << '\n';
}

void example_03_filter_and_assign() {
  section("03 filter and assign");
  auto sales = sales_frame();
  auto enriched = sales.assign({{
      "revenue",
      [](const DataFrame& frame) {
        return frame.features("units") * frame.features("price");
      },
  }});

  auto high_value = enriched.query("revenue >= 30");
  std::cout << high_value.to_string() << '\n';
}

void example_04_group_and_aggregate() {
  section("04 group and aggregate");
  auto sales = sales_frame().assign({{
      "revenue",
      [](const DataFrame& frame) {
        return frame.features("units") * frame.features("price");
      },
  }});

  auto summary = sales.aggregate({
      {"total_units", "units", DataFrame::SummaryOp::Sum},
      {"mean_price", "price", DataFrame::SummaryOp::Mean},
      {"max_revenue", "revenue", DataFrame::SummaryOp::Max},
  }, false);
  std::cout << summary.to_string() << '\n';

  auto grouped = sales.group_by(
      {"region"},
      {
          {"units_sum", "units", DataFrame::GroupOp::Sum},
          {"revenue_mean", "revenue", DataFrame::GroupOp::Mean},
      },
      true,
      false);
  std::cout << grouped.to_string() << '\n';
}

void example_05_missing_data() {
  section("05 missing data");
  const std::vector<std::vector<std::string>> records = {
      {"id", "city", "units", "price"},
      {"A001", "Accra", "10", "3.0"},
      {"A002", "Tema", "", "4.0"},
      {"A003", "", "12", ""},
  };

  pluss::loader::ReadCsvOptions options;
  options.case_col = "id";
  auto df = pluss::loader::load_csv_records(records, options);

  std::cout << df.to_string() << '\n';
  std::cout << "null rows=" << df.count_null_rows() << '\n';
  std::cout << df.isna().to_string() << '\n';
  std::cout << df.fillna(0.0, {"units", "price"}).fillna("unknown", {"city"}).to_string() << '\n';
}

void example_06_csv_options(const std::filesystem::path& dir) {
  section("06 csv options");
  const auto path = dir / "plussmic_manual_sales.csv";
  {
    std::ofstream out(path);
    out << "id;city;region;units;price\n";
    out << "A001;Accra;south;10;3.0\n";
    out << "A002;Tema;south;7;4.0\n";
    out << "A003;Kumasi;middle;12;2.5\n";
    out << "A004;Tamale;north;5;5.0\n";
  }

  pluss::loader::ReadCsvOptions options;
  options.sep = ';';
  options.case_col = "id";
  options.usefeats = {"city", "units", "price"};
  options.ncases = 3;

  auto df = DataFrame::load_csv(path.string(), options);
  std::cout << df.to_string() << '\n';
  std::cout << "case dtype=" << df.label().values().dtype() << '\n';
}

void example_07_thread_operations() {
  section("07 thread operations");
  auto sales = sales_frame();
  auto units = sales.features("units");
  auto city = sales.features("city");

  std::cout << "units dtype=" << units.dtype() << '\n';
  std::cout << "units clipped:\n" << units.clip(6.0, 10.0).to_string() << '\n';
  std::cout << "city lower:\n" << city.lower().to_string() << '\n';
  std::cout << "city contains 'a':\n" << Thread(city.contains("a")).to_string() << '\n';
}

void example_08_atlas_and_conversion() {
  section("08 atlas and conversion");
  auto sales = sales_frame();
  auto south = sales.query("region == 'south'");
  auto other = sales.query("region != 'south'");
  Thread labels = pluss::make_note({"south", "other"});

  pluss::table::Atlas atlas({south, other}, labels);
  std::cout << "depth=" << atlas.depth() << ", ncases=" << atlas.ncases() << ", nfeatures=" << atlas.nfeatures() << '\n';
  std::cout << atlas.loc("south").to_string() << '\n';

  auto catalog = sales.kindelize();
  auto roundtrip = catalog.tabelize();
  std::cout << "roundtrip equals original=" << (roundtrip.equals(sales) ? "true" : "false") << '\n';
}

void example_09_sql() {
  section("09 sql");
  auto sales = sales_frame().assign({{
      "revenue",
      [](const DataFrame& frame) {
        return frame.features("units") * frame.features("price");
      },
  }});

  auto top = sales.sql().execute(
      "SELECT city, revenue FROM df WHERE revenue >= 28 ORDER BY revenue DESC LIMIT 2");
  std::cout << top.to_string() << '\n';

  DataFrame::SqlOptions options;
  options.params["min_revenue"] = std::int64_t{28};
  options.identifier_params["value_col"] = "revenue";
  auto parameterized = sales.sql(std::move(options)).execute(
      "SELECT city, {@value_col} FROM df WHERE revenue >= {min_revenue} ORDER BY revenue DESC LIMIT 3");
  std::cout << parameterized.to_string() << '\n';

  auto prepared = sales.prepare_sql(
      "SELECT city FROM df WHERE revenue >= {min_revenue} ORDER BY revenue DESC LIMIT {limit_n}");
  auto prepared_result = prepared.execute({
      {"min_revenue", std::int64_t{28}},
      {"limit_n", std::int64_t{2}},
  });
  std::cout << prepared_result.to_string() << '\n';
}

void example_10_shape_and_schema() {
  section("10 shape and schema");
  auto sales = sales_frame();

  auto sorted = sales.sort_values({DataFrame::ArrangeSpec{"price", false, true}})
                    .features({"city", "price"});
  std::cout << sorted.to_string() << '\n';

  std::cout << sales.slice(1, 3).to_string() << '\n';
  std::cout << sales.take(std::vector<std::int64_t>{3, 0}).to_string() << '\n';

  auto schema = sales.rename("units", "quantity")
                   .drop({"region"})
                   .reorder({"city", "price", "quantity"});
  std::cout << schema.to_string() << '\n';

  auto city_cases = sales.label().set("city", true);
  std::cout << city_cases.features({"region", "units"}).to_string() << '\n';

  auto reset = city_cases.label().reset("city_case").features({"city_case", "units"});
  std::cout << reset.to_string() << '\n';
}

void example_11_combine_frames() {
  section("11 combine frames");
  DataFrame left({
      {"sku", pluss::make_note({"A", "B", "C"})},
      {"qty", pluss::make_series({1.0, 2.0, 3.0})},
  });
  DataFrame right({
      {"sku", pluss::make_note({"B", "C", "D"})},
      {"qty", pluss::make_series({20.0, 30.0, 40.0})},
      {"cost", pluss::make_series({5.0, 8.0, 13.0})},
  });
  DataFrame patch({
      {"qty", pluss::make_series({10.0, 20.0, 30.0})},
      {"status", pluss::make_note({"new", "open", "closed"})},
  });

  std::cout << left.join(patch, "_patch").to_string() << '\n';
  std::cout << left.update(patch, true).to_string() << '\n';
  std::cout << left.stitch_on("sku", right, "sku", "_right").to_string() << '\n';
}

void example_12_thread_missing_and_rows() {
  section("12 thread missing and rows");
  Thread score(
      pluss::make_series({4.0, 1.0, 0.0, 3.0, 2.0}),
      bool_series({true, true, false, true, true}));

  std::cout << "score:\n" << score.to_string() << '\n';
  std::cout << "isnull:\n" << score.isnull().to_string() << '\n';
  std::cout << "fillna:\n" << score.fillna(99.0).to_string() << '\n';
  std::cout << "dropnull:\n" << score.dropnull().to_string() << '\n';
  std::cout << "shift(2):\n" << score.shift(2).to_string() << '\n';
  std::cout << "sort asc nulls first:\n" << score.sort_values(true, false).to_string() << '\n';
  std::cout << "take 4,1,0:\n" << score.take(std::vector<std::int64_t>{4, 1, 0}).to_string() << '\n';
}

void example_13_thread_windows_casts_and_ts() {
  section("13 thread windows casts and ts");
  Thread values = pluss::make_series({1.0, 2.0, 4.0, 8.0, 16.0});
  Thread labels(pluss::make_note({"10", "20", "bad", "40"}));
  Thread words(pluss::make_note({"North-East", "South", "north-west", "Central"}));

  std::cout << "values + 3:\n" << (values + 3.0).to_string() << '\n';
  std::cout << "between 2 and 8 inclusive:\n" << values.btw(2.0, 8.0, true).to_string() << '\n';
  std::cout << "rolling mean window 3 min 2:\n"
            << values.window(3, pluss::table::ThreadWindowOp::RollingMean, 2).to_string() << '\n';
  std::cout << "returns:\n" << values.ts().returns(1).to_string() << '\n';
  std::cout << "label cast to float64:\n"
            << labels.cast(pluss::table::ThreadCastTarget::Series, pluss::codem::ScalarType::Float64).to_string()
            << '\n';
  std::cout << "words replace/lower:\n" << words.replace("-", " ").lower().to_string() << '\n';
  std::cout << "words contains north:\n" << Thread(words.lower().contains("north")).to_string() << '\n';
}

void example_14_csv_roundtrip(const std::filesystem::path& dir) {
  section("14 csv roundtrip");
  const std::vector<std::vector<std::string>> records = {
      {"id", "city", "units", "active"},
      {"A001", "Accra", "10", "yes"},
      {"A002", "Tema", "", "no"},
      {"A003", "Kumasi", "12", "yes"},
  };

  pluss::loader::ReadCsvOptions read_options;
  read_options.case_col = "id";
  auto df = pluss::loader::load_csv_records(records, read_options);

  std::cout << df.to_string() << '\n';
  std::cout << "city dtype=" << df.features("city").dtype() << '\n';
  std::cout << "units dtype=" << df.features("units").dtype() << '\n';
  std::cout << "active dtype=" << df.features("active").dtype() << '\n';

  pluss::loader::WriteCsvOptions write_options;
  write_options.sep = ';';
  write_options.add_cases = true;
  write_options.case_label = "id";
  write_options.na_rep = "";

  const auto path = dir / "plussmic_manual_roundtrip.csv";
  df.save_csv(path.string(), write_options);

  std::ifstream in(path);
  std::string line;
  while (std::getline(in, line)) {
    std::cout << line << '\n';
  }
}

void example_15_plotting(const std::filesystem::path& dir) {
  section("15 plotting");
  DataFrame df({
      {"x", pluss::make_series({0.0, 1.0, 2.0, 3.0})},
      {"y", pluss::make_series({1.0, 1.4, 2.2, 3.0})},
      {"z", pluss::make_series({0.8, 1.0, 1.6, 2.4})},
      {"grp", pluss::make_note({"alpha", "alpha", "beta", "beta"})},
  });

  pluss::table::DataFramePlotOptions plot_options;
  plot_options.default_hue_feature = "grp";
  plot_options.width = 720.0;
  plot_options.height = 420.0;
  plot_options.use_tight_layout = true;

  auto line = df.plot(plot_options).line({"y", "z"}, "x");
  auto scatter = df.plot(plot_options).scatter("x", "y");
  auto hist = df.plot().hist({"x", "y"}, 6);
  auto corr = df.plot().corr_heatmap({"x", "y", "z"});
  auto brief = df.plot().brief({"x", "y", "z"});
  auto scatter3d = df.plot3d().scatter("x", "y", "z");

  DataFrame grid({
      {"gx", pluss::make_series({0.0, 1.0, 0.0, 1.0})},
      {"gy", pluss::make_series({0.0, 0.0, 1.0, 1.0})},
      {"gz", pluss::make_series({1.0, 2.0, 3.0, 4.0})},
  });
  auto surface = grid.plot3d().surface("gx", "gy", "gz");

  print_figure_summary("line", line);
  print_figure_summary("scatter default hue", scatter);
  print_figure_summary("hist multi", hist);
  print_figure_summary("corr heatmap", corr);
  std::cout << "brief: panes=" << brief.pane_count()
            << ", caption characters=" << brief.panes().front().caption_text().size() << '\n';
  print_figure_summary("scatter3d", scatter3d);
  print_figure_summary("surface3d", surface);

  const auto svg_path = dir / "plussmic_manual_line.svg";
  line.save_svg(svg_path.string());
  std::cout << "saved svg bytes=" << std::filesystem::file_size(svg_path) << '\n';

  const std::vector<std::pair<std::string, const pluss::pl::Studio*>> gallery = {
      {"line", &line},
      {"scatter", &scatter},
      {"hist", &hist},
      {"corr_heatmap", &corr},
      {"brief", &brief},
      {"scatter3d", &scatter3d},
      {"surface3d", &surface},
  };
  for (const auto& [name, figure] : gallery) {
    figure->save_svg((dir / (name + ".svg")).string());
    figure->save_png((dir / (name + ".png")).string(), 144);
  }
  std::cout << "saved gallery files=" << gallery.size() * 2 << '\n';
}

void example_16_time_series() {
  section("16 time series");

  DataFrame df({
      {"date", pluss::make_timeline({
          "2024-01-01T00:00:00Z",
          "2024-01-01T12:00:00Z",
          "2024-01-02T00:00:00Z",
          "2024-01-02T12:00:00Z"})},
      {"close", pluss::make_series({100.0, 105.0, 103.0, 109.0})},
      {"volume", pluss::make_series({10.0, 20.0, 30.0, 40.0})},
  });

  std::cout << "returns:\n" << df.features("close").ts().returns(1).to_string() << '\n';
  std::cout << "rolling mean window 2:\n"
            << df.features("close").ts().rolling(2, pluss::table::WindowStatistic::Mean, {.min_count = 2}).to_string() << '\n';

  auto values = df.label().set("date", false).features({"close", "volume"});
  auto daily = values.ts().resample(
      "D", pluss::subsp::stats::ts::ResampleOp::Mean);
  std::cout << "daily mean:\n" << daily.to_string() << '\n';

  auto calendar = df.features({"date"}).tl().year();
  std::cout << "calendar:\n" << calendar.to_string() << '\n';

  auto lags = df.features({"close", "volume"}).ts().lag_block({1, 2});
  std::cout << "lag block:\n" << lags.to_string() << '\n';
}

} // namespace

int main(int argc, char** argv) {
  const auto dir = argc > 1
      ? std::filesystem::path(argv[1])
      : std::filesystem::temp_directory_path();
  std::filesystem::create_directories(dir);
  example_01_construct_and_inspect();
  example_02_select_and_reduce();
  example_03_filter_and_assign();
  example_04_group_and_aggregate();
  example_05_missing_data();
  example_06_csv_options(dir);
  example_07_thread_operations();
  example_08_atlas_and_conversion();
  example_09_sql();
  example_10_shape_and_schema();
  example_11_combine_frames();
  example_12_thread_missing_and_rows();
  example_13_thread_windows_casts_and_ts();
  example_14_csv_roundtrip(dir);
  example_15_plotting(dir);
  example_16_time_series();
  return 0;
}

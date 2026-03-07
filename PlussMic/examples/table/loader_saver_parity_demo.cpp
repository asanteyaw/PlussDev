#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>

#include <pluss/pluss.h>

namespace {

namespace fs = std::filesystem;

fs::path data_file(std::string_view name) {
  return fs::path(PLUSSMIC_SOURCE_DIR) / "data" / std::string(name);
}

fs::path demo_temp_dir() {
  const auto dir = fs::temp_directory_path() / "plussmic_loader_saver_demo";
  fs::create_directories(dir);
  return dir;
}

void write_text_file(const fs::path& path, std::string_view text) {
  std::ofstream out(path, std::ios::out | std::ios::trunc | std::ios::binary);
  if (!out.is_open()) {
    throw std::runtime_error("failed to open file for write: " + path.string());
  }
  out << text;
}

void print_frame_shape(std::string_view label, const pluss::table::DataFrame& frame) {
  fmt::print("{} -> [{} cs x {} feat]\n", label, frame.ncases(), frame.nfeatures());
}

void print_catalog_shape(std::string_view label, const pluss::kindle::Catalog& catalog) {
  fmt::print("{} -> [{} rows x {} streams]\n", label, catalog.count(), catalog.nstreams());
}

void print_registry_shape(std::string_view label, const pluss::bundle::Registry& registry) {
  fmt::print("{} -> [{} slots]\n", label, registry.nslots());
}

struct TextFixtures {
  fs::path delimited_txt;
  fs::path whitespace_txt;
  fs::path fixed_width_txt;
  fs::path hetero_json;
};

TextFixtures create_text_fixtures() {
  const auto root = demo_temp_dir();
  TextFixtures out{};
  out.delimited_txt = root / "demo_delimited.txt";
  out.whitespace_txt = root / "demo_whitespace.txt";
  out.fixed_width_txt = root / "demo_fixed_width.txt";
  out.hetero_json = root / "demo_registry_hetero.json";

  write_text_file(
      out.delimited_txt,
      "case|city|value|when\n"
      "0|Accra|1.5|2024-01-01T00:00:00Z\n"
      "1|Tema|2.0|2024-01-02T00:00:00Z\n"
      "2|Kumasi|3.25|2024-01-03T00:00:00Z\n");

  write_text_file(
      out.whitespace_txt,
      "case city value when\n"
      "0 Accra 1.5 2024-01-01T00:00:00Z\n"
      "1 Tema 2.0 2024-01-02T00:00:00Z\n"
      "2 Kumasi 3.25 2024-01-03T00:00:00Z\n");

  write_text_file(
      out.fixed_width_txt,
      "casecity       value   when                \n"
      "0   Accra      1.5     2024-01-01T00:00:00Z\n"
      "1   Tema       2.0     2024-01-02T00:00:00Z\n"
      "2   Kumasi     3.25    2024-01-03T00:00:00Z\n");

  write_text_file(
      out.hetero_json,
      R"([
  {"id": 1, "mixed": 10, "tag": "ok"},
  {"id": 2, "mixed": "text", "tag": "ok"},
  {"id": 3, "mixed": 3.5, "tag": "ok"}
])");

  return out;
}

void demo_dataframe_io(const TextFixtures& fixtures) {
  fmt::print("\n=== DataFrame Loaders/Savers ===\n");
  const auto report_excel_cfg = pluss::loader::config_from(
      {{"sheetindex", 0},
       {"auto_table_block", true},
       {"trim_outer_empty", true},
       {"header", true},
       {"header_row", 0},
       {"header_rows", 2},
       {"header_ffill", true},
       {"header_joiner", "_"}});

  fmt::print("loading DataFrame csv...\n");
  const auto df_csv = pluss::table::DataFrame::load(
      "csv",
      data_file("messy_data.csv").string(),
      {{"ncases", 6}});
  fmt::print("loading DataFrame excel...\n");
  const auto df_excel = pluss::table::DataFrame::load_excel(data_file("report.xlsx").string(), report_excel_cfg);
  fmt::print("loading DataFrame json...\n");
  const auto df_json = pluss::table::DataFrame::load_json(
      data_file("weather.json").string(),
      {{"ncases", 6}});

  fmt::print("loading DataFrame txt delimited...\n");
  const auto df_txt_delimited = pluss::table::DataFrame::load_txt(
      fixtures.delimited_txt.string(),
      {{"txt_mode", "delimited"}, {"sep", "pipe"}, {"header", true}});
  fmt::print("loading DataFrame txt whitespace...\n");
  const auto df_txt_whitespace = pluss::table::DataFrame::load_txt(
      fixtures.whitespace_txt.string(),
      {{"txt_mode", "whitespace"}, {"header", true}});
  fmt::print("loading DataFrame txt fixed-width...\n");
  const auto df_txt_fixed = pluss::table::DataFrame::load_txt(
      fixtures.fixed_width_txt.string(),
      {{"txt_mode", "fixed_width"}, {"widths", {"4", "11", "8", "20"}}, {"header", true}});

  print_frame_shape("DataFrame::load(\"csv\", ...)", df_csv);
  print_frame_shape("DataFrame::load_excel(...)", df_excel);
  print_frame_shape("DataFrame::load_json(...)", df_json);
  print_frame_shape("DataFrame::load_txt(delimited)", df_txt_delimited);
  print_frame_shape("DataFrame::load_txt(whitespace)", df_txt_whitespace);
  print_frame_shape("DataFrame::load_txt(fixed_width)", df_txt_fixed);

  try {
    const auto df_parquet = pluss::table::DataFrame::load_parquet(data_file("weather.parquet").string());
    print_frame_shape("DataFrame::load_parquet(...)", df_parquet);
  } catch (const std::exception& e) {
    fmt::print("DataFrame::load_parquet(...) not available in this build: {}\n", e.what());
  }

  const auto out_dir = demo_temp_dir() / "df_outputs";
  fs::create_directories(out_dir);
  const auto out_csv = out_dir / "df_demo.csv";
  const auto out_excel = out_dir / "df_demo.xlsx";
  const auto out_txt = out_dir / "df_demo.txt";
  const auto out_json = out_dir / "df_demo.json";
  const auto out_json_split = out_dir / "df_demo_split.json";

  df_txt_whitespace.save_csv(out_csv.string());
  df_txt_whitespace.save_excel(out_excel.string());
  df_txt_whitespace.save_txt(out_txt.string(), {{"header", true}});
  df_txt_whitespace.save_json(out_json.string());
  df_txt_whitespace.save_as("json", out_json_split.string(), {{"orient", "split"}});

  const auto rt_csv = pluss::table::DataFrame::load_csv(out_csv.string());
  const auto rt_txt = pluss::table::DataFrame::load_txt(out_txt.string(), {{"header", true}});
  const auto rt_json = pluss::table::DataFrame::load_json(out_json.string());
  print_frame_shape("roundtrip csv", rt_csv);
  print_frame_shape("roundtrip txt", rt_txt);
  print_frame_shape("roundtrip json", rt_json);

  try {
    const auto out_parquet = out_dir / "df_demo.parquet";
    df_txt_whitespace.save_parquet(out_parquet.string());
    fmt::print("saved parquet: {}\n", out_parquet.string());
  } catch (const std::exception& e) {
    fmt::print("DataFrame::save_parquet(...) not available in this build: {}\n", e.what());
  }
}

void demo_catalog_io(const TextFixtures& fixtures) {
  fmt::print("\n=== Catalog Loaders/Savers ===\n");
  const auto report_excel_cfg = pluss::loader::config_from(
      {{"sheetindex", 0},
       {"auto_table_block", true},
       {"trim_outer_empty", true},
       {"header", true},
       {"header_row", 0},
       {"header_rows", 2},
       {"header_ffill", true},
       {"header_joiner", "_"}});

  const auto cat_csv = pluss::kindle::Catalog::load(
      "csv",
      data_file("messy_data.csv").string(),
      pluss::loader::config_from({{"ncases", 6}}));
  const auto cat_excel = pluss::kindle::Catalog::load("excel", data_file("report.xlsx").string(), report_excel_cfg);
  const auto cat_json = pluss::kindle::Catalog::load_json(
      data_file("weather.json").string(),
      pluss::loader::config_from({{"ncases", 6}}));
  const auto cat_txt = pluss::kindle::Catalog::load_txt(
      fixtures.whitespace_txt.string(),
      pluss::loader::config_from({{"txt_mode", "whitespace"}, {"header", true}}));
  const auto cat_from_json_text = pluss::kindle::Catalog::from_json(
      R"([{"id": 1, "city": "Accra"}, {"id": 2, "city": "Tema"}])");

  print_catalog_shape("Catalog::load(\"csv\", ...)", cat_csv);
  print_catalog_shape("Catalog::load(\"excel\", ...)", cat_excel);
  print_catalog_shape("Catalog::load_json(...)", cat_json);
  print_catalog_shape("Catalog::load_txt(...)", cat_txt);
  print_catalog_shape("Catalog::from_json(...)", cat_from_json_text);

  try {
    const auto cat_parquet = pluss::kindle::Catalog::load_parquet(data_file("weather.parquet").string());
    print_catalog_shape("Catalog::load_parquet(...)", cat_parquet);
  } catch (const std::exception& e) {
    fmt::print("Catalog::load_parquet(...) not available in this build: {}\n", e.what());
  }

  const auto out_dir = demo_temp_dir() / "catalog_outputs";
  fs::create_directories(out_dir);
  cat_txt.save_csv((out_dir / "catalog_demo.csv").string());
  cat_txt.save_excel((out_dir / "catalog_demo.xlsx").string());
  cat_txt.save_txt(
      (out_dir / "catalog_demo.txt").string(),
      pluss::loader::config_from({{"header", true}}));
  cat_txt.save_json((out_dir / "catalog_demo.json").string());
  cat_txt.save_as(
      "json",
      (out_dir / "catalog_demo_split.json").string(),
      pluss::loader::config_from({{"orient", "split"}}));

  try {
    cat_txt.save_parquet((out_dir / "catalog_demo.parquet").string());
  } catch (const std::exception& e) {
    fmt::print("Catalog::save_parquet(...) not available in this build: {}\n", e.what());
  }
}

void demo_registry_io(const TextFixtures& fixtures) {
  fmt::print("\n=== Registry Loaders/Savers ===\n");
  const auto report_excel_cfg = pluss::loader::config_from(
      {{"sheetindex", 0},
       {"auto_table_block", true},
       {"trim_outer_empty", true},
       {"header", true},
       {"header_row", 0},
       {"header_rows", 2},
       {"header_ffill", true},
       {"header_joiner", "_"}});

  const auto reg_csv = pluss::bundle::Registry::load(
      "csv",
      data_file("messy_data.csv").string(),
      pluss::loader::config_from({{"ncases", 6}}));
  const auto reg_excel = pluss::bundle::Registry::load("excel", data_file("report.xlsx").string(), report_excel_cfg);
  const auto reg_txt = pluss::bundle::Registry::load_txt(
      fixtures.whitespace_txt.string(),
      pluss::loader::config_from({{"txt_mode", "whitespace"}, {"header", true}}));
  const auto reg_json = pluss::bundle::Registry::load_json(fixtures.hetero_json.string());
  const auto reg_stream_compatible = pluss::bundle::Registry::from_kindle(
      pluss::kindle::Catalog::load_txt(
          fixtures.whitespace_txt.string(),
          pluss::loader::config_from({{"txt_mode", "whitespace"}, {"header", true}})));

  print_registry_shape("Registry::load(\"csv\", ...)", reg_csv);
  print_registry_shape("Registry::load(\"excel\", ...)", reg_excel);
  print_registry_shape("Registry::load_txt(...)", reg_txt);
  print_registry_shape("Registry::load_json(heterogeneous)", reg_json);

  try {
    const auto reg_parquet = pluss::bundle::Registry::load_parquet(data_file("weather.parquet").string());
    print_registry_shape("Registry::load_parquet(...)", reg_parquet);
  } catch (const std::exception& e) {
    fmt::print("Registry::load_parquet(...) not available in this build: {}\n", e.what());
  }

  const auto out_dir = demo_temp_dir() / "registry_outputs";
  fs::create_directories(out_dir);

  reg_stream_compatible.save_csv((out_dir / "registry_demo.csv").string());
  reg_stream_compatible.save_excel((out_dir / "registry_demo.xlsx").string());
  reg_stream_compatible.save_txt(
      (out_dir / "registry_demo.txt").string(),
      pluss::loader::config_from({{"header", true}}));
  reg_stream_compatible.save_json((out_dir / "registry_demo.json").string());
  reg_stream_compatible.save_as(
      "json",
      (out_dir / "registry_demo_split.json").string(),
      pluss::loader::config_from({{"orient", "split"}}));

  reg_json.save_json((out_dir / "registry_hetero_native.json").string());
  fmt::print("Registry heterogeneous JSON save succeeded via native representation.\n");

  try {
    reg_json.save_csv((out_dir / "registry_hetero_should_fail.csv").string());
    fmt::print("WARNING: heterogeneous Registry unexpectedly saved as CSV.\n");
  } catch (const std::exception& e) {
    fmt::print("Registry heterogeneous save to tabular target is blocked (expected): {}\n", e.what());
  }

  try {
    reg_stream_compatible.save_parquet((out_dir / "registry_demo.parquet").string());
  } catch (const std::exception& e) {
    fmt::print("Registry::save_parquet(...) not available in this build: {}\n", e.what());
  }
}

void demo_loader_level_save_as() {
  fmt::print("\n=== Loader-Level save_as/load ===\n");
  const auto df = pluss::table::DataFrame::load(
      "csv",
      data_file("messy_data.csv").string(),
      {{"ncases", 5}});

  const auto out_dir = demo_temp_dir() / "loader_outputs";
  fs::create_directories(out_dir);
  const auto json_path = out_dir / "loader_save_as.json";
  const auto txt_path = out_dir / "loader_save_as.txt";

  pluss::loader::save_as(df, "json", json_path.string(), {{"orient", "records"}});
  pluss::loader::save_as(df, "txt", txt_path.string(), {{"header", true}});

  const auto loaded_json = pluss::loader::load("json", json_path.string());
  const auto loaded_txt = pluss::loader::load("txt", txt_path.string(), {{"header", true}});
  print_frame_shape("loader::load(json)", loaded_json.dataframe());
  print_frame_shape("loader::load(txt)", loaded_txt.dataframe());
}

} // namespace

int main() {
  try {
    fmt::print("PlussMic Loader/Saver Parity Demo\n");
    fmt::print("data dir: {}\n", (fs::path(PLUSSMIC_SOURCE_DIR) / "data").string());
    fmt::print("temp dir: {}\n", demo_temp_dir().string());

    const auto fixtures = create_text_fixtures();
    demo_dataframe_io(fixtures);
    demo_catalog_io(fixtures);
    demo_registry_io(fixtures);
    demo_loader_level_save_as();
  } catch (const std::exception& e) {
    fmt::print("demo failed: {}\n", e.what());
    return 1;
  }
  return 0;
}

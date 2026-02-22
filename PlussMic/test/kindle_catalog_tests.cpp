#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "pluss/pluss.h"

namespace {

using pluss::kindle::Catalog;
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
  const auto timeline = pluss::moment({
      "2024-01-01T00:00:00Z",
      "2024-01-02T00:00:00Z",
      "2024-01-03T00:00:00Z",
      "2024-01-04T00:00:00Z",
      "2024-01-05T00:00:00Z",
  });
  const auto timeindex = pluss::make_timeindex({
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
                       {"time_index", pluss::table::Thread(timeindex)},
                   },
                   cases);
}

Catalog make_base_catalog() {
  return Catalog::kindelize(make_base_frame());
}

void test_stream_methods() {
  const auto catalog = make_base_catalog();
  auto metric = catalog.stream("metric");

  expect(metric.defined(), "metric stream should be defined");
  expect(metric.kind() == StreamKind::Series, "metric should be Series stream");
  expect(metric.size() == 5, "metric stream size mismatch");
  expect(metric.has_cases(), "metric should carry cases after kindelize");
  expect(metric.has_validity_mask(), "metric should carry validity mask");

  metric = metric.with_meta("source", "test");
  expect(metric.meta("source").value_or("") == "test", "meta write/read mismatch");
  metric = metric.drop_meta("source");
  expect(!metric.meta("source").has_value(), "meta should be removed");

  const auto filled = metric.fillna(0.0);
  expect(!filled.has_validity_mask(), "fillna should clear validity mask");
  const auto dropped = metric.dropna();
  expect(dropped.size() == 3, "dropna should keep only valid rows");
  const auto taken = metric.take(std::vector<std::int64_t>{0, 2, 4});
  expect(taken.size() == 3, "take size mismatch");

  const auto mask = (filled > 15.0).as_series();
  const auto filtered = filled.filter(mask);
  expect(filtered.size() == 2, "filter result size mismatch");

  const auto math_out = ((filled + 3.0) * 2.0) / 2.0;
  expect(math_out.size() == 5, "stream arithmetic size mismatch");

  const auto mean = filled.mean();
  expect(std::fabs(scalar_stream_to_double(mean) - 16.0) < 1e-9, "stream mean mismatch");
}

void test_catalog_construction_and_selection() {
  const auto base = make_base_catalog();
  using namespace pluss::kindle::literals;
  const auto via_named = Catalog({
      "id"_k = base.stream("id"),
      "city"_k = base.stream("city"),
  });
  const auto via_pairs = Catalog(std::vector<std::pair<std::string, Stream>>{
      {"id", base.stream("id")},
      {"city", base.stream("city")},
  });
  std::vector<std::pair<std::string, Stream>> vec_items{
      {"id", base.stream("id")},
      {"city", base.stream("city")},
  };
  const auto via_vector = Catalog(std::move(vec_items));
  expect(via_named.nstreams() == 2, "named constructor mismatch");
  expect(via_pairs.nstreams() == 2, "pair constructor mismatch");
  expect(via_vector.nstreams() == 2, "vector constructor mismatch");

  expect(base.series().nstreams() == 2, "series projection mismatch");
  expect(base.note().nstreams() == 2, "note projection mismatch");
  expect(base.timeline().nstreams() == 2, "timeline projection should include Timeline + TimeIndex");
  expect(base.timeindex().nstreams() == 1, "timeindex projection mismatch");

  const auto h = base.head(2);
  const auto t = base.tail(2);
  const auto s = base.slice(1, 4);
  const auto i = base.iloc(0, 3);
  expect(h.count() == 2, "head count mismatch");
  expect(t.count() == 2, "tail count mismatch");
  expect(s.count() == 3, "slice count mismatch");
  expect(i.count() == 3, "iloc count mismatch");

  const auto filt = base.filter(bool_mask({true, false, true, true, false}));
  const auto masked = base.masked_select(bool_mask({true, false, true, true, false}));
  expect(filt.count() == 3, "filter count mismatch");
  expect(masked.count() == 3, "masked_select count mismatch");

  const auto selected = base.select({"id", "city"});
  const auto dropped = base.drop(std::vector<std::string>{"time_index"});
  const auto renamed = base.rename({{"metric", "distance"}});
  expect(selected.nstreams() == 2, "select stream count mismatch");
  expect(dropped.nstreams() == base.nstreams() - 1, "drop stream count mismatch");
  expect(renamed.has("distance"), "rename should create distance key");

  expect_throw([&]() { (void)base.select({"missing_key"}); }, "select unknown key should throw");
}

void test_catalog_sort_group_missing() {
  const auto base = make_base_catalog();
  const auto sorted = base.sort("id", true);
  const auto id_sorted = sorted.stream("id").as_series().to_vec<std::int64_t>();
  expect((id_sorted == std::vector<std::int64_t>{1, 2, 3, 4, 5}), "sort(id) order mismatch");

  const auto sorted_by = base.sort_by({"city", "id"}, {true, false});
  expect(sorted_by.count() == 5, "sort_by should preserve row count");

  const auto groups = base.group_by("city");
  expect(groups.size() == 3, "group_by(city) should produce 3 buckets");
  expect(groups.find("Accra") != groups.end(), "group_by(city) missing Accra");
  expect(groups.at("Accra").count() == 2, "Accra group row count mismatch");

  const auto filled = base.fillna(0.0, {"metric"});
  const auto dropped = base.dropna({"metric"});
  expect(filled.count() == 5, "fillna should not change row count");
  expect(dropped.count() == 3, "dropna(metric) should drop null rows");
}

void test_catalog_aggregates_apply_text_merge() {
  auto base = make_base_catalog();
  expect(std::fabs(scalar_stream_to_double(base.sum("id")) - 15.0) < 1e-9, "sum(id) mismatch");
  expect(std::fabs(scalar_stream_to_double(base.min("id")) - 1.0) < 1e-9, "min(id) mismatch");
  expect(std::fabs(scalar_stream_to_double(base.max("id")) - 5.0) < 1e-9, "max(id) mismatch");
  expect(base.count() == 5, "catalog count mismatch");

  const auto right = Catalog(std::vector<std::pair<std::string, Stream>>{
      {"id", base.stream("id")},
      {"bonus", Stream(pluss::make_series(std::vector<double>{100, 200, 300, 400, 500}, {5}))},
  });
  const auto merged = base.merge(right, "id", "inner");
  expect(merged.has("bonus"), "merge should include right-side feature");
  expect(merged.count() == 5, "merge row count mismatch");
  expect_throw([&]() { (void)base.merge(right, "id", "left"); }, "merge(how!=inner) should throw");

  const auto applied = base.apply("id", [](const Stream& s) { return s + 10.0; });
  const auto applied_values = applied.stream("id").as_series().to_vec<std::int64_t>();
  expect(applied_values[0] == 13, "apply should transform target field");

  base.apply_("id", [](const Stream& s) { return s - 1.0; });
  expect(base.stream("id").as_series().to_vec<std::int64_t>()[0] == 2, "apply_ should mutate target field");

  expect_throw(
      [&]() {
        (void)base.apply("id", [](const Stream& s) { return s.head(2); });
      },
      "apply with size-changing fn should throw");

  const auto text_clean = base.text_preprocess("text");
  const auto keywords = base.extract_keywords("text", 4);
  const auto similarity = base.compute_similarity("text", "city", "cosine");
  const auto text_filtered = base.filter_by_text("text", "rain");
  const auto text_features = base.add_text_features("text");
  const auto text_stats = base.aggregate_text_stats("text", "mean");
  const auto text_tfidf = base.tfidf("text");

  expect(text_clean.count() == 5, "text_preprocess should preserve row count");
  expect(keywords.has("keyword") && keywords.has("count"), "extract_keywords should emit keyword/count");
  expect(similarity.has("text__city_similarity"), "compute_similarity key mismatch");
  expect(text_filtered.count() == 2, "filter_by_text('rain') row count mismatch");
  expect(text_features.has("text_char_len"), "add_text_features missing text_char_len");
  expect(text_features.has("text_word_count"), "add_text_features missing text_word_count");
  expect(text_stats.count() == 1, "aggregate_text_stats should produce one row");
  expect(text_tfidf.has("text_tfidf_norm"), "tfidf should add norm feature");
}

void test_catalog_mutation_tabelize_io() {
  using namespace pluss::kindle::literals;
  auto base = make_base_catalog();

  base = base.set_stream("id_plus", base.stream("id") + 1.0);
  base.set_stream_("city_copy", base.stream("city"));
  base = base.set_streams({
      "ones"_k = Stream(pluss::make_series(std::vector<double>{1, 1, 1, 1, 1}, {5})),
  });
  base.set_streams_({
      "flag"_k = Stream(pluss::make_series(std::vector<double>{0, 1, 0, 1, 0}, {5})),
  });
  base = base.drop({"flag"});
  expect(base.has("id_plus"), "set_stream should add id_plus");
  expect(base.has("city_copy"), "set_stream_ should add city_copy");
  expect(base.has("ones"), "set_streams should add ones");
  expect(!base.has("flag"), "drop should remove flag");

  pluss::kindle::TabelizePolicy policy;
  policy.align = pluss::kindle::AlignPolicy::Inner;
  policy.index_mode = pluss::kindle::IndexMode::ByCaseLabel;
  policy.include_case_feature = true;
  policy.case_feature_name = "case";
  policy.collision = pluss::kindle::CollisionPolicy::Suffix;
  const auto frame = base.tabelize(policy);
  expect(frame.nrows() == 5, "tabelize row count mismatch");
  expect(frame.nfeatures() >= base.nstreams(), "tabelize feature count mismatch");
  const auto frame_via_to_table = base.to_table(policy, false);
  expect(frame_via_to_table.nrows() == frame.nrows(), "to_table(row count) should match tabelize");
  expect(frame_via_to_table.nfeatures() == frame.nfeatures(), "to_table(feature count) should match tabelize");

  expect(!base.to_string().empty(), "to_string should not be empty");
  expect(!base.to_json().empty(), "to_json should not be empty");

  std::int64_t iter_count = 0;
  for (const auto& key : base) {
    (void)key;
    ++iter_count;
  }
  expect(iter_count == base.nstreams(), "iterator should visit all keys");

  const auto out_path = temp_csv_path("catalog_save");
  base.save_csv(out_path.string());
  const auto reloaded = DataFrame::load_csv(out_path.string());
  expect(reloaded.nrows() == 5, "save_csv/load_csv roundtrip rows mismatch");

  const auto loaded_csv = Catalog::load_csv(data_file("messy_data.csv").string());
  expect(loaded_csv.nstreams() > 0, "load_csv should return non-empty catalog");

  const auto loaded_generic = Catalog::load(
      "csv",
      data_file("messy_data.csv").string(),
      pluss::loader::config_from({
          {"nrows", 8},
      }));
  expect(loaded_generic.count() == 8, "Catalog::load(csv, config) nrows mismatch");

  pluss::loader::ReadCsvOptions opts;
  opts.nrows = 24;
  const auto keyed = Catalog::load_csv_with_key(data_file("bank-full.csv").string(), "job", opts);
  expect(keyed.has("_case"), "load_csv_with_key should materialize _case stream");

  expect_throw([&]() { (void)Catalog::from_json("{}"); }, "from_json should throw (not implemented)");
}

void test_archive_to_table_bridge() {
  const auto base = make_base_frame();
  const auto f0 = base.slice(0, 3);
  const auto f1 = base.slice(2, 5);
  const auto labels = pluss::table::Thread(
      pluss::make_note(std::vector<std::string>{"batch_a", "batch_b"}, std::array<std::int64_t, 1>{2}));
  const pluss::table::Atlas atlas({f0, f1}, labels);

  const auto archive = pluss::kindle::Archive::kindelize(atlas);
  const auto atlas_a = archive.tabelize();
  const auto atlas_b = archive.to_table({}, false);
  expect(atlas_a.depth() == atlas_b.depth(), "Archive::to_table depth mismatch");
  expect(atlas_a.nrows() == atlas_b.nrows(), "Archive::to_table nrows mismatch");
  expect(atlas_a.ncols() == atlas_b.ncols(), "Archive::to_table ncols mismatch");
}

} // namespace

int main() {
  try {
    test_stream_methods();
    test_catalog_construction_and_selection();
    test_catalog_sort_group_missing();
    test_catalog_aggregates_apply_text_merge();
    test_catalog_mutation_tabelize_io();
    test_archive_to_table_bridge();
  } catch (const std::exception& e) {
    std::cerr << "[kindle_catalog_tests] failure: " << e.what() << "\n";
    return 1;
  }

  std::cout << "[kindle_catalog_tests] all checks passed\n";
  return 0;
}

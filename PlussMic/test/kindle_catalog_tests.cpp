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
#include "pluss/tils.h"

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
  const auto timeline = pluss::make_timeline({
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
  expect(metric.has_caselbl(), "metric should carry cases after kindelize");
  expect(pluss::kindle::detail::has_validity_mask(metric), "metric should carry validity mask");

  metric = metric.with_meta("source", "test");
  expect(metric.meta("source").value_or("") == "test", "meta write/read mismatch");
  metric = metric.drop_meta("source");
  expect(!metric.meta("source").has_value(), "meta should be removed");

  const auto filled = metric.fillna(0.0);
  expect(!pluss::kindle::detail::has_validity_mask(filled), "fillna should clear validity mask");
  expect(filled.as_series().to_vec<double>() == std::vector<double>({10.0, 0.0, 30.0, 40.0, 0.0}),
         "Stream::fillna(double) should preserve valid values and replace only null rows");
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
  const auto i = base.cloc(0, 3);
  expect(h.count() == 2, "head count mismatch");
  expect(t.count() == 2, "tail count mismatch");
  expect(s.count() == 3, "slice count mismatch");
  expect(i.count() == 3, "iloc count mismatch");

  const auto filt = base.filter(bool_mask({true, false, true, true, false}));
  const auto masked = base.masked_select(bool_mask({true, false, true, true, false}));
  const auto where = base.where(bool_mask({true, false, true, true, false}));
  expect(filt.count() == 3, "filter count mismatch");
  expect(masked.count() == 3, "masked_select count mismatch");
  expect(where.count() == 3, "where count mismatch");

  const auto taken = base.take(std::vector<std::int64_t>{0, 2, 4});
  expect(taken.count() == 3, "take count mismatch");

  const auto sampled_a = base.sample(2, 42);
  const auto sampled_b = base.sample(2, 42);
  expect(sampled_a.stream("id").as_series().to_vec<std::int64_t>()
             == sampled_b.stream("id").as_series().to_vec<std::int64_t>(),
         "sample with same seed should be deterministic");

  const auto queried = base.query([](const Catalog& c) {
    return (c.stream("id") > 2.0).as_series();
  });
  expect(queried.count() == 3, "query count mismatch");

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
  const auto ffilled = base.ffill({"metric"});
  const auto bfilled = base.bfill({"metric"});
  const auto nulls = base.isnull({"metric"});
  const auto not_nulls = base.notnull({"metric"});
  expect(filled.count() == 5, "fillna should not change row count");
  expect(filled.stream("metric").as_series().to_vec<double>() == std::vector<double>({10.0, 0.0, 30.0, 40.0, 0.0}),
         "Catalog::fillna(double, keys) should preserve valid values and replace only null rows");
  expect(dropped.count() == 3, "dropna(metric) should drop null rows");
  expect(ffilled.count() == 5, "ffill should preserve row count");
  expect(bfilled.count() == 5, "bfill should preserve row count");
  expect(nulls.stream("metric").kind() == StreamKind::Series, "isnull should return Series stream");
  expect(not_nulls.stream("metric").kind() == StreamKind::Series, "notnull should return Series stream");

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
       Stream(pluss::make_timeindex({
                  "2024-01-01T00:00:00Z",
                  "2024-01-02T00:00:00Z",
                  "2024-01-03T00:00:00Z",
                  "2024-01-04T00:00:00Z",
              }),
              bool_mask({true, false, true, false}))},
  });

  const auto note_filled = mixed_missing.fillna("missing", {"note_col"});
  expect(std::string(note_filled.stream("note_col").as_note().at(1)) == "missing",
         "Catalog::fillna(string, keys) should replace null Note values");
  expect(!pluss::kindle::detail::has_validity_mask(note_filled.stream("note_col")),
         "Catalog::fillna(string, keys) should clear Note mask when fully resolved");

  const auto ts_fill = pluss::make_timeline("2024-01-31T00:00:00Z");
  const auto ts_filled = mixed_missing.fillna(ts_fill, {"ts_col"});
  const auto ts_ticks = ts_filled.stream("ts_col").as_timeline().to_epoch_nanos_vector();
  const auto ts_fill_tick = ts_fill.to_epoch_nanos_vector()[0];
  expect(ts_ticks[1] == ts_fill_tick && ts_ticks[3] == ts_fill_tick,
         "Catalog::fillna(Timeline, keys) should replace null Timeline values");
  expect(!pluss::kindle::detail::has_validity_mask(ts_filled.stream("ts_col")),
         "Catalog::fillna(Timeline, keys) should clear Timeline mask when fully resolved");

  const auto ti_fill = pluss::make_timeindex("2024-02-01T00:00:00Z", false);
  const auto ti_filled = mixed_missing.fillna(ti_fill, {"ti_col"});
  const auto ti_ticks = ti_filled.stream("ti_col").as_timeindex().timeline().to_epoch_nanos_vector();
  const auto ti_fill_tick = ti_fill.timeline().to_epoch_nanos_vector()[0];
  expect(ti_ticks[1] == ti_fill_tick && ti_ticks[3] == ti_fill_tick,
         "Catalog::fillna(TimeIndex, keys) should replace null TimeIndex values");
  expect(!pluss::kindle::detail::has_validity_mask(ti_filled.stream("ti_col")),
         "Catalog::fillna(TimeIndex, keys) should clear TimeIndex mask when fully resolved");

  const auto id_as_note = base.cast({"id"}, pluss::table::ThreadCastTarget::Note);
  const auto ts_as_series = base.cast(
      {"timestamp"}, pluss::table::ThreadCastTarget::Series,
      pluss::table::ThreadCastOptions{.series_dtype = pluss::codem::ScalarType::Int64});
  const auto ts_as_ti = base.cast({"timestamp"}, pluss::table::ThreadCastTarget::TimeIndex);
  const auto id_as_f64 = base.astype({"id"}, pluss::codem::ScalarType::Float64);
  expect(id_as_note.stream("id").kind() == StreamKind::Note, "cast(Note) id kind mismatch");
  expect(ts_as_series.stream("timestamp").kind() == StreamKind::Series, "cast(Series) timestamp kind mismatch");
  expect(ts_as_ti.stream("timestamp").kind() == StreamKind::TimeIndex, "cast(TimeIndex) timestamp kind mismatch");
  expect(id_as_f64.stream("id").as_series().scalar_type() == pluss::codem::ScalarType::Float64,
         "astype(id,float64) scalar type mismatch");
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

  const auto transformed_all = base.transform([](const Stream& s) { return s; });
  const auto transformed_one = base.transform("id", [](const Stream& s) { return s + 1.0; });
  expect(transformed_all.count() == base.count(), "transform(all) should preserve row count");
  expect(transformed_one.stream("id").as_series().to_vec<std::int64_t>()[0] == 4,
         "transform(field) should transform target field");

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

  const DataFrame default_cases({
      {"city", pluss::make_note({"Accra", "Tema", "Kumasi"})},
      {"units", pluss::make_series({10.0, 7.0, 12.0})},
  });
  const auto default_cases_roundtrip = Catalog::kindelize(default_cases).tabelize();
  expect(default_cases_roundtrip.equals(default_cases),
         "kindelize/tabelize should preserve a numeric default case axis");

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
  expect(frame.ncases() == 5, "tabelize row count mismatch");
  expect(frame.nfeatures() >= base.nstreams(), "tabelize feature count mismatch");
  const auto frame_via_to_table = base.to_table(policy, false);
  expect(frame_via_to_table.ncases() == frame.ncases(), "to_table(row count) should match tabelize");
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
  expect(loaded_csv.nstreams() > 0, "load_csv should return non-empty catalog");

  const auto loaded_generic = Catalog::load(
      "csv",
      data_file("messy_data.csv").string(),
      pluss::loader::config_from({
          {"ncases", 8},
      }));
  expect(loaded_generic.count() == 8, "Catalog::load(csv, config) nrows mismatch");

  pluss::loader::ReadCsvOptions opts;
  opts.ncases = 24;
  const auto keyed = Catalog::load_csv_with_key(data_file("bank-full.csv").string(), "job", opts);
  expect(keyed.has("_case"), "load_csv_with_key should materialize _case stream");

  const auto from_json = Catalog::from_json(
      R"([
  {"id": 1, "city": "Accra", "when": "2024-01-01T00:00:00Z"},
  {"id": 2, "city": "Kumasi"},
  {"id": 3, "city": "Tema", "when": "2024-01-03T00:00:00Z"}
])");
  expect(from_json.nstreams() == 3, "Catalog::from_json stream count mismatch");
  expect(from_json.stream("id").kind() == StreamKind::Series, "Catalog::from_json id kind mismatch");
  expect(from_json.stream("city").kind() == StreamKind::Note, "Catalog::from_json city kind mismatch");
  expect(from_json.stream("when").kind() == StreamKind::Timeline, "Catalog::from_json when kind mismatch");

  auto json_path = temp_csv_path("catalog_json");
  json_path.replace_extension(".json");
  {
    std::ofstream out(json_path, std::ios::out | std::ios::trunc | std::ios::binary);
    expect(out.is_open(), "failed to create catalog json temp file");
    out << R"({"id":[1,2], "city":["Accra","Tema"]})";
  }
  const auto loaded_json = Catalog::load_json(json_path.string());
  expect(loaded_json.nstreams() == 2, "Catalog::load_json stream count mismatch");
  expect(loaded_json.count() == 2, "Catalog::load_json row count mismatch");
  const auto loaded_json_generic = Catalog::load("json", json_path.string());
  expect(loaded_json_generic.nstreams() == loaded_json.nstreams(),
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
  expect(loaded_txt.nstreams() == 2, "Catalog::load_txt stream count mismatch");
  expect(loaded_txt.count() == 2, "Catalog::load_txt row count mismatch");
}

void test_archive_to_table_bridge() {
  const auto base = make_base_frame();
  const auto f0 = base.slice(0, 3);
  const auto f1 = base.slice(2, 5);
  const auto labels = pluss::table::Thread(
      pluss::make_note(std::vector<std::string>{"batch_a", "batch_b"}, std::array<std::int64_t, 1>{2}));
  const pluss::table::Atlas atlas({f0, f1}, labels);

  const auto archive = pluss::kindle::Archive::kindelize(atlas);
  expect(archive.dloc(0).nstreams() == 6, "Archive::dloc stream count mismatch");
  expect(archive.loc("batch_b").nstreams() == 6, "Archive::loc(string) stream count mismatch");
  expect(archive.head(1).depth() == 1, "Archive::head depth mismatch");
  expect(archive.tail(1).depth() == 1, "Archive::tail depth mismatch");
  expect(archive.slice_depth(0, 1).depth() == 1, "Archive::slice_depth mismatch");

  const auto sorted_desc = archive.sort_depth(false);
  const auto sorted_labels = sorted_desc.depth_labels()
                                 ->to_thread()
                                 .cast(pluss::table::ThreadCastTarget::Note)
                                 .as_note();
  expect(std::string(sorted_labels.at(0)) == "batch_b", "Archive::sort_depth(desc) label mismatch");

  const auto relabeled = archive.relabel_depth(
      Stream(pluss::make_note(std::vector<std::string>{"A", "B"}, std::array<std::int64_t, 1>{2})));
  expect(relabeled.depth_labels().has_value(), "Archive::relabel_depth should set labels");

  const auto aligned = archive.align_depth(
      Stream(pluss::make_note(std::vector<std::string>{"batch_b", "batch_a"}, std::array<std::int64_t, 1>{2})));
  const auto aligned_first = aligned.catalog(0).stream("id").as_series().to_vec<std::int64_t>();
  expect(aligned_first == f1.features("id").as_series().to_vec<std::int64_t>(),
         "Archive::align_depth should reorder catalogs");

  const auto mapped = archive.map_catalogs([](const Catalog& c) { return c.select({"id", "city"}); });
  expect(mapped.catalog(0).nstreams() == 2, "Archive::map_catalogs should transform each catalog");
  const auto reduced = archive.aggregate_over_catalogs([](const std::vector<Catalog>& cats) { return cats.front(); });
  expect(reduced.nstreams() == archive.catalog(0).nstreams(),
         "Archive::aggregate_over_catalogs should return reducer output");

  const auto atlas_a = archive.tabelize();
  const auto atlas_b = archive.to_table({}, false);
  expect(atlas_a.depth() == atlas_b.depth(), "Archive::to_table depth mismatch");
  expect(atlas_a.ncases() == atlas_b.ncases(), "Archive::to_table nrows mismatch");
  expect(atlas_a.nfeats() == atlas_b.nfeats(), "Archive::to_table ncols mismatch");
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

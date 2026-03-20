#include <array>
#include <filesystem>
#include <iostream>
#include <string_view>
#include <vector>

#include <pluss/pluss.h>

namespace {

std::filesystem::path data_file(std::string_view name) {
  return std::filesystem::path(PLUSSMIC_SOURCE_DIR) / "data" / std::string(name);
}

pluss::Series make_bool_mask(std::initializer_list<bool> values) {
  const auto n = static_cast<std::int64_t>(values.size());
  auto mask = pluss::Series::zeros({n}, pluss::codem::ScalarType::Bool);
  auto* p = reinterpret_cast<bool*>(mask.raw_data());
  std::int64_t i = 0;
  for (const bool v : values) {
    p[i++] = v;
  }
  return mask;
}

pluss::table::DataFrame make_base_frame() {
  const std::array<std::int64_t, 1> shape{5};

  const auto ids = pluss::make_series(std::vector<std::int64_t>{3, 1, 2, 5, 4}, {5});
  const auto metric = pluss::make_series(std::vector<double>{10.0, 20.0, 30.0, 40.0, 50.0}, {5});
  const auto city = pluss::make_note(std::vector<std::string>{"Accra", "Kumasi", "Accra", "Tema", "Kumasi"}, shape);
  const auto text = pluss::make_note(
      std::vector<std::string>{
          "Heavy Rain on road",
          "Clear weather",
          "Fog near school",
          "Heavy rain near mall",
          "Storm warning issued",
      },
      shape);
  const auto timestamp = pluss::make_timeline({
      "2024-01-01T00:00:00Z",
      "2024-01-02T00:00:00Z",
      "2024-01-03T00:00:00Z",
      "2024-01-04T00:00:00Z",
      "2024-01-05T00:00:00Z",
  });
  const auto time_index = pluss::make_timeindex({
      "2024-01-01T00:00:00Z",
      "2024-01-02T00:00:00Z",
      "2024-01-03T00:00:00Z",
      "2024-01-04T00:00:00Z",
      "2024-01-05T00:00:00Z",
  });

  const auto metric_mask = make_bool_mask({true, false, true, true, false});
  const auto case_axis = pluss::table::Thread(
      pluss::make_note(std::vector<std::string>{"cs_0", "cs_1", "cs_2", "cs_3", "cs_4"}, shape));

  return pluss::table::DataFrame({
                                    {"id", pluss::table::Thread(ids)},
                                    {"metric", pluss::table::Thread(metric, metric_mask)},
                                    {"city", pluss::table::Thread(city)},
                                    {"text", pluss::table::Thread(text)},
                                    {"timestamp", pluss::table::Thread(timestamp)},
                                    {"time_index", pluss::table::Thread(time_index)},
                                },
                                case_axis);
}

double scalar_stream_value(const pluss::kindle::Stream& stream) {
  const auto values = stream.as_series().to(pluss::codem::ScalarType::Float64).to_vec<double>();
  return values.empty() ? 0.0 : values[0];
}

void demo_stream_methods(const pluss::kindle::Catalog& catalog) {
  std::cout << "\n=== Stream Methods ===\n";
  auto metric = catalog.stream("metric");
  std::cout << "metric: " << metric << "\n";
  std::cout << "kind: " << static_cast<int>(metric.kind())
            << ", dtype: " << metric.dtype_name()
            << ", has_mask: " << metric.has_validity_mask()
            << ", has_cases: " << metric.has_cases() << "\n";

  metric = metric.with_meta("source", "demo").with_meta("unit", "km");
  std::cout << "meta(source): " << metric.meta("source").value_or("none") << "\n";

  const auto cleaned = metric.fillna(0.0).clip(0.0, 42.0);
  const auto transformed = (cleaned + 2.0).log().exp();
  const auto mask = (cleaned > 15.0).as_series();
  const auto filtered = transformed.filter(mask);
  const auto dropped = metric.dropna();
  const auto taken = metric.take(std::vector<std::int64_t>{0, 2, 4});

  std::cout << "cleaned: " << cleaned << "\n";
  std::cout << "filtered (> 15): " << filtered << "\n";
  std::cout << "dropna: " << dropped << "\n";
  std::cout << "take([0,2,4]): " << taken << "\n";
  std::cout << "sum: " << scalar_stream_value(cleaned.sum())
            << ", mean: " << scalar_stream_value(cleaned.mean()) << "\n";
}

void demo_catalog_methods(const pluss::kindle::Catalog& base_catalog) {
  using namespace pluss::kindle::literals;

  std::cout << "\n=== Catalog Methods ===\n";

  const auto ids = base_catalog.stream("id");
  const auto metric = base_catalog.stream("metric");
  const auto city = base_catalog.stream("city");

  const pluss::kindle::Catalog catalog_a(std::vector<std::pair<std::string, pluss::kindle::Stream>>{
      {"id", ids},
      {"metric", metric},
      {"city", city},
  });
  const pluss::kindle::Catalog catalog_b({
      "id"_k = ids,
      "metric"_k = metric,
      "city"_k = city,
  });
  std::vector<std::pair<std::string, pluss::kindle::Stream>> items{
      {"id", ids},
      {"metric", metric},
      {"city", city},
  };
  const pluss::kindle::Catalog catalog_c(std::move(items));

  std::cout << "constructor shapes: "
            << catalog_a.nstreams() << ", "
            << catalog_b.nstreams() << ", "
            << catalog_c.nstreams() << "\n";

  auto catalog = base_catalog;
  std::cout << "keys:";
  for (const auto& k : catalog) std::cout << " " << k;
  std::cout << "\n";

  const auto s_proj = catalog.series();
  const auto n_proj = catalog.note();
  const auto t_proj = catalog.timeline();
  const auto x_proj = catalog.timeindex();
  std::cout << "series/note/timeline/timeindex streams: "
            << s_proj.nstreams() << "/" << n_proj.nstreams() << "/"
            << t_proj.nstreams() << "/" << x_proj.nstreams() << "\n";

  const auto row_mask = make_bool_mask({true, false, true, true, false});
  const auto selected = catalog.select({"id", "city"});
  const auto dropped = catalog.drop(std::vector<std::string>{"time_index"});
  const auto renamed = catalog.rename({{"metric", "distance"}});
  const auto sorted = catalog.sort("id");
  const auto sorted_by = catalog.sort_by({"city", "id"}, {true, false});
  const auto grouped = catalog.group_by("city");
  const auto filtered = catalog.filter(row_mask);
  const auto sliced = catalog.slice(1, 4);
  const auto iloced = catalog.cloc(0, 3);

  std::cout << "selected keys: " << selected.nstreams()
            << ", dropped keys: " << dropped.nstreams()
            << ", renamed has(distance): " << renamed.has("distance") << "\n";
  std::cout << "sorted head id: " << sorted.stream("id").head(1).as_series().to_vec<std::int64_t>()[0] << "\n";
  std::cout << "sorted_by rows: " << sorted_by.count()
            << ", grouped buckets: " << grouped.size()
            << ", filtered rows: " << filtered.count()
            << ", slice rows: " << sliced.count()
            << ", iloc rows: " << iloced.count() << "\n";

  const auto metric_filled = catalog.fillna(0.0, {"metric"});
  const auto metric_dropna = catalog.dropna({"metric"});
  std::cout << "sum(id): " << scalar_stream_value(catalog.sum("id"))
            << ", mean(id): " << scalar_stream_value(catalog.mean("id"))
            << ", min(id): " << scalar_stream_value(catalog.min("id"))
            << ", max(id): " << scalar_stream_value(catalog.max("id"))
            << ", count: " << catalog.count() << "\n";
  std::cout << "metric fillna rows: " << metric_filled.count()
            << ", metric dropna rows: " << metric_dropna.count() << "\n";

  const auto right = pluss::kindle::Catalog(std::vector<std::pair<std::string, pluss::kindle::Stream>>{
      {"id", ids},
      {"bonus", pluss::kindle::Stream(pluss::make_series(std::vector<double>{100.0, 200.0, 300.0, 400.0, 500.0}, {5}))},
  });
  const auto merged = catalog.merge(right, "id");
  std::cout << "merged has bonus: " << merged.has("bonus") << "\n";

  auto applied = catalog.apply("id", [](const pluss::kindle::Stream& s) { return s + 10.0; });
  applied.apply_("id", [](const pluss::kindle::Stream& s) { return s * 2.0; });
  std::cout << "applied id head: " << applied.stream("id").head(1).as_series().to_vec<std::int64_t>()[0] << "\n";

  const auto text_clean = catalog.text_preprocess("text");
  const auto keywords = catalog.extract_keywords("text", 5);
  const auto text_sim = catalog.compute_similarity("text", "city");
  const auto text_filter = catalog.filter_by_text("text", "rain");
  const auto text_feat = catalog.add_text_features("text");
  const auto text_stats = catalog.aggregate_text_stats("text", "mean");
  const auto text_tfidf = catalog.tfidf("text");

  std::cout << "text_clean rows: " << text_clean.count()
            << ", keywords rows: " << keywords.count()
            << ", similarity key added: " << text_sim.has("text__city_similarity")
            << ", text_filter rows: " << text_filter.count()
            << ", text_feat keys: " << text_feat.nstreams()
            << ", text_stats rows: " << text_stats.count()
            << ", tfidf key added: " << text_tfidf.has("text_tfidf_norm") << "\n";

  catalog = catalog.set_stream("id_plus_one", catalog.stream("id") + 1.0);
  catalog.set_stream_("city_copy", catalog.stream("city"));
  catalog = catalog.set_streams({
      "ones"_k = pluss::kindle::Stream(pluss::make_series(std::vector<double>{1, 1, 1, 1, 1}, {5})),
  });
  catalog.set_streams_({
      "flag"_k = pluss::kindle::Stream(pluss::make_series(std::vector<double>{0, 1, 0, 1, 0}, {5})),
  });
  catalog = catalog.drop({"flag"});
  std::cout << "after set/drop streams: " << catalog.nstreams() << "\n";

  pluss::kindle::TabelizePolicy policy;
  policy.align = pluss::kindle::AlignPolicy::Inner;
  policy.index_mode = pluss::kindle::IndexMode::ByCaseLabel;
  policy.include_case_feature = true;
  policy.case_feature_name = "case";
  policy.collision = pluss::kindle::CollisionPolicy::Suffix;
  const auto back_df = catalog.tabelize(policy);
  std::cout << "tabelize shape: [" << back_df.ncases() << " cs x " << back_df.nfeatures() << " feat]\n";
  std::cout << "to_table (table renderer):\n";
  (void)catalog.head(5).to_table(policy, true);

  std::cout << "\nragged catalog -> to_table (inner/outer/strict)\n";
  const std::array<std::int64_t, 1> city_shape{3};
  const pluss::kindle::Catalog ragged({
      "price"_k = pluss::kindle::Stream(pluss::make_series(std::vector<double>{100.0, 101.0, 102.0, 103.0}, {4})),
      "city"_k = pluss::kindle::Stream(pluss::make_note(std::vector<std::string>{"A", "B", "C"}, city_shape)),
      "risk"_k = pluss::kindle::Stream(pluss::make_series(std::vector<double>{0.1, 0.2, 0.3, 0.4, 0.5}, {5})),
  });

  pluss::kindle::TabelizePolicy inner_policy;
  inner_policy.align = pluss::kindle::AlignPolicy::Inner;
  std::cout << "inner alignment:\n";
  (void)ragged.to_table(inner_policy, true);

  pluss::kindle::TabelizePolicy outer_policy;
  outer_policy.align = pluss::kindle::AlignPolicy::Outer;
  std::cout << "outer alignment:\n";
  (void)ragged.to_table(outer_policy, true);

  pluss::kindle::TabelizePolicy strict_policy;
  strict_policy.align = pluss::kindle::AlignPolicy::Strict;
  try {
    (void)ragged.to_table(strict_policy, false);
    std::cout << "strict alignment unexpectedly succeeded\n";
  } catch (const std::exception& e) {
    std::cout << "strict alignment expected failure: " << e.what() << "\n";
  }

  const auto save_path = std::filesystem::temp_directory_path() / "plussmic_kindle_demo.csv";
  catalog.save_csv(save_path.string());
  std::cout << "saved csv: " << save_path.string() << "\n";

  std::cout << "catalog.to_string():\n" << catalog.to_string() << "\n";
  std::cout << "catalog.to_json():\n" << catalog.to_json() << "\n";
  catalog.display();
  catalog.info();
}

void demo_archive_methods(const pluss::table::DataFrame& base_df) {
  std::cout << "\n=== Archive Methods ===\n";
  const auto f0 = base_df.slice(0, 3);
  const auto f1 = base_df.slice(2, 5);
  const auto labels = pluss::table::Thread(
      pluss::make_note(std::vector<std::string>{"batch_a", "batch_b"}, std::array<std::int64_t, 1>{2}));
  const pluss::table::Atlas atlas({f0, f1}, labels);

  const auto archive = atlas.kindelize();;
  std::cout << "archive depth: " << archive.depth()
            << ", has depth labels: " << archive.depth_labels().has_value() << "\n";
  std::cout << "archive:\n" << archive << "\n";

  const auto atlas_back = archive.tabelize();
  std::cout << "archive.tabelize depth: " << atlas_back.depth()
            << ", rows/frame: " << atlas_back.ncases()
            << ", features/frame: " << atlas_back.nfeats() << "\n";
  std::cout << "archive.to_table (atlas renderer):\n";
  (void)archive.to_table({}, true);
}

void demo_loader_entrypoints() {
  std::cout << "\n=== Loader Entry Points ===\n";
  const auto csv_path = data_file("messy_data.csv").string();
  const auto bank_path = data_file("bank-full.csv").string();

  const auto loaded_csv = pluss::kindle::Catalog::load_csv(csv_path);
  std::cout << "load_csv rows: " << loaded_csv.count()
            << ", streams: " << loaded_csv.nstreams() << "\n";

  const auto loaded_generic = pluss::kindle::Catalog::load(
      "csv",
      csv_path,
      pluss::loader::config_from({
          {"ncases", 10},
      }));
  std::cout << "load(csv, config) rows: " << loaded_generic.count() << "\n";

  pluss::loader::ReadCsvOptions key_opts;
  key_opts.ncases = 32;
  const auto keyed = pluss::kindle::Catalog::load_csv_with_key(bank_path, "job", key_opts);
  std::cout << "load_csv_with_key has(_case): " << keyed.has("_case")
            << ", rows: " << keyed.count() << "\n";

  try {
    (void)pluss::kindle::Catalog::from_json("{}");
  } catch (const std::exception& e) {
    std::cout << "from_json expected failure: " << e.what() << "\n";
  }
}

} // namespace

int main() {
  try {
    const auto df = make_base_frame();
    const auto catalog = pluss::kindle::Catalog::kindelize(df);

    std::cout << "Base catalog streams: " << catalog.nstreams()
              << ", rows: " << catalog.count() << "\n";
    demo_stream_methods(catalog);
    demo_catalog_methods(catalog);
    demo_archive_methods(df);
    demo_loader_entrypoints();
  } catch (const std::exception& e) {
    std::cerr << "kindle_model_demo failed: " << e.what() << "\n";
    return 1;
  }
  return 0;
}

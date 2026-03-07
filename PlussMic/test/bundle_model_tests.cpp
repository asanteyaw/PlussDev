#include <cctype>
#include <chrono>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "pluss/pluss.h"

namespace {

using pluss::bundle::Aisle;
using pluss::bundle::Registry;
using pluss::bundle::Slot;

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

std::filesystem::path write_temp_csv(const std::string& stem, const std::string& contents) {
  const auto dir = std::filesystem::temp_directory_path() / "plussmic_bundle_tests";
  std::filesystem::create_directories(dir);
  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  const auto path = dir / (stem + "_" + std::to_string(stamp) + ".csv");
  std::ofstream out(path, std::ios::out | std::ios::trunc | std::ios::binary);
  expect(out.is_open(), "failed to create temporary csv");
  out << contents;
  out.close();
  return path;
}

std::filesystem::path write_temp_text_file(const std::string& stem,
                                           const std::string& ext,
                                           const std::string& contents) {
  const auto dir = std::filesystem::temp_directory_path() / "plussmic_bundle_tests";
  std::filesystem::create_directories(dir);
  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  const auto path = dir / (stem + "_" + std::to_string(stamp) + "." + ext);
  std::ofstream out(path, std::ios::out | std::ios::trunc | std::ios::binary);
  expect(out.is_open(), "failed to create temporary text file");
  out << contents;
  out.close();
  return path;
}

Slot::Value as_series_i64(std::int64_t v) {
  return pluss::make_series(std::vector<std::int64_t>{v}, {1});
}

Slot::Value as_series_f64(double v) {
  return pluss::make_series(std::vector<double>{v}, {1});
}

Slot::Value as_series_bool(bool v) {
  auto s = pluss::Series::zeros({1}, pluss::codem::ScalarType::Bool);
  reinterpret_cast<bool*>(s.raw_data())[0] = v;
  return s;
}

Slot::Value as_note(std::string_view v) {
  return pluss::make_note(v);
}

bool is_series(const Slot::Value& v) {
  return std::holds_alternative<pluss::Series>(v);
}

bool is_note(const Slot::Value& v) {
  return std::holds_alternative<pluss::Note>(v);
}

bool is_timeline(const Slot::Value& v) {
  return std::holds_alternative<pluss::Timeline>(v);
}

bool is_null_value(const Slot::Value& v) {
  return std::holds_alternative<std::monostate>(v);
}

std::int64_t to_i64(const Slot::Value& v) {
  expect(is_series(v), "expected Series value");
  const auto& s = std::get<pluss::Series>(v);
  expect(s.numel() == 1, "expected scalar Series");
  return s.to(pluss::codem::ScalarType::Int64).to_vec<std::int64_t>()[0];
}

double to_f64(const Slot::Value& v) {
  expect(is_series(v), "expected Series value");
  const auto& s = std::get<pluss::Series>(v);
  expect(s.numel() == 1, "expected scalar Series");
  return s.to(pluss::codem::ScalarType::Float64).to_vec<double>()[0];
}

bool to_bool(const Slot::Value& v) {
  expect(is_series(v), "expected Series value");
  const auto& s = std::get<pluss::Series>(v);
  expect(s.numel() == 1, "expected scalar Series");
  return s.to_vec<bool>()[0];
}

std::string to_string_value(const Slot::Value& v) {
  expect(is_note(v), "expected Note value");
  const auto& n = std::get<pluss::Note>(v);
  expect(n.numel() == 1, "expected scalar Note");
  return std::string(n.at(0));
}

Slot make_ids_slot() {
  return Slot({
      as_series_i64(1),
      as_series_i64(2),
      as_series_i64(3),
      as_series_i64(4),
      as_series_i64(5),
  });
}

Slot make_city_slot() {
  return Slot({
      as_note("Accra"),
      as_note("Kumasi"),
      as_note("Tema"),
      as_note("Tamale"),
      as_note("Cape Coast"),
  });
}

void test_slot_methods() {
  auto ids = make_ids_slot();
  expect(ids.size() == 5, "ids slot size mismatch");

  ids.push_(as_series_i64(6));
  expect(ids.size() == 6, "push_ should append");

  const auto head = ids.head(3);
  const auto tail = ids.tail(2);
  const auto slice = ids.slice(1, 4);
  const auto take = ids.take(std::vector<std::int64_t>{0, 2, 4});
  const auto filtered = ids.filter([](const Slot::Value& v) {
    return is_series(v) && to_i64(v) >= 4;
  });
  const auto transformed = ids.transform([](const Slot::Value& v) -> Slot::Value {
    if (is_series(v)) return as_series_i64(to_i64(v) * 2);
    return v;
  });
  const auto concat = head.concat(tail);
  const auto counts = ids.type_counts();

  expect(head.size() == 3, "head size mismatch");
  expect(tail.size() == 2, "tail size mismatch");
  expect(slice.size() == 3, "slice size mismatch");
  expect(take.size() == 3, "take size mismatch");
  expect(filtered.size() == 3, "filter size mismatch");
  expect(transformed.size() == ids.size(), "transform size mismatch");
  expect(concat.size() == 5, "concat size mismatch");
  expect(counts.at("Series") == 6, "type_counts[Series] mismatch");

  const auto stream = ids.to_kindle();
  expect(stream.size() == ids.size(), "Slot::to_kindle size mismatch");
  expect(stream.kind() == pluss::kindle::StreamKind::Series, "Slot::to_kindle kind mismatch");
  const auto roundtrip = Slot::from_kindle(stream);
  expect(roundtrip.size() == ids.size(), "Slot::from_kindle size mismatch");

  const Slot hetero({
      as_series_i64(1),
      as_note("x"),
  });
  expect_throw([&]() { (void)hetero.to_kindle(); }, "heterogeneous slot to_kindle should throw");
}

void test_registry_methods() {
  const auto ids = make_ids_slot();
  const auto city = make_city_slot();
  const auto score = Slot({
      as_series_f64(1.5),
      as_series_f64(2.5),
      as_series_f64(3.5),
      as_series_f64(4.5),
      as_series_f64(5.5),
  });

  auto reg = Registry({
      {"ids", ids},
      {"city", city},
      {"score", score},
  });

  expect(reg.nslots() == 3, "registry nslots mismatch");
  expect(reg.has("ids"), "registry should have ids");
  expect(!reg.get("missing").has_value(), "registry get(missing) should return nullopt");

  const auto head = reg.head(2);
  const auto tail = reg.tail(2);
  const auto slice = reg.slice(1, 4);
  const auto select = reg.select({"ids", "city"});
  const auto drop = reg.drop({"score"});
  const auto rename = reg.rename({{"city", "location"}});
  expect(head.slot("ids").size() == 2, "Registry::head slot size mismatch");
  expect(tail.slot("ids").size() == 2, "Registry::tail slot size mismatch");
  expect(slice.slot("ids").size() == 3, "Registry::slice slot size mismatch");
  expect(select.nslots() == 2, "Registry::select slot count mismatch");
  expect(drop.nslots() == 2, "Registry::drop slot count mismatch");
  expect(rename.has("location"), "Registry::rename should rename key");
  expect_throw([&]() { (void)reg.select({"unknown"}); }, "Registry::select unknown key should throw");

  const auto applied = reg.apply("ids", [](const Slot& s) {
    return s.transform([](const Slot::Value& v) -> Slot::Value {
      if (is_series(v)) return as_series_i64(to_i64(v) + 10);
      return v;
    });
  });
  expect(to_i64(applied.slot("ids").at(0)) == 11, "Registry::apply transform mismatch");

  reg.apply_("score", [](const Slot& s) {
    return s.transform([](const Slot::Value& v) -> Slot::Value {
      if (is_series(v)) return as_series_f64(to_f64(v) * 2.0);
      return v;
    });
  });
  expect(to_f64(reg.slot("score").at(0)) == 3.0, "Registry::apply_ mutate mismatch");

  const auto upper = reg.apply_all([](const Slot& s) {
    return s.transform([](const Slot::Value& v) -> Slot::Value {
      if (is_note(v)) {
        auto out = to_string_value(v);
        for (auto& ch : out) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        return as_note(out);
      }
      return v;
    });
  });
  expect(to_string_value(upper.slot("city").at(0)) == "ACCRA", "Registry::apply_all mismatch");

  const auto sizes = reg.slot_sizes();
  expect(sizes.at("ids") == 5, "Registry::slot_sizes mismatch");

  const auto other = Registry({
      {"ids", Slot({as_series_i64(9)})},
      {"flag", Slot({as_series_bool(true)})},
  });
  const auto combined = reg.combine(other, "_rhs");
  expect(combined.nslots() == 5, "Registry::combine slot count mismatch");
  expect(combined.has("ids_rhs"), "Registry::combine should suffix colliding key");

  const auto with_new = reg.set_slot("status", Slot({as_note("ok"), as_note("ok")}));
  expect(with_new.has("status"), "Registry::set_slot should add new key");
  reg.set_slot_("city_copy", city);
  expect(reg.has("city_copy"), "Registry::set_slot_ should add new key");

  expect(!reg.to_string().empty(), "Registry::to_string should not be empty");
  expect(!reg.to_json().empty(), "Registry::to_json should not be empty");

  std::int64_t iter_count = 0;
  for (const auto& key : reg) {
    (void)key;
    ++iter_count;
  }
  expect(iter_count == reg.nslots(), "Registry iterator mismatch");

  const auto clean = Registry({
      {"ids", ids},
      {"city", city},
  });
  const auto cat = clean.to_kindle();
  expect(cat.nstreams() == 2, "Registry::to_kindle stream count mismatch");
  const auto from_cat = Registry::from_kindle(cat);
  expect(from_cat.nslots() == 2, "Registry::from_kindle slot count mismatch");

  const auto mixed = Registry({
      {"ids", ids},
      {"mixed", Slot({as_series_i64(7), as_note("bad")})},
  });
  expect_throw([&]() { (void)mixed.to_kindle(); }, "Registry::to_kindle should throw on heterogeneous slot");
}

void test_registry_loaders() {
  const auto csv_path = write_temp_csv(
      "registry_loader",
      "id,mixed,when,text\n"
      "1,10,2024-01-01T00:00:00Z,alpha\n"
      "2,hello,,beta\n"
      "3,3.5,2024-01-03T00:00:00Z,\n");

  const auto reg_csv = Registry::load_csv(csv_path.string());
  expect(reg_csv.has("id"), "Registry::load_csv should load id");
  expect(reg_csv.has("mixed"), "Registry::load_csv should load mixed");
  expect(reg_csv.has("when"), "Registry::load_csv should load when");
  expect(reg_csv.has("text"), "Registry::load_csv should load text");
  expect(reg_csv.slot("mixed").size() == 3, "Registry::load_csv should preserve slot length");

  const auto& mixed0 = reg_csv.slot("mixed").at(0);
  const auto& mixed1 = reg_csv.slot("mixed").at(1);
  const auto& mixed2 = reg_csv.slot("mixed").at(2);
  expect(is_series(mixed0), "mixed[0] should infer as numeric Series scalar");
  expect(is_note(mixed1), "mixed[1] should infer as Note scalar");
  expect(is_series(mixed2), "mixed[2] should infer as numeric Series scalar");

  const auto& when1 = reg_csv.slot("when").at(1);
  expect(is_null_value(when1), "when[1] should infer as null/monostate");
  expect(is_timeline(reg_csv.slot("when").at(0)), "when[0] should infer as Timeline scalar");

  const auto reg_via_load = Registry::load("csv", csv_path.string());
  expect(reg_via_load.nslots() == reg_csv.nslots(), "Registry::load(csv) should match Registry::load_csv");

  pluss::loader::Config csv_cfg;
  csv_cfg.emplace("ncases", 2);
  const auto reg_cfg = Registry::load_csv(csv_path.string(), csv_cfg);
  expect(reg_cfg.slot("id").size() == 2, "Registry::load_csv(config) should honor nrows");

  const auto txt_path = write_temp_text_file(
      "registry_loader_txt",
      "txt",
      "id mixed when text\n"
      "1 10 2024-01-01T00:00:00Z alpha\n"
      "2 hello  beta\n"
      "3 3.5 2024-01-03T00:00:00Z \n");
  const auto reg_txt = Registry::load_txt(
      txt_path.string(),
      pluss::loader::config_from({
          {"txt_mode", "whitespace"},
          {"header", true},
      }));
  expect(reg_txt.has("id"), "Registry::load_txt should load id");
  expect(reg_txt.slot("id").size() == 3, "Registry::load_txt row count mismatch");

  const auto json_path = write_temp_text_file(
      "registry_loader_json",
      "json",
      R"({"id":[1,2,3], "mixed":[10,"hello",3.5], "when":["2024-01-01T00:00:00Z",null,"2024-01-03T00:00:00Z"]})");
  const auto reg_json = Registry::load_json(json_path.string());
  expect(reg_json.has("mixed"), "Registry::load_json should load mixed");
  expect(reg_json.slot("mixed").size() == 3, "Registry::load_json row count mismatch");
  expect(is_series(reg_json.slot("mixed").at(0)), "Registry::load_json mixed[0] kind mismatch");
  expect(is_note(reg_json.slot("mixed").at(1)), "Registry::load_json mixed[1] kind mismatch");
  expect(is_series(reg_json.slot("mixed").at(2)), "Registry::load_json mixed[2] kind mismatch");

  auto reg_save_json = write_temp_text_file("registry_save_json", "json", "");
  reg_csv.save_json(reg_save_json.string());
  {
    std::ifstream in(reg_save_json, std::ios::in | std::ios::binary);
    expect(in.is_open(), "Registry::save_json should create output file");
    std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    expect(text.find("\"_type\": \"Registry\"") != std::string::npos,
           "heterogeneous Registry::save_json should emit native Registry JSON");
  }

  const auto stream_compatible = Registry({
      {"id", Slot({as_series_i64(1), as_series_i64(2), as_series_i64(3)})},
      {"score", Slot({as_series_f64(1.0), as_series_f64(2.0), as_series_f64(3.0)})},
  });
  auto reg_json_table = write_temp_text_file("registry_save_json_table", "json", "");
  stream_compatible.save_json(reg_json_table.string());
  const auto reg_json_roundtrip = Registry::load_json(reg_json_table.string());
  expect(reg_json_roundtrip.has("id"), "stream-compatible Registry::save_json roundtrip should preserve id field");

  auto reg_save_txt = write_temp_text_file("registry_save_txt", "txt", "");
  stream_compatible.save_txt(reg_save_txt.string(), pluss::loader::config_from({{"header", true}}));
  const auto reg_txt_roundtrip = Registry::load_txt(
      reg_save_txt.string(),
      pluss::loader::config_from({{"header", true}, {"txt_mode", "delimited"}}));
  expect(reg_txt_roundtrip.slot("id").size() == stream_compatible.slot("id").size(),
         "Registry::save_txt roundtrip row count mismatch");

  auto reg_save_as_json = write_temp_text_file("registry_save_as_json", "json", "");
  stream_compatible.save_as("json", reg_save_as_json.string(), pluss::loader::config_from({{"orient", "split"}}));
  const auto reg_save_as_roundtrip = Registry::load_json(reg_save_as_json.string());
  expect(reg_save_as_roundtrip.slot("id").size() == stream_compatible.slot("id").size(),
         "Registry::save_as(json) roundtrip row count mismatch");

  const auto hetero = Registry({
      {"mixed", Slot({as_series_i64(1), as_note("x")})},
  });
  auto hetero_json = write_temp_text_file("registry_hetero_json", "json", "");
  hetero.save_json(hetero_json.string());
  expect(std::filesystem::exists(hetero_json), "Registry::save_json should work for heterogeneous Registry");

  expect_throw(
      [&]() { (void)Registry::load_parquet("not_implemented.parquet"); },
      "Registry::load_parquet should throw on invalid parquet path");
}

void test_aisle_methods() {
  const auto r0 = Registry({
      {"ids", Slot({as_series_i64(1), as_series_i64(2), as_series_i64(3)})},
      {"city", Slot({as_note("Accra"), as_note("Kumasi"), as_note("Tema")})},
  });
  const auto r1 = Registry({
      {"ids", Slot({as_series_i64(10), as_series_i64(20), as_series_i64(30)})},
      {"city", Slot({as_note("Accra"), as_note("Kumasi"), as_note("Tema")})},
  });
  const auto labels = Slot({as_note("train"), as_note("test")});
  const auto aisle = Aisle({r0, r1}, labels);

  expect(aisle.depth() == 2, "Aisle depth mismatch");
  expect(aisle.labels().has_value(), "Aisle labels should be present");
  expect(aisle.head(1).depth() == 1, "Aisle::head depth mismatch");
  expect(aisle.tail(1).depth() == 1, "Aisle::tail depth mismatch");
  expect(aisle.slice(0, 2).depth() == 2, "Aisle::slice depth mismatch");

  const auto mapped = aisle.map([](const Registry& reg) {
    return reg.set_slot("flag", Slot({as_series_bool(true), as_series_bool(false), as_series_bool(true)}));
  });
  expect(mapped.registry(0).has("flag"), "Aisle::map should transform registries");

  const auto selected = aisle.select({"ids"});
  expect(selected.registry(0).nslots() == 1, "Aisle::select should project keys");

  const auto applied = aisle.apply_slot("ids", [](const Slot& s) {
    return s.transform([](const Slot::Value& v) -> Slot::Value {
      if (is_series(v)) return as_series_i64(to_i64(v) * 2);
      return v;
    });
  });
  expect(to_i64(applied.registry(0).slot("ids").at(0)) == 2, "Aisle::apply_slot mismatch");

  const auto concat = aisle.concat_depth(aisle);
  expect(concat.depth() == 4, "Aisle::concat_depth mismatch");
  expect(concat.labels().has_value(), "concat_depth should keep labels when both inputs labeled");
  expect(concat.labels()->size() == 4, "concat_depth labels size mismatch");

  const auto archive = aisle.to_kindle();
  expect(archive.depth() == 2, "Aisle::to_kindle archive depth mismatch");
  const auto back = Aisle::from_kindle(archive);
  expect(back.depth() == 2, "Aisle::from_kindle depth mismatch");

  expect(!aisle.to_string().empty(), "Aisle::to_string should not be empty");
  expect(!aisle.to_json().empty(), "Aisle::to_json should not be empty");

  expect_throw(
      [&]() {
        (void)Aisle({r0, r1}, Slot({as_note("one_label")}));
      },
      "Aisle constructor should enforce label/depth alignment");
}

} // namespace

int main() {
  try {
    test_slot_methods();
    test_registry_methods();
    test_registry_loaders();
    test_aisle_methods();
  } catch (const std::exception& e) {
    std::cerr << "[bundle_model_tests] failure: " << e.what() << "\n";
    return 1;
  }

  std::cout << "[bundle_model_tests] all checks passed\n";
  return 0;
}

#include <cctype>
#include <chrono>
#include <cstdlib>
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
#include "pluss/tils.h"

namespace {

using pluss::bundle::Axel;
using pluss::bundle::Registry;
using pluss::bundle::Slot;
using pluss::bundle::SlotKind;

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
  expect(ids.size() == 5, "ids field size mismatch");
  expect(ids.shape() == std::make_pair<std::int64_t, std::int64_t>(5, 1),
         "Slot::shape should be (size, 1)");
  expect(ids.ptype() == "Series", "homogeneous Series Slot ptype mismatch");
  expect(ids.kind() == SlotKind::Series, "homogeneous Series Slot kind mismatch");

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

  const Slot nullable({
      as_series_i64(1),
      std::monostate{},
      as_series_i64(1),
      as_series_i64(3),
  });
  expect(nullable.kind() == SlotKind::Series, "nullable homogeneous Slot kind should ignore nulls");
  expect_throw([&]() { (void)nullable.to_kindle(); }, "nullable Slot should reject to_kindle");
  const auto null_mask = nullable.isnull();
  const auto present_mask = nullable.notnull();
  const auto invalid_mask = nullable.invalid();
  expect(!to_bool(null_mask.at(0)), "Slot::isnull should mark present scalar false");
  expect(to_bool(null_mask.at(1)), "Slot::isnull should mark monostate true");
  expect(!to_bool(present_mask.at(1)), "Slot::notnull should mark monostate false");
  expect(to_bool(invalid_mask.at(1)), "Slot::invalid should mark monostate true");

  const auto filled = nullable.fillnull(9.0);
  expect(to_i64(filled.at(1)) == 9, "Slot::fillnull(double) should fill monostate");
  const auto dropped_nulls = nullable.dropnull();
  expect(dropped_nulls.size() == 3, "Slot::dropnull should remove monostate values");

  const Slot dup_ids({
      as_series_i64(1),
      as_series_i64(1),
      as_series_i64(2),
      as_series_i64(3),
      as_series_i64(3),
  });
  expect(dup_ids.distinct().size() == 3, "Slot::distinct should delegate to Stream");
  expect(dup_ids.freq().size() >= 3, "Slot::freq should delegate to Stream");
  expect(to_i64(dup_ids.ndistinct().at(0)) == 3, "Slot::ndistinct should return scalar count");
  expect(to_f64(dup_ids.sum().at(0)) == 10.0, "Slot::sum should reduce through Stream");
  expect(to_f64(dup_ids.mean().at(0)) == 2.0, "Slot::mean should reduce through Stream");
  expect(to_i64(dup_ids.min().at(0)) == 1, "Slot::min should reduce through Stream");
  expect(to_i64(dup_ids.max().at(0)) == 3, "Slot::max should reduce through Stream");
  const auto cast_note = dup_ids.cast().note();
  expect(is_note(cast_note.at(0)), "Slot::cast(Note) should convert payload");

  const Slot note_dates({
      as_note("2024-01-01"),
      std::monostate{},
      as_note("2024-01-03"),
  });
  const auto cast_timeline = note_dates.cast({.format = "%Y-%m-%d"}).timeline();
  expect(is_timeline(cast_timeline.at(0)), "nullable homogeneous Slot cast should batch notes to timeline");
  expect(is_null_value(cast_timeline.at(1)), "nullable homogeneous Slot cast should preserve nulls");
  expect(is_timeline(cast_timeline.at(2)), "nullable homogeneous Slot cast should batch all note positions");
  const auto lower_notes = note_dates.nt().lower();
  expect(to_string_value(lower_notes.at(0)) == "2024-01-01",
         "Slot::nt().lower should delegate through Stream and preserve values");
  expect(is_null_value(lower_notes.at(1)), "Slot::nt().lower should preserve nulls");
  const auto years = cast_timeline.tl().year();
  expect(to_i64(years.at(0)) == 2024, "Slot::tl().year should delegate through Stream");
  expect(is_null_value(years.at(1)), "Slot::tl().year should preserve nulls");
  expect(to_i64(years.at(2)) == 2024, "Slot::tl().year should keep later values");

  const Slot hetero({
      as_series_i64(1),
      as_note("x"),
  });
  expect(hetero.ptype() == "Mixed", "heterogeneous Slot ptype should be Mixed");
  expect(hetero.kind() == SlotKind::Mixed, "heterogeneous Slot kind should be Mixed");
  expect_throw([&]() { (void)hetero.to_kindle(); }, "heterogeneous slot to_kindle should throw");
  expect_throw([&]() { (void)hetero.fillnull(0.0); }, "Slot::fillnull(double) should reject non-Series values");
  expect_throw([&]() { (void)hetero.nt().lower(); }, "Slot::nt() should reject heterogeneous slots");

  const Slot all_null({
      std::monostate{},
      std::monostate{},
  });
  expect(all_null.kind() == SlotKind::Null, "all-null Slot kind should be Null");
  expect_throw([&]() { (void)all_null.to_kindle(); }, "all-null Slot should reject to_kindle");

  const Slot empty_slot({});
  expect(empty_slot.kind() == SlotKind::Empty, "empty Slot kind should be Empty");
  expect(empty_slot.to_kindle().size() == 0, "empty Slot should convert to empty Stream");
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

  expect(reg.nfields() == 3, "registry nfields mismatch");
  expect(reg.size() == reg.nfields(), "Registry::size should match nfields");
  expect(reg.nstate() == 5, "Registry::nstate mismatch");
  expect(reg.minstate() == 5, "Registry::minstate mismatch");
  expect(reg.maxstate() == 5, "Registry::maxstate mismatch");
  expect(reg.fields() == std::vector<std::string>({"ids", "city", "score"}),
         "Registry::fields should return field names in storage order");
  expect(reg.shape() == std::make_pair<std::int64_t, std::int64_t>(5, 3),
         "Registry::shape should be (max field length, number of fields)");
  expect(reg.has("ids"), "registry should have ids");
  expect(!reg.get("missing").has_value(), "registry get(missing) should return nullopt");

  const auto ragged = Registry({
      {"short", Slot({as_series_i64(1), as_series_i64(2)})},
      {"long", Slot({as_note("a"), as_note("b"), as_note("c")})},
  });
  expect(ragged.minstate() == 2, "Registry::minstate should inspect ragged registries");
  expect(ragged.maxstate() == 3, "Registry::maxstate should inspect ragged registries");
  expect_throw([&]() { (void)ragged.nstate(); }, "Registry::nstate should throw for ragged registries");
  expect(ragged.shape() == std::make_pair<std::int64_t, std::int64_t>(3, 2),
         "Registry::shape should support length-heterogeneous fields");

  const auto empty = Registry({});
  expect(empty.nstate() == 0, "empty Registry::nstate should be zero");
  expect(empty.minstate() == 0, "empty Registry::minstate should be zero");
  expect(empty.maxstate() == 0, "empty Registry::maxstate should be zero");

  const auto head = reg.head(2);
  const auto tail = reg.tail(2);
  const auto slice = reg.slice(1, 4);
  const auto select = reg.fields({"ids", "city"});
  const auto drop = reg.drop({"score"});
  const auto rename = reg.rename({{"city", "location"}});
  expect(head.fields("ids").size() == 2, "Registry::head field size mismatch");
  expect(tail.fields("ids").size() == 2, "Registry::tail field size mismatch");
  expect(slice.fields("ids").size() == 3, "Registry::slice field size mismatch");
  expect(select.nfields() == 2, "Registry::fields field count mismatch");
  expect(drop.nfields() == 2, "Registry::drop field count mismatch");
  expect(rename.has("location"), "Registry::rename should rename key");
  expect_throw([&]() { (void)reg.fields({"unknown"}); }, "Registry::fields unknown key should throw");

  const auto applied = reg.set("ids", reg.fields("ids").transform([](const Slot::Value& v) -> Slot::Value {
    if (is_series(v)) return as_series_i64(to_i64(v) + 10);
    return v;
  }));
  expect(to_i64(applied.fields("ids").at(0)) == 11, "Registry::set(transform) mismatch");

  reg.set_("score", reg.fields("score").transform([](const Slot::Value& v) -> Slot::Value {
    if (is_series(v)) return as_series_f64(to_f64(v) * 2.0);
    return v;
  }));
  expect(to_f64(reg.fields("score").at(0)) == 3.0, "Registry::set_ transform mismatch");

  const auto upper = reg.map([](const Slot& s) {
    return s.transform([](const Slot::Value& v) -> Slot::Value {
      if (is_note(v)) {
        auto out = to_string_value(v);
        for (auto& ch : out) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        return as_note(out);
      }
      return v;
    });
  });
  expect(to_string_value(upper.fields("city").at(0)) == "ACCRA", "Registry::map mismatch");

  const auto sizes = reg.sizes();
  expect(sizes.at("ids") == 5, "Registry::sizes mismatch");

  const auto nullable = Registry({
      {"score", Slot({as_series_f64(1.0), std::monostate{}, as_series_f64(3.0)})},
      {"city", Slot({as_note("Accra"), as_note("Kumasi"), std::monostate{}})},
  });
  const auto score_nulls = nullable.fields({"score"}).isnull();
  expect(score_nulls.nfields() == 1, "Registry::fields should project selected fields");
  expect(to_bool(score_nulls.fields("score").at(1)), "Registry::fields(...).isnull should delegate to selected Slot");
  const auto all_nulls = nullable.isnull();
  expect(to_bool(all_nulls.fields("city").at(2)), "Registry::isnull() should apply to all fields");
  const auto filled_scores = nullable.fields({"score"}).fillnull(0.0);
  expect(to_f64(filled_scores.fields("score").at(1)) == 0.0, "Registry::fields(...).fillnull(double) mismatch");
  const auto filled_city = nullable.fields({"city"}).fillnull("missing");
  expect(to_string_value(filled_city.fields("city").at(2)) == "missing", "Registry::fields(...).fillnull(string) mismatch");
  const auto filled_scores_accessor = nullable.fields("score").fillnull(0.0);
  expect(to_f64(filled_scores_accessor.at(1)) == 0.0,
         "Registry::fields(key).fillnull should delegate to Slot");
  const auto lowered_city = nullable.fields({"city"}).nt().lower();
  expect(to_string_value(lowered_city.fields("city").at(0)) == "accra",
         "Registry::fields(...).nt().lower should delegate to Slot");
  const auto dropped = nullable.fields({"score"}).dropnull();
  expect(dropped.fields("score").size() == 2, "Registry::fields(...).dropnull should delegate to selected Slot");

  const auto temporal_reg = Registry({
      {"when", Slot({as_note("2024-01-01"), std::monostate{}, as_note("2024-01-03")})},
  });
  const auto tl_reg = temporal_reg.fields({"when"}).cast({.format = "%Y-%m-%d"}).timeline();
  expect(is_timeline(tl_reg.fields("when").at(0)),
         "Registry::fields(...).cast(options).timeline should delegate to Slot");
  const auto years = tl_reg.fields({"when"}).tl().year();
  expect(to_i64(years.fields("when").at(0)) == 2024 && is_null_value(years.fields("when").at(1)),
         "Registry::fields(...).tl().year should preserve nullable timeline slots");

  const auto duplicate_reg = Registry({
      {"ids", Slot({as_series_i64(1), as_series_i64(1), as_series_i64(2)})},
      {"city", Slot({as_note("Accra"), as_note("Accra"), as_note("Kumasi")})},
  });
  expect(duplicate_reg.fields({"ids"}).distinct().fields("ids").size() == 2,
         "Registry::distinct(fields) should delegate to selected Slot");
  expect(duplicate_reg.fields({"city"}).freq().fields("city").size() >= 2,
         "Registry::freq(fields) should delegate to selected Slot");
  expect(to_i64(duplicate_reg.fields({"ids"}).ndistinct().fields("ids").at(0)) == 2,
         "Registry::ndistinct(fields) should delegate to selected Slot");
  expect(to_f64(duplicate_reg.fields("ids").sum().at(0)) == 4.0, "Slot::sum through Registry::fields mismatch");
  expect(to_f64(duplicate_reg.fields("ids").mean().at(0)) == 4.0 / 3.0, "Slot::mean through Registry::fields mismatch");
  const auto ids_as_note = duplicate_reg.fields({"ids"}).cast().note();
  expect(is_note(ids_as_note.fields("ids").at(0)), "Registry::cast(fields) should delegate to Slot");
  const auto ids_as_note_accessor = duplicate_reg.fields({"ids"}).cast().note();
  expect(is_note(ids_as_note_accessor.fields("ids").at(0)),
         "Registry::fields(...).cast().note should delegate to Slot");

  const auto axis_reg = Registry({
      {"score", Slot({as_series_f64(1.0), std::monostate{}, as_series_f64(3.0)})},
      {"city", Slot({as_note("Accra"), as_note("Kumasi"), std::monostate{}})},
      {"mixed", Slot({as_series_i64(1), as_note("x")})},
      {"all_null", Slot({std::monostate{}, std::monostate{}})},
      {"vacant", Slot({})},
  });
  expect(axis_reg.fieldaxis().series().has("score"), "Registry::fieldaxis().series should select Series slots");
  expect(axis_reg.fieldaxis().note().has("city"), "Registry::fieldaxis().note should select Note slots");
  expect(axis_reg.fieldaxis().mixed().has("mixed"), "Registry::fieldaxis().mixed should select Mixed slots");
  expect(axis_reg.fieldaxis().nulls().has("all_null"), "Registry::fieldaxis().nulls should select all-null slots");
  expect(axis_reg.fieldaxis().empty().has("vacant"), "Registry::fieldaxis().empty should select empty slots");
  expect(axis_reg.fieldaxis().prefix("sc").has("score"), "Registry::fieldaxis().prefix should filter by field prefix");

  const auto other = Registry({
      {"ids", Slot({as_series_i64(9)})},
      {"flag", Slot({as_series_bool(true)})},
  });
  const auto combined = reg.combine(other, "_rhs");
  expect(combined.nfields() == 5, "Registry::combine field count mismatch");
  expect(combined.has("ids_rhs"), "Registry::combine should suffix colliding key");

  const auto with_new = reg.set("status", Slot({as_note("ok"), as_note("ok")}));
  expect(with_new.has("status"), "Registry::set should add new field");
  reg.set_("city_copy", city);
  expect(reg.has("city_copy"), "Registry::set_ should add new field");

  expect(!reg.to_string().empty(), "Registry::to_string should not be empty");
  expect(!reg.to_json().empty(), "Registry::to_json should not be empty");

  std::int64_t iter_count = 0;
  for (const auto& key : reg) {
    (void)key;
    ++iter_count;
  }
  expect(iter_count == reg.nfields(), "Registry iterator mismatch");

  const auto clean = Registry({
      {"ids", ids},
      {"city", city},
  });
  const auto cat = clean.to_kindle();
  expect(cat.nkeys() == 2, "Registry::to_kindle key count mismatch");
  const auto from_cat = Registry::from_kindle(cat);
  expect(from_cat.nfields() == 2, "Registry::from_kindle field count mismatch");

  const auto mixed = Registry({
      {"ids", ids},
      {"mixed", Slot({as_series_i64(7), as_note("bad")})},
  });
  expect_throw([&]() { (void)mixed.to_kindle(); }, "Registry::to_kindle should throw on heterogeneous slot");
}

void test_bundle_lvalue_name_ranges() {
  const auto reg = Registry({
      {"ids", Slot({as_series_i64(1), as_series_i64(2), as_series_i64(3)})},
      {"city", Slot({as_note("Accra"), as_note("Kumasi"), as_note("Tema")})},
      {"score", Slot({as_series_f64(1.5), std::monostate{}, as_series_f64(3.5)})},
  });
  const std::vector<std::string> keys{"ids", "city"};
  const auto selected = reg.fields(keys);
  expect(selected.nfields() == 2, "Registry lvalue fields should accept reusable key vectors");
  expect(reg.has_all(keys), "Registry lvalue has_all should accept reusable key vectors");

  const std::vector<std::string> score{"score"};
  const auto filled = reg.fields(score).fillnull(0.0);
  expect(to_f64(filled.fields("score").at(1)) == 0.0, "Registry lvalue fillnull should delegate to Slot");

  const auto casted = reg.fields(std::vector<std::string>{"ids"}).cast().note();
  expect(is_note(casted.fields("ids").at(0)), "Registry lvalue cast should delegate to Slot");

  const auto axel = Axel({
      reg,
      Registry({
          {"ids", Slot({as_series_i64(10), as_series_i64(20), as_series_i64(30)})},
          {"city", Slot({as_note("Tamale"), as_note("Tema"), as_note("Accra")})},
          {"score", Slot({as_series_f64(9.0), std::monostate{}, as_series_f64(7.0)})},
      }),
  });
  const auto axel_selected = axel.fields(keys);
  expect(axel_selected.registry(0).nfields() == 2, "Axel lvalue fields should delegate to Registry");

  const auto axel_dropnull = axel.fields(score).dropnull();
  expect(axel_dropnull.registry(0).fields("score").size() == 2,
         "Axel lvalue dropnull should delegate to Registry");
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
  expect(reg_csv.fields("mixed").size() == 3, "Registry::load_csv should preserve field length");

  const auto& mixed0 = reg_csv.fields("mixed").at(0);
  const auto& mixed1 = reg_csv.fields("mixed").at(1);
  const auto& mixed2 = reg_csv.fields("mixed").at(2);
  expect(is_series(mixed0), "mixed[0] should infer as numeric Series scalar");
  expect(is_note(mixed1), "mixed[1] should infer as Note scalar");
  expect(is_series(mixed2), "mixed[2] should infer as numeric Series scalar");

  const auto& when1 = reg_csv.fields("when").at(1);
  expect(is_null_value(when1), "when[1] should infer as null/monostate");
  expect(is_timeline(reg_csv.fields("when").at(0)), "when[0] should infer as Timeline scalar");

  const auto reg_via_load = Registry::load("csv", csv_path.string());
  expect(reg_via_load.nfields() == reg_csv.nfields(), "Registry::load(csv) should match Registry::load_csv");

  pluss::loader::Config csv_cfg;
  csv_cfg.emplace("ncases", 2);
  const auto reg_cfg = Registry::load_csv(csv_path.string(), csv_cfg);
  expect(reg_cfg.fields("id").size() == 2, "Registry::load_csv(config) should honor nrows");

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
  expect(reg_txt.fields("id").size() == 3, "Registry::load_txt row count mismatch");

  const auto json_path = write_temp_text_file(
      "registry_loader_json",
      "json",
      R"({"id":[1,2,3], "mixed":[10,"hello",3.5], "when":["2024-01-01T00:00:00Z",null,"2024-01-03T00:00:00Z"]})");
  const auto reg_json = Registry::load_json(json_path.string());
  expect(reg_json.has("mixed"), "Registry::load_json should load mixed");
  expect(reg_json.fields("mixed").size() == 3, "Registry::load_json row count mismatch");
  expect(is_series(reg_json.fields("mixed").at(0)), "Registry::load_json mixed[0] kind mismatch");
  expect(is_note(reg_json.fields("mixed").at(1)), "Registry::load_json mixed[1] kind mismatch");
  expect(is_series(reg_json.fields("mixed").at(2)), "Registry::load_json mixed[2] kind mismatch");

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
  expect(reg_txt_roundtrip.fields("id").size() == stream_compatible.fields("id").size(),
         "Registry::save_txt roundtrip row count mismatch");

  auto reg_save_as_json = write_temp_text_file("registry_save_as_json", "json", "");
  stream_compatible.save_as("json", reg_save_as_json.string(), pluss::loader::config_from({{"orient", "split"}}));
  const auto reg_save_as_roundtrip = Registry::load_json(reg_save_as_json.string());
  expect(reg_save_as_roundtrip.fields("id").size() == stream_compatible.fields("id").size(),
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

void test_axel_methods() {
  const auto r0 = Registry({
      {"ids", Slot({as_series_i64(1), as_series_i64(2), as_series_i64(3)})},
      {"city", Slot({as_note("Accra"), as_note("Kumasi"), as_note("Tema")})},
  });
  const auto r1 = Registry({
      {"ids", Slot({as_series_i64(10), as_series_i64(20), as_series_i64(30)})},
      {"city", Slot({as_note("Accra"), as_note("Kumasi"), as_note("Tema")})},
  });
  const auto labels = Slot({as_note("train"), as_note("test")});
  const auto axel = Axel({r0, r1}, labels);

  expect(axel.depth() == 2, "Axel depth mismatch");
  expect(axel.size() == axel.depth(), "Axel::size should match depth");
  expect(axel.labels().has_value(), "Axel labels should be present");
  expect(axel.head(1).depth() == 1, "Axel::head depth mismatch");
  expect(axel.tail(1).depth() == 1, "Axel::tail depth mismatch");
  expect(axel.slice(0, 2).depth() == 2, "Axel::slice depth mismatch");

  const auto mapped = axel.map([](const Registry& reg) {
    return reg.set("flag", Slot({as_series_bool(true), as_series_bool(false), as_series_bool(true)}));
  });
  expect(mapped.registry(0).has("flag"), "Axel::map should transform registries");
  expect(axel.has("ids"), "Axel::has should scan registries");
  expect(!axel.has("missing"), "Axel::has should return false for absent fields");

  const auto selected = axel.fields({"ids"});
  expect(selected.registry(0).nfields() == 1, "Axel::fields should project keys");
  const auto dropped = axel.drop({"city"});
  expect(!dropped.registry(0).has("city"), "Axel::drop should delegate to Registry");
  const auto renamed = axel.rename("city", "town");
  expect(renamed.registry(0).has("town"), "Axel::rename should delegate to Registry");

  const auto applied = axel.map([](const Registry& reg) {
    if (!reg.has("ids")) return reg;
    return reg.set("ids", reg.fields("ids").transform([](const Slot::Value& v) -> Slot::Value {
      if (is_series(v)) return as_series_i64(to_i64(v) * 2);
      return v;
    }));
  });
  expect(to_i64(applied.registry(0).fields("ids").at(0)) == 2, "Axel::map(set) mismatch");

  const auto nullable_axel = Axel({
      Registry({
          {"score", Slot({as_series_f64(1.0), std::monostate{}, as_series_f64(3.0)})},
          {"city", Slot({as_note("Accra"), as_note("Kumasi"), std::monostate{}})},
      }),
      Registry({
          {"score", Slot({as_series_f64(4.0), as_series_f64(5.0), std::monostate{}})},
          {"city", Slot({as_note("Tema"), std::monostate{}, as_note("Tamale")})},
      }),
  });
  const auto ax_nulls = nullable_axel.fields({"score"}).isnull();
  expect(to_bool(ax_nulls.registry(0).fields("score").at(1)), "Axel::fields(...).isnull should delegate to Registry");
  const auto ax_filled = nullable_axel.fields({"score"}).fillnull(0.0);
  expect(to_f64(ax_filled.registry(1).fields("score").at(2)) == 0.0,
         "Axel::fields(...).fillnull(double) should delegate to Registry");
  const auto ax_filled_accessor = nullable_axel.fields({"score"}).fillnull(0.0);
  expect(to_f64(ax_filled_accessor.registry(1).fields("score").at(2)) == 0.0,
         "Axel::fields(...).fillnull should delegate to Registry");
  const auto ax_dropnull = nullable_axel.fields({"score"}).dropnull();
  expect(ax_dropnull.registry(0).fields("score").size() == 2,
         "Axel::fields(...).dropnull should delegate to Registry");
  const auto ax_upper = nullable_axel.fields({"city"}).nt().upper();
  expect(to_string_value(ax_upper.registry(0).fields("city").at(0)) == "ACCRA",
         "Axel::fields(...).nt().upper should delegate to Registry");

  const auto ax_distinct = axel.fields({"city"}).distinct();
  expect(ax_distinct.registry(0).fields("city").size() == 3, "Axel::distinct(fields) should delegate to Registry");
  const auto ax_ndistinct = axel.fields({"city"}).ndistinct();
  expect(to_i64(ax_ndistinct.registry(0).fields("city").at(0)) == 3,
         "Axel::ndistinct(fields) should delegate to Registry");
  const auto ax_cast = axel.fields({"ids"}).cast().note();
  expect(is_note(ax_cast.registry(0).fields("ids").at(0)), "Axel::cast(fields) should delegate to Registry");
  const auto ax_cast_accessor = axel.fields({"ids"}).cast().note();
  expect(is_note(ax_cast_accessor.registry(0).fields("ids").at(0)),
         "Axel::fields(...).cast().note should delegate to Registry");
  const auto ax_fieldaxis = nullable_axel.fieldaxis().note();
  expect(ax_fieldaxis.registry(0).has("city") && !ax_fieldaxis.registry(0).has("score"),
         "Axel::fieldaxis().note should project per-registry note fields");

  const auto concat = axel.concat_depth(axel);
  expect(concat.depth() == 4, "Axel::concat_depth mismatch");
  expect(concat.labels().has_value(), "concat_depth should keep labels when both inputs labeled");
  expect(concat.labels()->size() == 4, "concat_depth labels size mismatch");

  const auto archive = axel.to_kindle();
  expect(archive.depth() == 2, "Axel::to_kindle archive depth mismatch");
  const auto back = Axel::from_kindle(archive);
  expect(back.depth() == 2, "Axel::from_kindle depth mismatch");

  expect(!axel.to_string().empty(), "Axel::to_string should not be empty");
  expect(!axel.to_json().empty(), "Axel::to_json should not be empty");

  expect_throw(
      [&]() {
        (void)Axel({r0, r1}, Slot({as_note("one_label")}));
      },
      "Axel constructor should enforce label/depth alignment");
}

} // namespace

int main() {
  ::setenv("PLUSS_ERROR_NATIVE_TRACE", "0", 0);
  try {
    test_slot_methods();
    test_registry_methods();
    test_bundle_lvalue_name_ranges();
    test_registry_loaders();
    test_axel_methods();
  } catch (const std::exception& e) {
    std::cerr << "[bundle_model_tests] failure: " << e.what() << "\n";
    return 1;
  }

  std::cout << "[bundle_model_tests] all checks passed\n";
  return 0;
}

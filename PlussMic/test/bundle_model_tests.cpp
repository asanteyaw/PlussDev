#include <cctype>
#include <cstdint>
#include <exception>
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

Slot::Value as_series_i64(std::int64_t v) {
  return pluss::make_series(v, pluss::codem::ScalarType::Int64);
}

Slot::Value as_series_f64(double v) {
  return pluss::make_series(v, pluss::codem::ScalarType::Float64);
}

Slot::Value as_series_bool(bool v) {
  return pluss::make_series(v, pluss::codem::ScalarType::Bool);
}

Slot::Value as_note(std::string_view v) {
  return pluss::content(v);
}

bool is_series(const Slot::Value& v) {
  return std::holds_alternative<pluss::Series>(v);
}

bool is_note(const Slot::Value& v) {
  return std::holds_alternative<pluss::Note>(v);
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

  const auto stream = ids.kindelize();
  expect(stream.size() == ids.size(), "Slot::kindelize size mismatch");
  expect(stream.kind() == pluss::kindle::StreamKind::Series, "Slot::kindelize kind mismatch");
  const auto roundtrip = Slot::from_kindle(stream);
  expect(roundtrip.size() == ids.size(), "Slot::from_kindle size mismatch");

  const Slot hetero({
      as_series_i64(1),
      as_note("x"),
  });
  expect_throw([&]() { (void)hetero.kindelize(); }, "heterogeneous slot kindelize should throw");
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
  const auto cat = clean.kindelize();
  expect(cat.nstreams() == 2, "Registry::kindelize stream count mismatch");
  const auto from_cat = Registry::from_kindle(cat);
  expect(from_cat.nslots() == 2, "Registry::from_kindle slot count mismatch");

  const auto mixed = Registry({
      {"ids", ids},
      {"mixed", Slot({as_series_i64(7), as_note("bad")})},
  });
  expect_throw([&]() { (void)mixed.kindelize(); }, "Registry::kindelize should throw on heterogeneous slot");
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

  const auto archive = aisle.kindelize();
  expect(archive.depth() == 2, "Aisle::kindelize archive depth mismatch");
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
    test_aisle_methods();
  } catch (const std::exception& e) {
    std::cerr << "[bundle_model_tests] failure: " << e.what() << "\n";
    return 1;
  }

  std::cout << "[bundle_model_tests] all checks passed\n";
  return 0;
}

#include <cctype>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
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

Slot make_ids_slot() {
  return Slot({
      std::int64_t{1},
      std::int64_t{2},
      std::int64_t{3},
      std::int64_t{4},
      std::int64_t{5},
  });
}

Slot make_city_slot() {
  return Slot({
      std::string("Accra"),
      std::string("Kumasi"),
      std::string("Tema"),
      std::string("Tamale"),
      std::string("Cape Coast"),
  });
}

void test_slot_methods() {
  auto ids = make_ids_slot();
  expect(ids.size() == 5, "ids slot size mismatch");

  ids.push_(std::int64_t{6});
  expect(ids.size() == 6, "push_ should append");

  const auto head = ids.head(3);
  const auto tail = ids.tail(2);
  const auto slice = ids.slice(1, 4);
  const auto take = ids.take(std::vector<std::int64_t>{0, 2, 4});
  const auto filtered = ids.filter([](const Slot::Value& v) {
    return std::holds_alternative<std::int64_t>(v) && std::get<std::int64_t>(v) >= 4;
  });
  const auto transformed = ids.transform([](const Slot::Value& v) -> Slot::Value {
    if (std::holds_alternative<std::int64_t>(v)) return std::get<std::int64_t>(v) * 2;
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
  expect(counts.at("int64") == 6, "type_counts[int64] mismatch");

  const auto stream = ids.signalize();
  expect(stream.size() == ids.size(), "Slot::signalize size mismatch");
  expect(stream.kind() == pluss::signal::StreamKind::Series, "Slot::signalize kind mismatch");
  const auto roundtrip = Slot::from_signal(stream);
  expect(roundtrip.size() == ids.size(), "Slot::from_signal size mismatch");

  const Slot hetero({
      std::int64_t{1},
      std::string("x"),
  });
  expect_throw([&]() { (void)hetero.signalize(); }, "heterogeneous slot signalize should throw");
}

void test_registry_methods() {
  const auto ids = make_ids_slot();
  const auto city = make_city_slot();
  const auto score = Slot({
      1.5,
      2.5,
      3.5,
      4.5,
      5.5,
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
      if (std::holds_alternative<std::int64_t>(v)) return std::get<std::int64_t>(v) + 10;
      return v;
    });
  });
  expect(std::get<std::int64_t>(applied.slot("ids").at(0)) == 11, "Registry::apply transform mismatch");

  reg.apply_("score", [](const Slot& s) {
    return s.transform([](const Slot::Value& v) -> Slot::Value {
      if (std::holds_alternative<double>(v)) return std::get<double>(v) * 2.0;
      return v;
    });
  });
  expect(std::get<double>(reg.slot("score").at(0)) == 3.0, "Registry::apply_ mutate mismatch");

  const auto upper = reg.apply_all([](const Slot& s) {
    return s.transform([](const Slot::Value& v) -> Slot::Value {
      if (std::holds_alternative<std::string>(v)) {
        auto out = std::get<std::string>(v);
        for (auto& ch : out) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        return out;
      }
      return v;
    });
  });
  expect(std::get<std::string>(upper.slot("city").at(0)) == "ACCRA", "Registry::apply_all mismatch");

  const auto sizes = reg.slot_sizes();
  expect(sizes.at("ids") == 5, "Registry::slot_sizes mismatch");

  const auto other = Registry({
      {"ids", Slot({std::int64_t{9}})},
      {"flag", Slot({true})},
  });
  const auto combined = reg.combine(other, "_rhs");
  expect(combined.nslots() == 5, "Registry::combine slot count mismatch");
  expect(combined.has("ids_rhs"), "Registry::combine should suffix colliding key");

  const auto with_new = reg.set_slot("status", Slot({std::string("ok"), std::string("ok")}));
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
  const auto cat = clean.signalize();
  expect(cat.nstreams() == 2, "Registry::signalize stream count mismatch");
  const auto from_cat = Registry::from_signal(cat);
  expect(from_cat.nslots() == 2, "Registry::from_signal slot count mismatch");

  const auto mixed = Registry({
      {"ids", ids},
      {"mixed", Slot({std::int64_t{7}, std::string("bad")})},
  });
  expect_throw([&]() { (void)mixed.signalize(); }, "Registry::signalize should throw on heterogeneous slot");
}

void test_aisle_methods() {
  const auto r0 = Registry({
      {"ids", Slot({std::int64_t{1}, std::int64_t{2}, std::int64_t{3}})},
      {"city", Slot({std::string("Accra"), std::string("Kumasi"), std::string("Tema")})},
  });
  const auto r1 = Registry({
      {"ids", Slot({std::int64_t{10}, std::int64_t{20}, std::int64_t{30}})},
      {"city", Slot({std::string("Accra"), std::string("Kumasi"), std::string("Tema")})},
  });
  const auto labels = Slot({std::string("train"), std::string("test")});
  const auto aisle = Aisle({r0, r1}, labels);

  expect(aisle.depth() == 2, "Aisle depth mismatch");
  expect(aisle.labels().has_value(), "Aisle labels should be present");
  expect(aisle.head(1).depth() == 1, "Aisle::head depth mismatch");
  expect(aisle.tail(1).depth() == 1, "Aisle::tail depth mismatch");
  expect(aisle.slice(0, 2).depth() == 2, "Aisle::slice depth mismatch");

  const auto mapped = aisle.map([](const Registry& reg) {
    return reg.set_slot("flag", Slot({true, false, true}));
  });
  expect(mapped.registry(0).has("flag"), "Aisle::map should transform registries");

  const auto selected = aisle.select({"ids"});
  expect(selected.registry(0).nslots() == 1, "Aisle::select should project keys");

  const auto applied = aisle.apply_slot("ids", [](const Slot& s) {
    return s.transform([](const Slot::Value& v) -> Slot::Value {
      if (std::holds_alternative<std::int64_t>(v)) return std::get<std::int64_t>(v) * 2;
      return v;
    });
  });
  expect(std::get<std::int64_t>(applied.registry(0).slot("ids").at(0)) == 2, "Aisle::apply_slot mismatch");

  const auto concat = aisle.concat_depth(aisle);
  expect(concat.depth() == 4, "Aisle::concat_depth mismatch");
  expect(concat.labels().has_value(), "concat_depth should keep labels when both inputs labeled");
  expect(concat.labels()->size() == 4, "concat_depth labels size mismatch");

  const auto archive = aisle.signalize();
  expect(archive.depth() == 2, "Aisle::signalize archive depth mismatch");
  const auto back = Aisle::from_signal(archive);
  expect(back.depth() == 2, "Aisle::from_signal depth mismatch");

  expect(!aisle.to_string().empty(), "Aisle::to_string should not be empty");
  expect(!aisle.to_json().empty(), "Aisle::to_json should not be empty");

  expect_throw(
      [&]() {
        (void)Aisle({r0, r1}, Slot({std::string("one_label")}));
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

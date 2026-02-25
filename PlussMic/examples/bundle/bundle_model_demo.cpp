#include <cctype>
#include <cstdint>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include "pluss/pluss.h"

namespace {

using pluss::bundle::Aisle;
using pluss::bundle::Registry;
using pluss::bundle::Slot;

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
  const auto& s = std::get<pluss::Series>(v);
  return s.to(pluss::codem::ScalarType::Int64).to_vec<std::int64_t>()[0];
}

double to_f64(const Slot::Value& v) {
  const auto& s = std::get<pluss::Series>(v);
  return s.to(pluss::codem::ScalarType::Float64).to_vec<double>()[0];
}

std::string to_string_value(const Slot::Value& v) {
  const auto& n = std::get<pluss::Note>(v);
  return std::string(n.at(0));
}

void demo_slot_methods() {
  std::cout << "\n=== Slot Methods ===\n";

  Slot ids({
      as_series_i64(1),
      as_series_i64(2),
      as_series_i64(3),
      as_series_i64(4),
      as_series_i64(5),
  });
  ids.push_(as_series_i64(6));

  const auto head = ids.head(3);
  const auto tail = ids.tail(2);
  const auto sliced = ids.slice(1, 5);
  const auto taken = ids.take(std::vector<std::int64_t>{0, 2, 4});
  const auto filtered = ids.filter([](const Slot::Value& v) {
    return is_series(v) && (to_i64(v) % 2 == 0);
  });
  const auto transformed = ids.transform([](const Slot::Value& v) -> Slot::Value {
    if (is_series(v)) return as_series_i64(to_i64(v) * 10);
    return v;
  });
  const auto concatenated = head.concat(tail);
  const auto type_counts = ids.type_counts();

  std::cout << "ids size: " << ids.size()
            << ", head size: " << head.size()
            << ", tail size: " << tail.size()
            << ", slice size: " << sliced.size()
            << ", take size: " << taken.size()
            << ", filter size: " << filtered.size()
            << ", transform size: " << transformed.size()
            << ", concat size: " << concatenated.size() << "\n";
  std::cout << "type_counts[Series]: " << type_counts.at("Series") << "\n";

  const auto stream = ids.to_kindle();
  const auto back_slot = Slot::from_kindle(stream);
  std::cout << "slot -> stream -> slot size: " << back_slot.size() << "\n";

  const Slot hetero({
      as_series_i64(7),
      as_note("mixed"),
  });
  try {
    (void)hetero.to_kindle();
  } catch (const std::exception& e) {
    std::cout << "expected heterogeneous to_kindle failure: " << e.what() << "\n";
  }

  std::cout << "slot json:\n" << ids.to_json() << "\n";
}

void demo_registry_methods() {
  std::cout << "\n=== Registry Methods ===\n";

  const Slot ids({
      as_series_i64(1),
      as_series_i64(2),
      as_series_i64(3),
      as_series_i64(4),
  });
  const Slot cities({
      as_note("Accra"),
      as_note("Kumasi"),
      as_note("Tema"),
      as_note("Tamale"),
  });
  const Slot scores({
      as_series_f64(10.5),
      as_series_f64(20.5),
      as_series_f64(30.5),
      as_series_f64(40.5),
  });

  auto registry = Registry({
      {"ids", ids},
      {"city", cities},
      {"score", scores},
  });

  const auto head = registry.head(2);
  const auto tail = registry.tail(2);
  const auto slice = registry.slice(1, 3);
  const auto select = registry.select({"ids", "city"});
  const auto drop = registry.drop({"score"});
  const auto rename = registry.rename({{"city", "location"}});
  const auto applied = registry.apply("ids", [](const Slot& s) {
    return s.transform([](const Slot::Value& v) -> Slot::Value {
      if (is_series(v)) return as_series_i64(to_i64(v) + 100);
      return v;
    });
  });
  registry.apply_("score", [](const Slot& s) {
    return s.transform([](const Slot::Value& v) -> Slot::Value {
      if (is_series(v)) return as_series_f64(to_f64(v) * 2.0);
      return v;
    });
  });
  const auto all_upper = registry.apply_all([](const Slot& s) {
    return s.transform([](const Slot::Value& v) -> Slot::Value {
      if (is_note(v)) {
        auto out = to_string_value(v);
        for (auto& ch : out) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        return as_note(out);
      }
      return v;
    });
  });

  const auto other = Registry({
      {"ids", Slot({as_series_i64(9), as_series_i64(8)})},
      {"flag", Slot({as_series_bool(true), as_series_bool(false)})},
  });
  const auto combined = registry.combine(other, "_rhs");
  const auto with_new_slot = registry.set_slot("status", Slot({as_note("ok"), as_note("ok")}));
  registry.set_slot_("city_copy", cities);

  std::cout << "registry nslots: " << registry.nslots()
            << ", head(ids) size: " << head.slot("ids").size()
            << ", tail(ids) size: " << tail.slot("ids").size()
            << ", slice(ids) size: " << slice.slot("ids").size()
            << ", select nslots: " << select.nslots()
            << ", drop nslots: " << drop.nslots()
            << ", rename has(location): " << rename.has("location")
            << ", applied ids[0] updated: " << to_i64(applied.slot("ids").at(0))
            << ", apply_all city[0]: " << to_string_value(all_upper.slot("city").at(0))
            << ", combined nslots: " << combined.nslots()
            << ", with_new_slot has(status): " << with_new_slot.has("status")
            << ", has(city_copy): " << registry.has("city_copy") << "\n";

  const auto sizes = registry.slot_sizes();
  for (const auto& [k, n] : sizes) {
    std::cout << "slot_sizes[" << k << "] = " << n << "\n";
  }

  const auto catalog = Registry({
                           {"ids", ids},
                           {"city", cities},
                       })
                           .to_kindle();
  const auto from_catalog = Registry::from_kindle(catalog);
  std::cout << "registry -> catalog -> registry nslots: " << from_catalog.nslots() << "\n";
  std::cout << "registry display:\n" << registry << "\n";
  std::cout << "registry json:\n" << registry.to_json() << "\n";
}

void demo_aisle_methods() {
  std::cout << "\n=== Aisle Methods ===\n";

  const auto r0 = Registry({
      {"ids", Slot({as_series_i64(1), as_series_i64(2), as_series_i64(3)})},
      {"city", Slot({as_note("Accra"), as_note("Kumasi"), as_note("Tema")})},
  });
  const auto r1 = Registry({
      {"ids", Slot({as_series_i64(10), as_series_i64(20), as_series_i64(30)})},
      {"city", Slot({as_note("Accra"), as_note("Kumasi"), as_note("Tema")})},
  });
  const auto labels = Slot({as_note("train"), as_note("test")});
  const Aisle aisle({r0, r1}, labels);

  const auto head = aisle.head(1);
  const auto tail = aisle.tail(1);
  const auto slice = aisle.slice(0, 2);
  const auto mapped = aisle.map([](const Registry& r) {
    return r.set_slot("source", Slot({as_note("demo"), as_note("demo"), as_note("demo")}));
  });
  const auto selected = aisle.select({"ids"});
  const auto applied_slot = aisle.apply_slot("ids", [](const Slot& s) {
    return s.transform([](const Slot::Value& v) -> Slot::Value {
      if (is_series(v)) return as_series_i64(to_i64(v) * 3);
      return v;
    });
  });
  const auto concatenated = aisle.concat_depth(aisle);

  std::cout << "aisle depth: " << aisle.depth()
            << ", head depth: " << head.depth()
            << ", tail depth: " << tail.depth()
            << ", slice depth: " << slice.depth()
            << ", mapped depth: " << mapped.depth()
            << ", selected depth: " << selected.depth()
            << ", applied depth: " << applied_slot.depth()
            << ", concat depth: " << concatenated.depth() << "\n";

  const auto archive = aisle.to_kindle();
  const auto back = Aisle::from_kindle(archive);
  std::cout << "aisle -> archive -> aisle depth: " << back.depth()
            << ", labels: " << (back.labels().has_value() ? "yes" : "no") << "\n";
  std::cout << "aisle display:\n" << aisle << "\n";
  std::cout << "aisle json:\n" << aisle.to_json() << "\n";
}

} // namespace

int main() {
  try {
    demo_slot_methods();
    demo_registry_methods();
    demo_aisle_methods();
  } catch (const std::exception& e) {
    std::cerr << "bundle_model_demo failed: " << e.what() << "\n";
    return 1;
  }
  return 0;
}

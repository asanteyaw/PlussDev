#include <array>
#include <cctype>
#include <exception>
#include <iostream>
#include <string>
#include <variant>
#include <vector>

#include "pluss/mic.h"

namespace {

void demo_slot_methods() {
  std::cout << "\n=== Slot Methods ===\n";

  pluss::bundle::Slot ids({
      std::int64_t{1},
      std::int64_t{2},
      std::int64_t{3},
      std::int64_t{4},
      std::int64_t{5},
  });
  ids.push_(std::int64_t{6});

  const auto head = ids.head(3);
  const auto tail = ids.tail(2);
  const auto sliced = ids.slice(1, 5);
  const auto taken = ids.take(std::vector<std::int64_t>{0, 2, 4});
  const auto filtered = ids.filter([](const pluss::bundle::Slot::Value& v) {
    return std::holds_alternative<std::int64_t>(v) && std::get<std::int64_t>(v) % 2 == 0;
  });
  const auto transformed = ids.transform([](const pluss::bundle::Slot::Value& v) -> pluss::bundle::Slot::Value {
    if (std::holds_alternative<std::int64_t>(v)) return std::get<std::int64_t>(v) * 10;
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
  std::cout << "type_counts[int64]: " << type_counts.at("int64") << "\n";

  const auto stream = ids.signalize();
  const auto back_slot = pluss::bundle::Slot::from_signal(stream);
  std::cout << "slot -> stream -> slot size: " << back_slot.size() << "\n";

  const pluss::bundle::Slot hetero({
      std::int64_t{7},
      std::string("mixed"),
      true,
  });
  try {
    (void)hetero.signalize();
  } catch (const std::exception& e) {
    std::cout << "expected heterogeneous signalize failure: " << e.what() << "\n";
  }

  std::cout << "slot json:\n" << ids.to_json() << "\n";
}

void demo_registry_methods() {
  std::cout << "\n=== Registry Methods ===\n";

  const pluss::bundle::Slot ids({
      std::int64_t{1},
      std::int64_t{2},
      std::int64_t{3},
      std::int64_t{4},
  });
  const pluss::bundle::Slot cities({
      std::string("Accra"),
      std::string("Kumasi"),
      std::string("Tema"),
      std::string("Tamale"),
  });
  const pluss::bundle::Slot scores({
      10.5,
      20.5,
      30.5,
      40.5,
  });

  auto registry = pluss::bundle::Registry({
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
  const auto applied = registry.apply("ids", [](const pluss::bundle::Slot& s) {
    return s.transform([](const pluss::bundle::Slot::Value& v) -> pluss::bundle::Slot::Value {
      if (std::holds_alternative<std::int64_t>(v)) return std::get<std::int64_t>(v) + 100;
      return v;
    });
  });
  registry.apply_("score", [](const pluss::bundle::Slot& s) {
    return s.transform([](const pluss::bundle::Slot::Value& v) -> pluss::bundle::Slot::Value {
      if (std::holds_alternative<double>(v)) return std::get<double>(v) * 2.0;
      return v;
    });
  });
  const auto all_upper = registry.apply_all([](const pluss::bundle::Slot& s) {
    return s.transform([](const pluss::bundle::Slot::Value& v) -> pluss::bundle::Slot::Value {
      if (std::holds_alternative<std::string>(v)) {
        auto out = std::get<std::string>(v);
        for (auto& ch : out) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        return out;
      }
      return v;
    });
  });

  const auto other = pluss::bundle::Registry({
      {"ids", pluss::bundle::Slot({std::int64_t{9}, std::int64_t{8}})},
      {"flag", pluss::bundle::Slot({true, false})},
  });
  const auto combined = registry.combine(other, "_rhs");
  const auto with_new_slot = registry.set_slot("status", pluss::bundle::Slot({std::string("ok"), std::string("ok")}));
  registry.set_slot_("city_copy", cities);

  std::cout << "registry nslots: " << registry.nslots()
            << ", head(ids) size: " << head.slot("ids").size()
            << ", tail(ids) size: " << tail.slot("ids").size()
            << ", slice(ids) size: " << slice.slot("ids").size()
            << ", select nslots: " << select.nslots()
            << ", drop nslots: " << drop.nslots()
            << ", rename has(location): " << rename.has("location")
            << ", applied ids[0] updated: " << std::get<std::int64_t>(applied.slot("ids").at(0))
            << ", apply_all city[0]: " << std::get<std::string>(all_upper.slot("city").at(0))
            << ", combined nslots: " << combined.nslots()
            << ", with_new_slot has(status): " << with_new_slot.has("status")
            << ", has(city_copy): " << registry.has("city_copy") << "\n";

  const auto sizes = registry.slot_sizes();
  for (const auto& [k, n] : sizes) {
    std::cout << "slot_sizes[" << k << "] = " << n << "\n";
  }

  const auto catalog = pluss::bundle::Registry({
                           {"ids", ids},
                           {"city", cities},
                       })
                           .signalize();
  const auto from_catalog = pluss::bundle::Registry::from_signal(catalog);
  std::cout << "registry -> catalog -> registry nslots: " << from_catalog.nslots() << "\n";
  std::cout << "registry json:\n" << registry.to_json() << "\n";
}

void demo_aisle_methods() {
  std::cout << "\n=== Aisle Methods ===\n";

  const auto r0 = pluss::bundle::Registry({
      {"ids", pluss::bundle::Slot({std::int64_t{1}, std::int64_t{2}, std::int64_t{3}})},
      {"city", pluss::bundle::Slot({std::string("Accra"), std::string("Kumasi"), std::string("Tema")})},
  });
  const auto r1 = pluss::bundle::Registry({
      {"ids", pluss::bundle::Slot({std::int64_t{10}, std::int64_t{20}, std::int64_t{30}})},
      {"city", pluss::bundle::Slot({std::string("Accra"), std::string("Kumasi"), std::string("Tema")})},
  });
  const auto labels = pluss::bundle::Slot({std::string("train"), std::string("test")});
  const pluss::bundle::Aisle aisle({r0, r1}, labels);

  const auto head = aisle.head(1);
  const auto tail = aisle.tail(1);
  const auto slice = aisle.slice(0, 2);
  const auto mapped = aisle.map([](const pluss::bundle::Registry& r) {
    return r.set_slot("source", pluss::bundle::Slot({std::string("demo"), std::string("demo"), std::string("demo")}));
  });
  const auto selected = aisle.select({"ids"});
  const auto applied_slot = aisle.apply_slot("ids", [](const pluss::bundle::Slot& s) {
    return s.transform([](const pluss::bundle::Slot::Value& v) -> pluss::bundle::Slot::Value {
      if (std::holds_alternative<std::int64_t>(v)) return std::get<std::int64_t>(v) * 3;
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

  const auto archive = aisle.signalize();
  const auto back = pluss::bundle::Aisle::from_signal(archive);
  std::cout << "aisle -> archive -> aisle depth: " << back.depth()
            << ", labels: " << (back.labels().has_value() ? "yes" : "no") << "\n";
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

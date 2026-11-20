// Layered configuration: defaults, a site override and a user override, merged in priority order, validated for required fields
// and with the values that came from each layer traced.
#include <pluss/pluss.h>

#include <iostream>
#include <map>
#include <set>
#include <string>

using pluss::scribble::Registry;
using pluss::scribble::Slot;
using Value = Slot::Value;

static Slot text(const char *v) { return Slot(Value(pluss::make_note({v}))); }
static Slot number(double v) { return Slot(Value(pluss::make_series({v}))); }

int main() {
  Registry defaults({{"timeout", number(30)}, {"retries", number(3)}, {"region", text("eu")}, {"log", text("info")}});
  Registry site({{"timeout", number(60)}, {"region", text("us-east")}});
  Registry user({{"log", text("debug")}, {"colour", text("on")}});

  // 1. combine keeps both versions of a clashing field (the second under a suffix); it does not override.
  auto both = defaults.combine(site, "_site");
  std::cout << "combine keeps both:";
  for (const auto &name : both.fields()) std::cout << " " << name;
  std::cout << "\n";

  // 2. Layering is overriding: set each field of a higher layer over the layer below, lowest priority first.
  const auto overlay = [](Registry base, const Registry &top) {
    for (const auto &name : top.fields()) base = base.set(name, *top.get(name));
    return base;
  };
  auto merged = overlay(overlay(defaults, site), user);
  std::map<std::string, std::string> source;
  for (const auto &name : defaults.fields()) source[name] = "defaults";
  for (const auto &name : site.fields()) source[name] = "site";
  for (const auto &name : user.fields()) source[name] = "user";
  const auto value_of = [&](const Registry &r, const std::string &key) -> std::string {
    const auto cell = r.get(key)->values().front();
    if (std::holds_alternative<pluss::Note>(cell)) return std::string(std::get<pluss::Note>(cell).at(0));
    return std::to_string(static_cast<int>(std::get<pluss::Series>(cell).items<double>()[0]));
  };
  for (const auto &name : std::set<std::string>(merged.fields().begin(), merged.fields().end()))
    std::cout << "  " << name << " = " << value_of(merged, name) << "   (from " << source.at(name) << ")\n";

  // 3. Validate: required settings must be present.
  std::cout << "has every required field (timeout, retries, region): " << (merged.has_all({"timeout", "retries", "region"}) ? "yes" : "no") << "\n";
  std::cout << "has a proxy setting: " << (merged.has("proxy") ? "yes" : "no") << "\n";
  auto incomplete = overlay(site, user);
  std::cout << "site + user alone has every required field: " << (incomplete.has_all({"timeout", "retries", "region"}) ? "yes" : "no") << "\n";
}

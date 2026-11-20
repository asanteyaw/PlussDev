// Ingest JSON whose fields are not always the same type: see which fields are mixed, find the offending records, and turn the clean part
// into a typed table.
#include <pluss/pluss.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>

using pluss::scribble::Registry;

int main() {
  const auto path = std::filesystem::temp_directory_path() / "pluss_guide_events.json";
  {
    std::ofstream file(path);
    file << R"([{"id":1,"value":12.5,"tag":"a"},{"id":2,"value":"n/a","tag":"b"},{"id":3,"value":7,"tag":null},{"id":4,"value":true,"tag":"c"},{"id":5,"value":9.25,"tag":"a"}])";
  }
  auto events = Registry::load_json(path.string());
  std::filesystem::remove(path);
  std::cout << "fields " << events.nfields() << ", records " << events.sizes().at("id") << "\n";

  // 1. A profile counts the value types in every field. A field with more than one type is the one that needs attention.
  const auto profiles = events.profiles();
  const std::map<std::string, std::unordered_map<std::string, std::int64_t>> sorted(profiles.begin(), profiles.end());
  for (const auto &[field, counts] : sorted) {
    std::map<std::string, std::int64_t> ordered(counts.begin(), counts.end());
    std::cout << field << ":";
    for (const auto &[type, n] : ordered) std::cout << " " << type << "=" << n;
    std::cout << (ordered.size() > 1 ? "   <- mixed" : "") << "\n";
  }

  // 2. Look at the mixed field: its cells are float, null, integer, boolean, float. The string "n/a" did not survive as a number and is null.
  std::cout << events.get("value")->to_string() << "\n";

  // 3. Make the field one type: widen every cell to Float64 (an integer becomes a float, a boolean 0 or 1), then decide what a null means.
  auto widened = events.get("value")->astype(pluss::ScalarType::Float64);
  auto complete = events.set("value", widened.fillnull(0.0));
  std::cout << "profile of value after cleaning:";
  for (const auto &[type, n] : std::map<std::string, std::int64_t>(complete.profiles().at("value").begin(), complete.profiles().at("value").end())) std::cout << " " << type << "=" << n;
  std::cout << "\n";

  // 4. Cross to the typed world: a Registry converts to a Catalog and from there to a DataFrame. A field that is still mixed would be refused.
  try {
    (void)events.kindelize();
    std::cout << "the raw registry converted\n";
  } catch (const pluss::Error &e) {
    std::cout << "raw registry refused: " << e.message() << "\n";
  }
  auto table = complete.drop("tag").kindelize().tabelize();
  std::cout << table.to_string() << "\n";
  std::cout << "mean value " << table.features("value").mean().to_dbl() << "\n";
}

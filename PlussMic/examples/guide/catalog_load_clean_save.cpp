// A file round trip in the Catalog model: load a CSV into streams, drop the rows with no reading, fill the counts, widen a stream's dtype,
// summarise per sensor and save the clean version.
#include <pluss/pluss.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>

using pluss::kindle::Catalog;

int main() {
  const auto dir = std::filesystem::temp_directory_path();
  const auto raw_path = dir / "pluss_guide_catalog_raw.csv";
  const auto clean_path = dir / "pluss_guide_catalog_clean.csv";
  {
    std::ofstream file(raw_path);
    file << "sensor,reading,count\nA,12.5,3\nB,,4\nA,13.1,\nB,14.0,2\nB,15.2,5\nA,11.9,1\n";
  }
  auto raw = Catalog::load_csv(raw_path.string());
  std::cout << "loaded " << raw.nrows() << " rows, " << raw.nkeys() << " streams\n" << raw << "\n";

  // 1. A row without a reading is useless: dropnull removes the rows where any stream is missing, so first fill what may be missing.
  auto counted = raw.set("count", raw.keys("count").fillnull(0.0));
  auto kept = counted.dropnull();
  std::cout << "rows kept after filling counts and dropping missing readings: " << kept.nrows() << " of " << raw.nrows() << "\n";

  // 2. Counts are whole numbers in the file but averages need a floating dtype: widen the stream.
  auto prepared = kept.set("count", kept.keys("count").astype(pluss::ScalarType::Float64));
  std::cout << "count dtype is now " << prepared.keys("count").as_series().scalar_type() << "\n";

  // 3. One line per sensor.
  std::map<std::string, Catalog> by_sensor;
  for (const auto &[name, part] : prepared.group().by("sensor")) by_sensor.emplace(name, part);
  for (const auto &[name, part] : by_sensor)
    std::cout << "sensor " << name << ": " << part.nrows() << " readings, mean " << part.keys("reading").mean().as_series().items<double>()[0] << ", total count "
              << part.keys("count").sum().as_series().items<double>()[0] << "\n";

  // 4. Save and read back; the totals must survive.
  prepared.save_csv(clean_path.string());
  auto again = Catalog::load_csv(clean_path.string());
  std::cout << "reloaded " << again.nrows() << " rows; reading total " << again.keys("reading").sum().as_series().items<double>()[0] << " vs "
            << prepared.keys("reading").sum().as_series().items<double>()[0] << "\n";
  std::filesystem::remove(raw_path);
  std::filesystem::remove(clean_path);
}

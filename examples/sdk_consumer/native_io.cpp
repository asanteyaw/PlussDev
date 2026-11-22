#include <pluss/mic.h>
#include <pluss/pluss.h>

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

using pluss::table::DataFrame;

namespace {

bool round_trip(const char* name, const DataFrame& source, const std::filesystem::path& path,
                void (*save)(const DataFrame&, const std::string&),
                DataFrame (*load)(const std::string&)) {
  try {
    save(source, path.string());
    const DataFrame back = load(path.string());
    if (back.shape() != source.shape() || !back.equals(source)) {
      std::cerr << name << ": round trip changed the data\n" << back << '\n';
      return false;
    }
    std::cout << name << ": ok " << back.shape().first << "x" << back.shape().second << '\n';
    return true;
  } catch (const std::exception& e) {
    std::cerr << name << ": failed: " << e.what() << '\n';
    return false;
  }
}

}  // namespace

int main() {
  const auto dir = std::filesystem::temp_directory_path() / "pluss_sdk_native_io";
  std::filesystem::create_directories(dir);

  const auto csv = dir / "source.csv";
  {
    std::ofstream out(csv);
    out << "id,price,label\n1,2.5,alpha\n2,3.75,beta\n3,-1.25,gamma\n";
  }
  const DataFrame source = DataFrame::load_csv(csv.string());

  const bool excel = round_trip(
      "excel", source, dir / "frame.xlsx",
      [](const DataFrame& f, const std::string& p) { f.save_excel(p); },
      [](const std::string& p) { return DataFrame::load_excel(p); });
  const bool parquet = round_trip(
      "parquet", source, dir / "frame.parquet",
      [](const DataFrame& f, const std::string& p) { f.save_parquet(p); },
      [](const std::string& p) { return DataFrame::load_parquet(p); });

  std::filesystem::remove_all(dir);
  return excel && parquet ? 0 : 1;
}

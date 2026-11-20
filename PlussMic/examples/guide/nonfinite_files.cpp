// Files: the tokens a CSV reader turns into missing cells, what it does with inf, and what a NaN becomes when it is written.
#include <pluss/pluss.h>

#include <filesystem>
#include <fstream>
#include <iostream>

using pluss::table::DataFrame;
using pluss::table::Shelf;

int main() {
  const auto path = std::filesystem::temp_directory_path() / "pluss_guide_nonfinite.csv";
  {
    std::ofstream file(path);
    // A blank cell, NA, null and nan are the usual ways a spreadsheet or a database export says "no value"; inf and -inf are real numbers.
    file << "id,reading\n1,1.5\n2,\n3,NA\n4,null\n5,nan\n6,inf\n7,-inf\n8,2.5\n";
  }
  auto loaded = DataFrame::load_csv(path.string());
  std::cout << loaded << "\n";
  std::cout << loaded.count_kinds() << "\n";

  // Writing: a missing cell is written as a blank, a NaN VALUE is written as the word NaN, infinities as inf and -inf.
  DataFrame computed({{"ratio", Shelf(pluss::make_series({0.5, std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity()}),
                                      pluss::make_series(std::vector<bool>{true, true, true}))},
                      {"gap", Shelf(pluss::make_series({1.0, 0.0, 3.0}), pluss::make_series(std::vector<bool>{true, false, true}))}});
  computed.save_csv(path.string());
  std::ifstream written(path);
  std::cout << written.rdbuf() << "\n";
  std::filesystem::remove(path);
}

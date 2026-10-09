// Labelling each slice by its position. When the function takes a second argument, map_axis passes the
// position of the slice, so the output can say which row it describes.
#include <pluss/mic.h>
#include <pluss/tils/aux/translate.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const auto readings = pluss::make_series(std::vector<double>{20.5, 21.0, 19.5, 30.0, 31.5, 29.0}, {3, 2}, cfg);

  const auto averages = pluss::tils::map_axis(
      [](const pluss::Series& slice, std::size_t position) {
        const auto values = slice.to_vec<double>();
        const double mean = (values[0] + values[1]) / 2.0;
        std::ostringstream line;
        line << "sensor " << position + 1 << " average " << mean;
        return line.str();
      },
      readings, 1);

  for (const auto& line : averages) std::cout << line << '\n';
  return 0;
}

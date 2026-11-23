#include <pluss/pluss.h>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

int failures = 0;

void require(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "example table test failed: " << message << '\n';
    ++failures;
  }
}

std::vector<double> column(const pluss::table::DataFrame& frame, const std::string& name) {
  return frame.features(name).as_series().contiguous().host<double>();
}

double mean(const std::vector<double>& v) {
  double s = 0;
  for (const double x : v) s += x;
  return s / static_cast<double>(v.size());
}

double correlation(const std::vector<double>& a, const std::vector<double>& b) {
  const double ma = mean(a), mb = mean(b);
  double ab = 0, aa = 0, bb = 0;
  for (std::size_t i = 0; i < a.size(); ++i) {
    ab += (a[i] - ma) * (b[i] - mb);
    aa += (a[i] - ma) * (a[i] - ma);
    bb += (b[i] - mb) * (b[i] - mb);
  }
  return ab / std::sqrt(aa * bb);
}

std::string text_of(const pluss::table::DataFrame& frame) {
  std::ostringstream out;
  out << frame;
  return out.str();
}

}  // namespace

int main() {
  using namespace pluss::table;

  const auto list = examples();
  require(list.size() == 5, "five example tables");
  for (const auto& item : list) {
    require(!item.about.empty() && item.cases > 0 && item.features > 0, item.name + " has a description and a size");
    const auto frame = example(item.name);
    require(frame.ncases() == item.cases && frame.nfeatures() == item.features, item.name + " matches the size examples() reports");
    require(text_of(frame) == text_of(example(item.name)), item.name + " is the same every time it is built");
  }

  // Anscombe's quartet: the four pairs share their statistics.
  const auto q = example("anscombe");
  require(q.ncases() == 11, "the quartet has eleven cases");
  const auto x = column(q, "x"), x4 = column(q, "x4");
  require(std::fabs(mean(x) - 9.0) < 1e-12 && std::fabs(mean(x4) - 9.0) < 1e-12, "the mean of x is 9 in every pair");
  for (const char* y : {"y1", "y2", "y3"}) {
    const auto v = column(q, y);
    require(std::fabs(mean(v) - 7.5) < 0.01, std::string("the mean of ") + y + " is about 7.5");
    require(std::fabs(correlation(x, v) - 0.816) < 0.001, std::string("the correlation of x and ") + y + " is about 0.816");
  }
  require(std::fabs(correlation(x4, column(q, "y4")) - 0.817) < 0.001, "the fourth pair has the same correlation");

  // The accounts table holds all four kinds of cell.
  const auto accounts = text_of(example("accounts"));
  require(accounts.find("NULL") != std::string::npos && accounts.find("NaN") != std::string::npos &&
              accounts.find("inf") != std::string::npos && accounts.find("-inf") != std::string::npos,
          "accounts shows NULL, NaN, inf and -inf");

  // Sensors: a Timeline column and a gap in one column.
  const auto sensors = example("sensors");
  require(sensors.ncases() == 48 && sensors.dropnull().ncases() == 42, "six hours of humidity are missing");

  // Sales: four cities, six months, and revenue follows units.
  const auto sales = example("sales");
  require(sales.ncases() == 24, "four cities over six months");
  require(correlation(column(sales, "units"), column(sales, "revenue")) > 0.99, "revenue follows units");

  // A wrong name says which names exist.
  try {
    (void)example("salez");
    require(false, "an unknown name is an error");
  } catch (const std::exception& error) {
    const std::string text = error.what();
    require(text.find("salez") != std::string::npos && text.find("sales") != std::string::npos, "the error names the mistake and the choices");
  }

  if (failures == 0) std::cout << "example table tests passed\n";
  return failures == 0 ? 0 : 1;
}

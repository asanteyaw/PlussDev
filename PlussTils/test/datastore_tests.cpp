#include <pluss/pluss.h>
#include <pluss/tils/kit/datastore.h>

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
    std::cerr << "datastore test failed: " << message << '\n';
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
  using pluss::loader::datastore;
  using pluss::loader::datastores;

  // The listing: every dataset has a description, a type, a shape and a size, and builds the same every time.
  const auto list = datastores();
  require(list.size() == 26, "twenty-six datasets");
  int per_type[6] = {0, 0, 0, 0, 0, 0};
  int ragged = 0;
  for (const auto& item : list) {
    require(!item.about.empty() && item.cases > 0 && item.features > 0, item.name + " has a description and a size");
    require(item.shape == "1D" || item.shape == "2D" || item.shape == "ragged 2D", item.name + " has a shape");
    const auto store = datastore(item.name);
    require(store.type() == item.type, item.name + " holds the type the listing reports");
    const std::string names[] = {"Shelf", "DataFrame", "Stream", "Catalog", "Slot", "Registry"};
    for (int k = 0; k < 6; ++k) per_type[k] += item.type == names[k];
    ragged += item.shape == "ragged 2D";
    require(item.shape != "1D" || item.features == 1, item.name + " is one column");
    require(item.name != "" && datastore(item.name).type() == store.type(), item.name + " can be asked for twice");
  }
  for (int k = 0; k < 6; ++k) require(per_type[k] > 0, "every container type has datasets");
  require(ragged == 5, "five ragged datasets (three catalogs, two registries)");

  // A table: all the table examples are the same as ever.
  const pluss::table::DataFrame q = datastore("anscombe");
  require(q.ncases() == 11, "the quartet has eleven cases");
  const auto x = column(q, "x"), x4 = column(q, "x4");
  require(std::fabs(mean(x) - 9.0) < 1e-12 && std::fabs(mean(x4) - 9.0) < 1e-12, "the mean of x is 9 in every pair");
  for (const char* y : {"y1", "y2", "y3"}) {
    const auto v = column(q, y);
    require(std::fabs(mean(v) - 7.5) < 0.01, std::string("the mean of ") + y + " is about 7.5");
    require(std::fabs(correlation(x, v) - 0.816) < 0.001, std::string("the correlation of x and ") + y + " is about 0.816");
  }
  require(std::fabs(correlation(x4, column(q, "y4")) - 0.817) < 0.001, "the fourth pair has the same correlation");
  const auto accounts = text_of(datastore("accounts").frame());
  require(accounts.find("NULL") != std::string::npos && accounts.find("NaN") != std::string::npos &&
              accounts.find("inf") != std::string::npos && accounts.find("-inf") != std::string::npos,
          "accounts shows NULL, NaN, inf and -inf");
  const auto sensors = datastore("sensors").frame();
  require(sensors.ncases() == 48 && sensors.dropnull().ncases() == 42, "six hours of humidity are missing");
  const auto sales = datastore("sales").frame();
  require(sales.ncases() == 24 && correlation(column(sales, "units"), column(sales, "revenue")) > 0.99, "sales: revenue follows units");

  // 1D: a column, a stream and a slot.
  require(datastore("temperature").shelf().size() == 48, "two days of hourly temperatures");
  require(datastore("pulse").stream().size() == 12, "twelve heart-rate readings");
  require(datastore("scores").slot().size() == 6, "six scores");
  require(datastore("mixed").slot().kind() == pluss::scribble::SlotKind::Mixed, "the mixed slot holds several kinds");

  // 2D catalogs and registries: the streams (slots) have the same length.
  const auto weather = datastore("weather").catalog();
  require(weather.shape() == std::make_pair<std::int64_t, std::int64_t>(24, 4), "weather: four streams of 24");
  const auto patients = datastore("patients").registry();
  require(patients.nfields() == 4 && patients.nstate() == 5, "patients: four fields of five");

  // Ragged: the streams (slots) differ in length, and the shortest is not the longest.
  const auto rain = datastore("rainfall").catalog();
  require(rain.shape() == std::make_pair<std::int64_t, std::int64_t>(12, 4), "rainfall: four stations, the longest with 12 months");
  require(rain.get("Tamale")->size() == 9 && rain.get("Wa")->size() == 7, "rainfall: stations have different lengths");
  const auto trials = datastore("trials").registry();
  require(trials.minstate() == 2 && trials.maxstate() == 5, "trials: from 2 to 5 trials");

  // Asking for the wrong type, or for a name that is not there, says what exists.
  try {
    (void)datastore("sales").catalog();
    require(false, "the wrong type is an error");
  } catch (const pluss::Error& error) {
    const std::string text = error.message();
    require(text.find("sales") != std::string::npos && text.find("DataFrame") != std::string::npos, "the error names the dataset and the type it holds");
  }
  try {
    (void)datastore("salez");
    require(false, "an unknown name is an error");
  } catch (const std::exception& error) {
    const std::string text = error.what();
    require(text.find("salez") != std::string::npos && text.find("sales") != std::string::npos, "the error names the mistake and the choices");
  }

  if (failures == 0) std::cout << "datastore tests passed\n";
  return failures == 0 ? 0 : 1;
}

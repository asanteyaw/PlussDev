#pragma once

// Example data for a first program, a guide or a bug report. Every table is built in memory from
// numbers written in this file or from a fixed formula, so it is the same on every machine and
// needs no file, no network and no licence: the numbers are either from a published table of
// facts or invented here.
//
//   for (const auto& item : pluss::table::examples()) std::cout << item.name << ": " << item.about << '\n';
//   auto sales = pluss::table::example("sales");

#include "table/DataFrame.h"
#include "table/Shelf.h"

#include <pluss/exception.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace pluss::table {

struct ExampleInfo {
  std::string name;
  std::string about;
  std::int64_t cases{0};
  std::int64_t features{0};
};

namespace example_detail {

// The i-th value of a fixed stream, in [0, 1): splitmix64 on a counter, so no random engine whose
// output could differ between standard libraries.
[[nodiscard]] inline double unit_(std::uint64_t seed, std::uint64_t i) noexcept {
  std::uint64_t z = seed + 0x9e3779b97f4a7c15ULL * (i + 1);
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
  z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
  z ^= z >> 31;
  return static_cast<double>(z >> 11) / 9007199254740992.0;
}

// Sales of four cities over six months: units follow a city's own level and a rising trend, and
// revenue is units times the city's price. A few returns make a third numeric column.
[[nodiscard]] inline DataFrame sales_() {
  const std::vector<std::string> cities{"Accra", "Tema", "Kumasi", "Takoradi"};
  const std::vector<double> level{120.0, 95.0, 140.0, 80.0};
  const std::vector<double> price{0.24, 0.22, 0.235, 0.22};
  std::vector<std::string> city, month;
  std::vector<double> units, revenue, returns;
  std::uint64_t i = 0;
  for (int m = 0; m < 6; ++m) {
    for (std::size_t c = 0; c < cities.size(); ++c, ++i) {
      const double u = std::round(level[c] * (1.0 + 0.04 * m) * (0.9 + 0.2 * unit_(11, i)));
      city.push_back(cities[c]);
      month.push_back("2024-0" + std::to_string(m + 1));
      units.push_back(u);
      revenue.push_back(std::round(u * price[c] * 100.0) / 100.0);
      returns.push_back(std::round(u * 0.03 * unit_(29, i)));
    }
  }
  return DataFrame({{"city", Shelf(pluss::make_note(city))},
                    {"month", Shelf(pluss::make_note(month))},
                    {"units", Shelf(pluss::make_series(units))},
                    {"revenue", Shelf(pluss::make_series(revenue))},
                    {"returns", Shelf(pluss::make_series(returns))}});
}

// Seven accounts with all four kinds of cell: ordinary numbers, a missing balance (NULL), and the
// divisions by zero that give NaN, inf and -inf.
[[nodiscard]] inline DataFrame accounts_() {
  const auto balance = Shelf(pluss::make_series({1200.0, 0.0, 450.0, 0.0, 0.0, 80.0, 0.0}),
                             pluss::make_series(std::vector<bool>{true, true, true, true, false, true, true}));
  const auto spend = Shelf(pluss::make_series({300.0, 0.0, 90.0, 120.0, 60.0, 40.0, -75.0}));
  return DataFrame({{"balance", balance}, {"spend", spend}, {"ratio", spend / balance}});
}

// Two days of hourly readings from one sensor: a daily swing in temperature, humidity that moves
// the other way, and a six-hour gap in the humidity (missing, NULL).
[[nodiscard]] inline DataFrame sensors_() {
  std::vector<std::string> stamp;
  std::vector<double> temperature, humidity;
  std::vector<bool> present;
  for (int h = 0; h < 48; ++h) {
    const int day = 1 + h / 24, hour = h % 24;
    stamp.push_back(std::string("2024-03-0") + std::to_string(day) + "T" + (hour < 10 ? "0" : "") + std::to_string(hour) + ":00:00Z");
    const double swing = std::sin((hour - 9) * 3.14159265358979323846 / 12.0);
    temperature.push_back(std::round((14.0 + 6.0 * swing + 0.8 * (unit_(41, static_cast<std::uint64_t>(h)) - 0.5)) * 10.0) / 10.0);
    humidity.push_back(std::round((62.0 - 15.0 * swing + 3.0 * (unit_(43, static_cast<std::uint64_t>(h)) - 0.5)) * 10.0) / 10.0);
    present.push_back(!(h >= 30 && h < 36));
  }
  return DataFrame({{"time", Shelf(pluss::make_timeline(stamp))},
                    {"temperature", Shelf(pluss::make_series(temperature))},
                    {"humidity", Shelf(pluss::make_series(humidity), pluss::make_series(present))}});
}

// Anscombe's quartet (F. J. Anscombe, "Graphs in Statistical Analysis", 1973): four pairs of
// columns with the same mean, variance, correlation and fitted line, and four different shapes.
// x2 and x3 equal x1, so only three x columns are stored.
[[nodiscard]] inline DataFrame anscombe_() {
  const std::vector<double> x{10, 8, 13, 9, 11, 14, 6, 4, 12, 7, 5};
  const std::vector<double> y1{8.04, 6.95, 7.58, 8.81, 8.33, 9.96, 7.24, 4.26, 10.84, 4.82, 5.68};
  const std::vector<double> y2{9.14, 8.14, 8.74, 8.77, 9.26, 8.10, 6.13, 3.10, 9.13, 7.26, 4.74};
  const std::vector<double> y3{7.46, 6.77, 12.74, 7.11, 7.81, 8.84, 6.08, 5.39, 8.15, 6.42, 5.73};
  const std::vector<double> x4{8, 8, 8, 8, 8, 8, 8, 19, 8, 8, 8};
  const std::vector<double> y4{6.58, 5.76, 7.71, 8.84, 8.47, 7.04, 5.25, 12.50, 5.56, 7.91, 6.89};
  return DataFrame({{"x", Shelf(pluss::make_series(x))},
                    {"y1", Shelf(pluss::make_series(y1))},
                    {"y2", Shelf(pluss::make_series(y2))},
                    {"y3", Shelf(pluss::make_series(y3))},
                    {"x4", Shelf(pluss::make_series(x4))},
                    {"y4", Shelf(pluss::make_series(y4))}});
}

// Twelve short messages written for this library, half about orders and half about delivery
// problems, with the label a text model would learn.
[[nodiscard]] inline DataFrame messages_() {
  const std::vector<std::string> text{
      "Please send two more boxes of the blue notebooks to the Accra office.",
      "Can I change my order to arrive on Friday instead of Thursday?",
      "I would like to add a second phone case to the order placed this morning.",
      "Do you have the green desk lamp in stock for delivery this week?",
      "Order 4471 should ship to the Tema warehouse, not the Kumasi one.",
      "Please confirm the price of twenty chairs before I place the order.",
      "My parcel has not arrived and the tracking page has not changed for five days.",
      "The box was damaged when it came and two of the cups are broken.",
      "I was charged twice for the same delivery and need one payment returned.",
      "The courier left the package outside in the rain and it is soaked.",
      "Delivery was promised for Monday but nobody came all day.",
      "The wrong item arrived: I ordered a lamp and received a kettle."};
  const std::vector<std::string> label{"order", "order", "order", "order", "order", "order",
                                       "problem", "problem", "problem", "problem", "problem", "problem"};
  std::vector<std::int64_t> id;
  for (std::int64_t i = 1; i <= static_cast<std::int64_t>(text.size()); ++i) id.push_back(i);
  return DataFrame({{"id", Shelf(pluss::make_series(id))},
                    {"text", Shelf(pluss::make_note(text))},
                    {"label", Shelf(pluss::make_note(label))}});
}

struct Entry {
  const char* name;
  const char* about;
  DataFrame (*build)();
};

inline const std::vector<Entry>& entries_() {
  static const std::vector<Entry> all{
      {"sales", "Units, revenue and returns for four cities over six months (invented)", &sales_},
      {"accounts", "Seven accounts with a missing balance, NaN, inf and -inf (invented)", &accounts_},
      {"sensors", "Two days of hourly readings with a gap in the humidity (invented)", &sensors_},
      {"anscombe", "Anscombe's quartet: four datasets with the same statistics and different shapes", &anscombe_},
      {"messages", "Twelve short order and delivery-problem messages with a label (invented)", &messages_},
  };
  return all;
}

}  // namespace example_detail

// The names of the example tables, with what each one is and its size.
[[nodiscard]] inline std::vector<ExampleInfo> examples() {
  std::vector<ExampleInfo> out;
  for (const auto& entry : example_detail::entries_()) {
    const DataFrame frame = entry.build();
    out.push_back(ExampleInfo{entry.name, entry.about, static_cast<std::int64_t>(frame.ncases()), static_cast<std::int64_t>(frame.nfeatures())});
  }
  return out;
}

// One example table by name. An unknown name is an error that lists the names that exist.
[[nodiscard]] inline DataFrame example(std::string_view name) {
  std::vector<std::string> known;
  for (const auto& entry : example_detail::entries_()) {
    if (name == entry.name) return entry.build();
    known.emplace_back(entry.name);
  }
  PLUSS_FAIL("pluss::table::example: no example table called '" + std::string(name) + "'", std::string(name), known);
}

}  // namespace pluss::table

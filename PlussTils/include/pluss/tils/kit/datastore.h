#pragma once

// Ready-made data for a first program, a guide or a bug report, in every shape the PlussMic containers take.
// Each one is built in memory from numbers written in this file or a fixed formula, so it is the same on every
// machine and needs no file, no network and no licence. The numbers are either from a published table of facts or
// invented here (the listing says which).
//
//   for (const auto& item : pluss::loader::datastores()) std::cout << item.name << " (" << item.type << "): " << item.about << '\n';
//   pluss::table::DataFrame sales = pluss::loader::datastore("sales");     // a table
//   auto rain = pluss::loader::datastore("rainfall").catalog();            // a catalog whose streams differ in length
//
// A dataset has one fixed type, so datastore(name) returns a Datastore that holds it: ask for the type you expect
// with frame(), shelf(), stream(), catalog(), slot() or registry() (or just assign it to a variable of that type).
// Asking for the wrong type is an error that says which one it holds.

#include <pluss/kindle/kindle.h>
#include <pluss/scribble/scribble.h>
#include <pluss/table/table.h>
#include <pluss/exception.h>

#include <cmath>
#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace pluss::loader {

// What a dataset is: its name, a line about it, its type, the shape of its data and its size.
struct DatastoreInfo {
  std::string name;
  std::string about;
  std::string type;   // Shelf, DataFrame, Stream, Catalog, Slot or Registry
  std::string shape;  // "1D", "2D" or "ragged 2D"
  std::int64_t cases{0};     // the length of the longest column
  std::int64_t features{0};  // the number of columns (1 for a single column)
};

namespace datastore_detail {

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
[[nodiscard]] inline table::DataFrame sales_() {
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
  return table::DataFrame({{"city", table::Shelf(pluss::make_note(city))},
                    {"month", table::Shelf(pluss::make_note(month))},
                    {"units", table::Shelf(pluss::make_series(units))},
                    {"revenue", table::Shelf(pluss::make_series(revenue))},
                    {"returns", table::Shelf(pluss::make_series(returns))}});
}

// Seven accounts with all four kinds of cell: ordinary numbers, a missing balance (NULL), and the
// divisions by zero that give NaN, inf and -inf.
[[nodiscard]] inline table::DataFrame accounts_() {
  const auto balance = table::Shelf(pluss::make_series({1200.0, 0.0, 450.0, 0.0, 0.0, 80.0, 0.0}),
                             pluss::make_series(std::vector<bool>{true, true, true, true, false, true, true}));
  const auto spend = table::Shelf(pluss::make_series({300.0, 0.0, 90.0, 120.0, 60.0, 40.0, -75.0}));
  return table::DataFrame({{"balance", balance}, {"spend", spend}, {"ratio", spend / balance}});
}

// Two days of hourly readings from one sensor: a daily swing in temperature, humidity that moves
// the other way, and a six-hour gap in the humidity (missing, NULL).
[[nodiscard]] inline table::DataFrame sensors_() {
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
  return table::DataFrame({{"time", table::Shelf(pluss::make_timeline(stamp))},
                    {"temperature", table::Shelf(pluss::make_series(temperature))},
                    {"humidity", table::Shelf(pluss::make_series(humidity), pluss::make_series(present))}});
}

// Anscombe's quartet (F. J. Anscombe, "Graphs in Statistical Analysis", 1973): four pairs of
// columns with the same mean, variance, correlation and fitted line, and four different shapes.
// x2 and x3 equal x1, so only three x columns are stored.
[[nodiscard]] inline table::DataFrame anscombe_() {
  const std::vector<double> x{10, 8, 13, 9, 11, 14, 6, 4, 12, 7, 5};
  const std::vector<double> y1{8.04, 6.95, 7.58, 8.81, 8.33, 9.96, 7.24, 4.26, 10.84, 4.82, 5.68};
  const std::vector<double> y2{9.14, 8.14, 8.74, 8.77, 9.26, 8.10, 6.13, 3.10, 9.13, 7.26, 4.74};
  const std::vector<double> y3{7.46, 6.77, 12.74, 7.11, 7.81, 8.84, 6.08, 5.39, 8.15, 6.42, 5.73};
  const std::vector<double> x4{8, 8, 8, 8, 8, 8, 8, 19, 8, 8, 8};
  const std::vector<double> y4{6.58, 5.76, 7.71, 8.84, 8.47, 7.04, 5.25, 12.50, 5.56, 7.91, 6.89};
  return table::DataFrame({{"x", table::Shelf(pluss::make_series(x))},
                    {"y1", table::Shelf(pluss::make_series(y1))},
                    {"y2", table::Shelf(pluss::make_series(y2))},
                    {"y3", table::Shelf(pluss::make_series(y3))},
                    {"x4", table::Shelf(pluss::make_series(x4))},
                    {"y4", table::Shelf(pluss::make_series(y4))}});
}

// Twelve short messages written for this library, half about orders and half about delivery
// problems, with the label a text model would learn.
[[nodiscard]] inline table::DataFrame messages_() {
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
  return table::DataFrame({{"id", table::Shelf(pluss::make_series(id))},
                    {"text", table::Shelf(pluss::make_note(text))},
                    {"label", table::Shelf(pluss::make_note(label))}});
}


// ---- 1D: one column -----------------------------------------------------------------------------------------

[[nodiscard]] inline table::Shelf temperature_() {
  std::vector<double> value;
  for (int h = 0; h < 48; ++h) {
    const int hour = h % 24;
    const double swing = std::sin((hour - 9) * 3.14159265358979323846 / 12.0);
    value.push_back(std::round((14.0 + 6.0 * swing + 0.8 * (unit_(41, static_cast<std::uint64_t>(h)) - 0.5)) * 10.0) / 10.0);
  }
  return table::Shelf(pluss::make_series(value));
}

[[nodiscard]] inline table::Shelf cities_() {
  return table::Shelf(pluss::make_note(std::vector<std::string>{"Accra", "Kumasi", "Tamale", "Takoradi", "Tema", "Cape Coast", "Sunyani", "Ho"}));
}

// The ratio column of accounts: ordinary numbers, a missing value (NULL), NaN, inf and -inf.
[[nodiscard]] inline table::Shelf gaps_() { return accounts_().features("ratio"); }

[[nodiscard]] inline table::Shelf hours_() {
  std::vector<std::string> stamp;
  for (int hour = 0; hour < 24; ++hour) stamp.push_back(std::string("2024-03-01T") + (hour < 10 ? "0" : "") + std::to_string(hour) + ":00:00Z");
  return table::Shelf(pluss::make_timeline(stamp));
}

[[nodiscard]] inline table::Shelf ratings_() {
  std::vector<std::int64_t> value;
  for (std::uint64_t i = 0; i < 30; ++i) value.push_back(1 + static_cast<std::int64_t>(unit_(53, i) * 5.0));
  return table::Shelf(pluss::make_series(value));
}

// ---- 1D: one stream -----------------------------------------------------------------------------------------

// Heart rate once a minute for twelve minutes, each record labelled with its minute.
[[nodiscard]] inline kindle::Stream pulse_() {
  std::vector<double> bpm{72, 74, 73, 78, 91, 104, 112, 109, 97, 88, 80, 76};
  std::vector<std::string> minute;
  for (int m = 1; m <= 12; ++m) minute.push_back("min " + std::to_string(m));
  return kindle::Stream(pluss::make_series(bpm), std::nullopt, table::Shelf(pluss::make_note(minute)));
}

[[nodiscard]] inline kindle::Stream words_() {
  return kindle::Stream(pluss::make_note(std::vector<std::string>{"alpha", "bravo", "charlie", "delta", "echo", "foxtrot"}));
}

// Eight readings of which the fifth is missing (NULL) and the sixth is a division by zero (NaN).
[[nodiscard]] inline kindle::Stream outliers_() {
  return kindle::Stream(pluss::make_series(std::vector<double>{4.1, 3.9, 4.4, 4.0, 0.0, std::nan(""), 4.2, 97.0}),
                        pluss::make_series(std::vector<bool>{true, true, true, true, false, true, true, true}));
}

// ---- 2D: a catalog whose streams have the same length ---------------------------------------------------------

[[nodiscard]] inline kindle::Catalog weather_() {
  std::vector<double> temperature, humidity, pressure, wind;
  for (int h = 0; h < 24; ++h) {
    const double swing = std::sin((h - 9) * 3.14159265358979323846 / 12.0);
    const auto k = static_cast<std::uint64_t>(h);
    temperature.push_back(std::round((15.0 + 6.0 * swing + 0.8 * (unit_(61, k) - 0.5)) * 10.0) / 10.0);
    humidity.push_back(std::round((60.0 - 15.0 * swing + 3.0 * (unit_(63, k) - 0.5)) * 10.0) / 10.0);
    pressure.push_back(std::round((1012.0 - 1.5 * swing + 0.6 * (unit_(67, k) - 0.5)) * 10.0) / 10.0);
    wind.push_back(std::round((9.0 + 4.0 * swing + 2.0 * (unit_(71, k) - 0.5)) * 10.0) / 10.0);
  }
  return kindle::Catalog(std::vector<std::pair<std::string, kindle::Stream>>{
      {"temperature", kindle::Stream(pluss::make_series(temperature))}, {"humidity", kindle::Stream(pluss::make_series(humidity))},
      {"pressure", kindle::Stream(pluss::make_series(pressure))}, {"wind", kindle::Stream(pluss::make_series(wind))}});
}

// Marks out of 100 for six students in three subjects.
[[nodiscard]] inline kindle::Catalog grades_() {
  const auto marks = [](std::vector<double> v) { return kindle::Stream(pluss::make_series(std::move(v))); };
  return kindle::Catalog(std::vector<std::pair<std::string, kindle::Stream>>{
      {"maths", marks({78, 92, 65, 88, 54, 71})}, {"english", marks({82, 70, 74, 91, 63, 68})}, {"science", marks({71, 95, 58, 84, 60, 77})}});
}

// Closing prices of three shares over ten days; the second share has no price on day 5 (NULL).
[[nodiscard]] inline kindle::Catalog prices_() {
  const auto close = [](std::vector<double> v, std::vector<bool> present = {}) {
    return present.empty() ? kindle::Stream(pluss::make_series(std::move(v)))
                           : kindle::Stream(pluss::make_series(std::move(v)), pluss::make_series(std::move(present)));
  };
  return kindle::Catalog(std::vector<std::pair<std::string, kindle::Stream>>{
      {"alpha", close({10.2, 10.4, 10.1, 10.6, 10.9, 11.0, 10.8, 11.2, 11.5, 11.4})},
      {"beta", close({48.0, 47.5, 47.9, 48.8, 0.0, 49.1, 49.6, 49.2, 50.0, 50.4}, {true, true, true, true, false, true, true, true, true, true})},
      {"gamma", close({3.05, 3.10, 3.02, 2.98, 3.00, 3.12, 3.20, 3.18, 3.25, 3.30})}});
}

// ---- ragged 2D: a catalog whose streams differ in length --------------------------------------------------------

// Monthly rainfall in millimetres at four stations that began recording in different months; one month at Wa is
// missing (NULL), as a gauge was out of order.
[[nodiscard]] inline kindle::Catalog rainfall_() {
  const auto mm = [](std::vector<double> v) { return pluss::make_series(std::move(v)); };
  return kindle::Catalog(std::vector<std::pair<std::string, kindle::Stream>>{
      {"Accra", kindle::Stream(mm({18, 34, 62, 88, 146, 189, 62, 23, 41, 66, 36, 16}))},
      {"Kumasi", kindle::Stream(mm({24, 49, 98, 131, 156, 191, 124, 94, 177, 160, 70, 25}))},
      {"Tamale", kindle::Stream(mm({2, 8, 39, 85, 130, 162, 182, 222, 252}))},
      {"Wa", kindle::Stream(mm({1, 5, 31, 80, 0, 133, 169}), pluss::make_series(std::vector<bool>{true, true, true, true, false, true, true}))}});
}

// How long each visitor stayed on each page they opened, in seconds: every visitor opened a different number of pages.
[[nodiscard]] inline kindle::Catalog sessions_() {
  const auto secs = [](std::vector<double> v) { return kindle::Stream(pluss::make_series(std::move(v))); };
  return kindle::Catalog(std::vector<std::pair<std::string, kindle::Stream>>{
      {"ama", secs({42, 128, 15, 77, 203})}, {"kofi", secs({9, 31, 64})}, {"esi", secs({55, 12, 140, 37, 22, 91, 18, 66})},
      {"yaw", secs({300})}, {"abena", secs({25, 48, 11, 83, 39, 57})}});
}

// Short review comments for three products; popular ones have more.
[[nodiscard]] inline kindle::Catalog reviews_() {
  const auto words = [](std::vector<std::string> v) { return kindle::Stream(pluss::make_note(std::move(v))); };
  return kindle::Catalog(std::vector<std::pair<std::string, kindle::Stream>>{
      {"lamp", words({"bright", "sturdy", "arrived late", "good value"})}, {"kettle", words({"leaks", "fast"})},
      {"chair", words({"comfortable", "wobbles", "easy to build"})}});
}

// ---- scribble: each cell holds a whole value (a series, text, a time or a duration) ------------------------------

using Value = scribble::Slot::Value;

[[nodiscard]] inline Value number_(double v) { return pluss::make_series(std::vector<double>{v}, {1}); }
[[nodiscard]] inline Value text_(const std::string& v) { return pluss::make_note(std::vector<std::string>{v}, {1}); }
[[nodiscard]] inline Value when_(const std::string& v) { return pluss::make_timeline(std::vector<std::string>{v}); }
[[nodiscard]] inline Value span_(std::int64_t seconds) { return pluss::duration::nanos(std::vector<std::int64_t>{seconds * 1'000'000'000LL}); }
[[nodiscard]] inline Value curve_(std::vector<double> v) {
  const auto n = static_cast<std::int64_t>(v.size());
  return pluss::make_series(std::move(v), {n});
}

// 1D: a slot of six scalar scores.
[[nodiscard]] inline scribble::Slot scores_() {
  return scribble::Slot({number_(61.5), number_(74.0), number_(88.25), number_(93.0), number_(55.5), number_(79.75)});
}

// 1D: a slot whose cells are curves of eight points each (a spectrum per sample).
[[nodiscard]] inline scribble::Slot spectra_() {
  return scribble::Slot({curve_({0.1, 0.4, 0.9, 1.6, 1.2, 0.7, 0.3, 0.1}), curve_({0.2, 0.3, 0.8, 1.9, 1.5, 0.6, 0.2, 0.1}),
                         curve_({0.1, 0.5, 1.1, 1.4, 1.0, 0.8, 0.4, 0.2}), curve_({0.3, 0.6, 0.7, 1.2, 1.3, 0.9, 0.5, 0.3})});
}

// 1D: one cell of every kind, and an empty one.
[[nodiscard]] inline scribble::Slot mixed_() {
  return scribble::Slot({number_(42.0), text_("hello"), when_("2024-03-01T12:00:00Z"), span_(90), Value{std::monostate{}}});
}

// 2D: five patients, one value of each kind in every field.
[[nodiscard]] inline scribble::Registry patients_() {
  return scribble::Registry({
      {"name", scribble::Slot({text_("Ama"), text_("Kofi"), text_("Esi"), text_("Yaw"), text_("Abena")})},
      {"heart_rate", scribble::Slot({number_(72), number_(88), number_(64), number_(95), number_(70)})},
      {"admitted", scribble::Slot({when_("2024-03-01T08:15:00Z"), when_("2024-03-01T09:40:00Z"), when_("2024-03-02T11:05:00Z"),
                                   when_("2024-03-02T14:30:00Z"), when_("2024-03-03T07:50:00Z")})},
      {"waiting", scribble::Slot({span_(600), span_(1500), span_(240), span_(2700), span_(900)})},
  });
}

// 2D: four orders, each with a list of line prices (a curve) and a note.
[[nodiscard]] inline scribble::Registry orders_() {
  return scribble::Registry({
      {"customer", scribble::Slot({text_("Accra Mart"), text_("Tema Foods"), text_("Kumasi Stores"), text_("Cape Cafe")})},
      {"lines", scribble::Slot({curve_({12.5, 8.0, 30.0}), curve_({99.0}), curve_({4.5, 4.5, 4.5, 9.0}), curve_({15.0, 22.0})})},
      {"note", scribble::Slot({text_("deliver Monday"), text_("fragile"), text_("none"), text_("call first")})},
  });
}

// ragged 2D: three runs of an experiment, with 3, 5 and 2 trials.
[[nodiscard]] inline scribble::Registry trials_() {
  return scribble::Registry({
      {"run_a", scribble::Slot({number_(1.2), number_(1.4), number_(1.1)})},
      {"run_b", scribble::Slot({number_(2.0), number_(2.2), number_(1.9), number_(2.1), number_(2.3)})},
      {"run_c", scribble::Slot({number_(0.8), number_(0.9)})},
  });
}

// ragged 2D: a diary with a different number of entries in each field.
[[nodiscard]] inline scribble::Registry diary_() {
  return scribble::Registry({
      {"mood", scribble::Slot({number_(3), number_(4), number_(2), number_(5)})},
      {"note", scribble::Slot({text_("slept badly"), text_("long walk")})},
      {"when", scribble::Slot({when_("2024-03-01T07:00:00Z"), when_("2024-03-02T07:30:00Z"), when_("2024-03-03T06:45:00Z")})},
  });
}

using Held = std::variant<table::Shelf, table::DataFrame, kindle::Stream, kindle::Catalog, scribble::Slot, scribble::Registry>;

struct Entry {
  const char* name;
  const char* about;
  const char* shape;
  Held (*build)();
};

template <auto F>
Held make_() { return Held(F()); }

inline const std::vector<Entry>& entries_() {
  static const std::vector<Entry> all{
      // a table
      {"sales", "Units, revenue and returns for four cities over six months (invented)", "2D", &make_<&sales_>},
      {"accounts", "Seven accounts with a missing balance, NaN, inf and -inf (invented)", "2D", &make_<&accounts_>},
      {"sensors", "Two days of hourly readings with a gap in the humidity (invented)", "2D", &make_<&sensors_>},
      {"anscombe", "Anscombe's quartet: four datasets with the same statistics and different shapes", "2D", &make_<&anscombe_>},
      {"messages", "Twelve short order and delivery-problem messages with a label (invented)", "2D", &make_<&messages_>},
      // one column
      {"temperature", "Forty-eight hourly temperatures in degrees Celsius (invented)", "1D", &make_<&temperature_>},
      {"cities", "Eight cities of Ghana, as text", "1D", &make_<&cities_>},
      {"gaps", "Seven ratios with a missing value, NaN, inf and -inf (invented)", "1D", &make_<&gaps_>},
      {"hours", "The 24 hours of one day, as times", "1D", &make_<&hours_>},
      {"ratings", "Thirty survey answers from 1 to 5 (invented)", "1D", &make_<&ratings_>},
      // one stream
      {"pulse", "Twelve heart-rate readings, one a minute, labelled by minute (invented)", "1D", &make_<&pulse_>},
      {"words", "Six code words, as text", "1D", &make_<&words_>},
      {"outliers", "Eight readings with a missing value, a NaN and an outlier (invented)", "1D", &make_<&outliers_>},
      // a catalog
      {"weather", "A day of hourly temperature, humidity, pressure and wind, one stream each (invented)", "2D", &make_<&weather_>},
      {"grades", "Marks of six students in three subjects (invented)", "2D", &make_<&grades_>},
      {"prices", "Closing prices of three shares over ten days, one price missing (invented)", "2D", &make_<&prices_>},
      // a ragged catalog
      {"rainfall", "Monthly rainfall at four stations with 12, 12, 9 and 7 months on record, one month missing (invented)", "ragged 2D", &make_<&rainfall_>},
      {"sessions", "Seconds spent on each page by five visitors who opened 5, 3, 8, 1 and 6 pages (invented)", "ragged 2D", &make_<&sessions_>},
      {"reviews", "Review comments for three products, four, two and three of them (invented)", "ragged 2D", &make_<&reviews_>},
      // a slot
      {"scores", "Six scores, one value in each cell (invented)", "1D", &make_<&scores_>},
      {"spectra", "Four spectra of eight points, one curve in each cell (invented)", "1D", &make_<&spectra_>},
      {"mixed", "One cell of every kind: a number, text, a time, a duration and an empty cell", "1D", &make_<&mixed_>},
      // a registry
      {"patients", "Five patients with a name, a heart rate, an admission time and a waiting time (invented)", "2D", &make_<&patients_>},
      {"orders", "Four orders with a customer, a list of line prices and a note (invented)", "2D", &make_<&orders_>},
      // a ragged registry
      {"trials", "Three runs of an experiment with 3, 5 and 2 trials (invented)", "ragged 2D", &make_<&trials_>},
      {"diary", "A diary with 4 moods, 2 notes and 3 dates (invented)", "ragged 2D", &make_<&diary_>},
  };
  return all;
}

[[nodiscard]] inline const char* type_name_(const Held& value) {
  static const char* const names[] = {"Shelf", "DataFrame", "Stream", "Catalog", "Slot", "Registry"};
  return names[value.index()];
}

[[nodiscard]] inline std::pair<std::int64_t, std::int64_t> measure_(const Held& value) {
  return std::visit(
      [](const auto& item) -> std::pair<std::int64_t, std::int64_t> {
        using T = std::decay_t<decltype(item)>;
        if constexpr (std::is_same_v<T, table::Shelf> || std::is_same_v<T, kindle::Stream> || std::is_same_v<T, scribble::Slot>) {
          return {static_cast<std::int64_t>(item.size()), 1};
        } else if constexpr (std::is_same_v<T, table::DataFrame>) {
          return {static_cast<std::int64_t>(item.ncases()), static_cast<std::int64_t>(item.nfeatures())};
        } else {
          const auto shape = item.shape();
          return {static_cast<std::int64_t>(shape.first), static_cast<std::int64_t>(shape.second)};
        }
      },
      value);
}

}  // namespace datastore_detail

// A dataset and its name. It holds one of the six container types; the accessors return that one and fail, naming the
// one it holds, when asked for another.
class Datastore {
 public:
  Datastore(std::string name, datastore_detail::Held value) : name_(std::move(name)), value_(std::move(value)) {}

  [[nodiscard]] const std::string& name() const noexcept { return name_; }
  // Shelf, DataFrame, Stream, Catalog, Slot or Registry.
  [[nodiscard]] std::string type() const { return datastore_detail::type_name_(value_); }

  template <class T>
  [[nodiscard]] const T& as() const {
    if (const auto* held = std::get_if<T>(&value_)) return *held;
    PLUSS_FAIL("pluss::loader::datastore(\"" + name_ + "\") holds a " + type() + ", which is not the type that was asked for", name_, std::vector<std::string>{type()});
  }

  [[nodiscard]] const table::Shelf& shelf() const { return as<table::Shelf>(); }
  [[nodiscard]] const table::DataFrame& frame() const { return as<table::DataFrame>(); }
  [[nodiscard]] const kindle::Stream& stream() const { return as<kindle::Stream>(); }
  [[nodiscard]] const kindle::Catalog& catalog() const { return as<kindle::Catalog>(); }
  [[nodiscard]] const scribble::Slot& slot() const { return as<scribble::Slot>(); }
  [[nodiscard]] const scribble::Registry& registry() const { return as<scribble::Registry>(); }

  // So that `pluss::table::DataFrame sales = pluss::loader::datastore("sales");` works.
  operator const table::Shelf&() const { return shelf(); }
  operator const table::DataFrame&() const { return frame(); }
  operator const kindle::Stream&() const { return stream(); }
  operator const kindle::Catalog&() const { return catalog(); }
  operator const scribble::Slot&() const { return slot(); }
  operator const scribble::Registry&() const { return registry(); }

  friend std::ostream& operator<<(std::ostream& out, const Datastore& store) {
    std::visit([&out](const auto& item) { out << item; }, store.value_);
    return out;
  }

 private:
  std::string name_;
  datastore_detail::Held value_;
};

// Every dataset: its name, what it is, its type, its shape and its size.
[[nodiscard]] inline std::vector<DatastoreInfo> datastores() {
  std::vector<DatastoreInfo> out;
  for (const auto& entry : datastore_detail::entries_()) {
    const auto built = entry.build();
    const auto [cases, features] = datastore_detail::measure_(built);
    out.push_back(DatastoreInfo{entry.name, entry.about, datastore_detail::type_name_(built), entry.shape, cases, features});
  }
  return out;
}

// One dataset by name. An unknown name is an error that lists the names that exist.
[[nodiscard]] inline Datastore datastore(std::string_view name) {
  std::vector<std::string> known;
  for (const auto& entry : datastore_detail::entries_()) {
    if (name == entry.name) return Datastore(std::string(name), entry.build());
    known.emplace_back(entry.name);
  }
  PLUSS_FAIL("pluss::loader::datastore: no dataset called '" + std::string(name) + "'", std::string(name), known);
}

}  // namespace pluss::loader

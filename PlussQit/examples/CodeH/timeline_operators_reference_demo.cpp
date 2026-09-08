#include <pluss/pluss.h>

#include <iostream>
#include <limits>
#include <string_view>

namespace {
void show(std::string_view label, const pluss::Timeline& value) {
  std::cout << "\n=== " << label << " ===\n"
            << pluss::timeline::print(value) << '\n';
}

void show(std::string_view label, const pluss::Duration& value) {
  std::cout << "\n=== " << label << " ===\n"
            << pluss::series::print(value.total().seconds()) << '\n';
}

void show(std::string_view label, const pluss::Series& value) {
  std::cout << "\n=== " << label << " ===\n"
            << pluss::series::print(value) << '\n';
}

void show_epochs(std::string_view label, const pluss::Timeline& value) {
  show(label, value.epoch_nanos());
}

template <class Fn>
void show_rejection(std::string_view label, std::string_view reason, Fn&& fn) {
  try {
    fn();
    std::cout << "\n=== " << label << " ===\nnot rejected\n";
  } catch (const std::exception&) {
    std::cout << "\n=== " << label << " ===\nrejected: " << reason << '\n';
  }
}
} // namespace

int main() {
  const auto base = pluss::timeline::make({
      "2024-01-01T00:00:00Z", "2024-01-01T02:00:00Z"});
  const auto delta = pluss::duration::hours({1, -2});

  std::cout << "\n=== stream insertion ===\n" << base << '\n';

  show("timeline + duration", base + delta);
  show("duration + timeline", delta + base);
  show("timeline - duration", base - delta);
  show("timeline - timeline", (base + delta) - base);

  const auto scalar = pluss::timeline::make("2024-01-01T00:00:00Z");
  show("scalar + scalar duration", scalar + pluss::duration::minutes(90));

  const std::vector<std::string> row_time_values{
      "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z"};
  const auto row_times = pluss::timeline::make(row_time_values, {2, 1});
  const auto column_offsets = pluss::duration::minutes({{0, 30, 60}});
  show("two-dimensional outer broadcast", row_times + column_offsets);

  const auto precise = pluss::timeline::from_epoch({100LL, pluss::Timeline::kNaT});
  show_epochs("nanosecond arithmetic", precise + pluss::duration::nanos({7, 9}));

  const auto before_spring_clock_change =
      pluss::timeline::make("2024-03-10T08:30:00Z")
          .tz_convert("America/Edmonton");
  show("fixed elapsed hours across clock change",
       before_spring_clock_change + pluss::duration::hours(2));

  auto changed = base;
  changed += pluss::duration::hours(1);
  changed -= pluss::duration::minutes(30);
  show("compound assignment", changed);
  show("original after compound assignment on copy", base);

  const auto later = pluss::timeline::make({
      "2024-01-01T00:00:00Z", "2024-01-01T03:30:00Z"});
  show("signed timeline difference", later - base);

  const auto comparison_matrix = pluss::timeline::make({
      {"2024-01-01T00:00:00Z", "2024-01-01T02:00:00Z"},
      {"2024-01-02T00:00:00Z", "2024-01-02T02:00:00Z"},
  });
  const auto comparison_row = pluss::timeline::make({
      "2024-01-01T01:00:00Z", "2024-01-02T02:00:00Z"});
  show("comparison row broadcast", comparison_matrix < comparison_row);

  const auto boundary = pluss::timeline::make("2024-01-01T02:00:00Z");
  show("operator ==", base == boundary);
  show("operator !=", base != boundary);
  show("operator <", base < boundary);
  show("operator <=", base <= boundary);
  show("operator >", base > boundary);
  show("operator >=", base >= boundary);

  const auto partly_matching = pluss::timeline::make({
      "2024-01-01T00:00:00Z", "2024-01-01T03:00:00Z"});
  const auto matches = base == partly_matching;
  std::cout << "\n=== explicit comparison reductions ===\n"
            << "all=" << std::boolalpha << matches.all().item<bool>()
            << ", any=" << matches.any().item<bool>() << '\n';

  const auto start = pluss::timeline::make("2024-01-01T00:00:00Z");
  show("operator > start", base > start);
  show("operator >= start", base >= start);

  const auto samples = pluss::timeline::make({
      "2024-01-01T00:00:00Z", "2024-01-01T01:00:00Z",
      "2024-01-01T02:00:00Z", "2024-01-01T03:00:00Z"});
  show("closed interval", (samples >= start).logical_and(samples <= boundary));

  const auto utc = pluss::timeline::make("2024-07-01T18:00:00Z");
  show("timezone-converted equality",
       utc == utc.tz_convert("America/Edmonton"));

  const auto with_nat = pluss::timeline::make({"", "2024-01-01T00:00:00Z"});
  show("NaT == self", with_nat == with_nat);
  show("NaT != self", with_nat != with_nat);
  show("NaT < self", with_nat < with_nat);
  show("NaT <= self", with_nat <= with_nat);
  show("NaT > self", with_nat > with_nat);
  show("NaT >= self", with_nat >= with_nat);

  const std::vector<std::int64_t> no_ticks;
  const std::vector<std::int64_t> empty_shape{0, 2};
  const auto empty = pluss::timeline::from_epoch(no_ticks, empty_shape);
  show("empty arithmetic", empty + pluss::duration::hours(1));
  show("empty comparison", empty == pluss::timeline::make("2024-01-01T00:00:00Z"));

  show_rejection("incompatible shapes", "shapes do not broadcast", [&] {
    const auto matrix = pluss::timeline::make({
        {"2024-01-01T00:00:00Z", "2024-01-01T01:00:00Z"},
        {"2024-01-01T02:00:00Z", "2024-01-01T03:00:00Z"},
    });
    (void)(matrix == pluss::timeline::make({
        "2024-01-01T00:00:00Z", "2024-01-01T01:00:00Z",
        "2024-01-01T02:00:00Z"}));
  });

  const auto latest = pluss::timeline::from_epoch(
      std::numeric_limits<std::int64_t>::max());
  const auto earliest = pluss::timeline::from_epoch(
      std::numeric_limits<std::int64_t>::min() + 1);
  show_rejection("addition overflow", "result is outside Timeline range", [&] {
    (void)(latest + pluss::duration::nanos(1));
  });
  show_rejection("subtraction overflow", "result is outside Timeline range", [&] {
    (void)(earliest - pluss::duration::nanos(1));
  });
  show_rejection("difference overflow", "result is outside Duration range",
                 [&] { (void)(latest - earliest); });
}

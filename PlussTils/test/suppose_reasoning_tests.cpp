#include <pluss/tils/suppose.h>

#include <stdexcept>
#include <string>
#include <vector>

namespace pluss::codem {
// A small, move-only stand-in keeps this header-only test independent of CodeM.
class Series {
 public:
  explicit Series(int value) : value_(value) {}
  Series(const Series&) = delete;
  Series& operator=(const Series&) = delete;
  Series(Series&&) = default;
  Series& operator=(Series&&) = default;
  int value() const { return value_; }
  Series scaled(int factor) const { return Series(value_ * factor); }

 private:
  int value_;
};
}  // namespace pluss::codem

namespace {
void expect(bool condition) {
  if (!condition) throw std::runtime_error("suppose reasoning test failed");
}

template <class T>
concept CanSuppose = requires(T& value) {
  pluss::tils::suppose("premise", value, [](const auto&) { return true; });
};
static_assert(CanSuppose<pluss::codem::Series>);
static_assert(!CanSuppose<int>);
static_assert(!CanSuppose<std::vector<int>>);
}  // namespace

int main() {
  using pluss::codem::Series;
  using pluss::tils::SuppositionStatus;
  using pluss::tils::SuppositionStepStatus;
  using pluss::tils::suppose;

  Series input(4);
  int calls = 0;
  auto checked = suppose("input is four", input,
      [](const Series& x) { return x.value() == 4; })
      .then("triple is twelve",
          [&](const Series& x) { ++calls; return x.scaled(3); },
          [&](const Series&, const Series& triple) {
            ++calls; return triple.value() == 12;
          })
      .conclude("positive", [&](const Series&, const Series& triple) {
        ++calls; return triple.value() > 0;
      });
  expect(checked.verified() && calls == 3);
  expect(checked.derived<0>()->value() == 12 && input.value() == 4);
  expect(checked.steps()[1].status == SuppositionStepStatus::passed);

  auto absent = suppose("input is five", input,
      [](const Series& x) { return x.value() == 5; })
      .then("never computed", [&](const Series& x) { ++calls; return x.scaled(3); })
      .conclude("never checked", [&](const Series&, const Series&) {
        ++calls; return true;
      });
  expect(!absent.applicable() && calls == 3 && !absent.derived<0>());
  expect(absent.status() == SuppositionStatus::premise_not_met);
  expect(absent.failed_step() == "input is five");
  expect(absent.steps()[1].status == SuppositionStepStatus::skipped);

  auto failed = suppose("input is four", Series(4),
      [](const Series& x) { return x.value() == 4; })
      .then("wrong relation", [](const Series& x) { return x.scaled(3); },
            [](const Series&, const Series& triple) { return triple.value() == 13; })
      .then("skipped", [&](const Series&, const Series& triple) {
        ++calls; return triple.scaled(2);
      })
      .conclude("also skipped", [&](const Series&, const Series&, const Series&) {
        ++calls; return true;
      });
  expect(failed.status() == SuppositionStatus::derivation_failed);
  expect(failed.failed_step() == "wrong relation" && calls == 3);
  expect(failed.derived<0>()->value() == 12 && !failed.derived<1>());
  expect(failed.steps()[2].status == SuppositionStepStatus::skipped);

  auto conclusion = suppose("input is four", input,
      [](const Series& x) { return x.value() == 4; })
      .conclude("input is negative", [](const Series& x) { return x.value() < 0; });
  expect(conclusion.status() == SuppositionStatus::conclusion_failed);
  expect(conclusion.failed_step() == "input is negative");
}

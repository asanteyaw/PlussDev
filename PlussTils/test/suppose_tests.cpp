#include <pluss/pluss.h>
#include <pluss/tils/suppose.h>

#include <stdexcept>
#include <string>
#include <vector>

namespace {

template <class T>
concept CanSuppose = requires(T& value) {
  pluss::tils::suppose("named", value, [](const T&) { return true; });
};

static_assert(CanSuppose<pluss::Series>);
static_assert(!CanSuppose<int>);
static_assert(!CanSuppose<std::vector<double>>);

void expect(bool condition) {
  if (!condition) throw std::runtime_error("suppose test failed");
}

}  // namespace

int main() {
  auto values = pluss::make_series({2.0, 4.0, 6.0});
  int calls = 0;
  auto argument = pluss::tils::suppose("three observations", values,
      [&](const pluss::Series& x) { ++calls; return x.numel() == 3; });
  expect(&argument.value() == &values);  // An lvalue is borrowed, not copied.
  auto verified = argument
      .then("positive sum", [&](const pluss::Series& x) {
        ++calls;
        return x.sum().item<double>() > 0.0;
      })
      .conclude("positive mean", [&](const pluss::Series& x) {
        ++calls;
        return x.mean().item<double>() > 0.0;
      });
  expect(verified.verified() && verified.applicable() && calls == 3);
  expect(verified.steps().size() == 3 && verified.failed_step().empty());

  auto absent = pluss::tils::suppose("four observations", values,
      [](const pluss::Series& x) { return x.numel() == 4; })
      .then("skipped derivation", [&](const pluss::Series&) { ++calls; return true; })
      .conclude("skipped conclusion", [&](const pluss::Series&) { ++calls; return true; });
  expect(!absent.applicable() && !absent.verified() && calls == 3);
  expect(absent.steps()[1].status == pluss::tils::SuppositionStepStatus::skipped);
  expect(absent.steps()[2].status == pluss::tils::SuppositionStepStatus::skipped);

  auto failed = pluss::tils::suppose("three observations", values,
      [](const pluss::Series& x) { return x.numel() == 3; })
      .then("negative sum", [](const pluss::Series& x) {
        return x.sum().item<double>() < 0.0;
      })
      .conclude("skipped conclusion", [&](const pluss::Series&) { ++calls; return true; });
  expect(failed.status() == pluss::tils::SuppositionStatus::derivation_failed);
  expect(failed.failed_step() == "negative sum" && calls == 3);

  auto owned = pluss::tils::suppose("temporary series", pluss::make_series({1.0}),
      [](const pluss::Series& x) { return x.numel() == 1; })
      .conclude("one element", [](const pluss::Series& x) { return x.numel() == 1; });
  expect(owned.verified());

  auto host_flow = pluss::tils::flow(std::vector<int>{1, 2})
      .then([](const std::vector<int>& x) { return x.size(); });
  expect(host_flow.value() == 2);
}

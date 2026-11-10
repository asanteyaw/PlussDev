#include <pluss/pluss.h>
#include <pluss/tils/suppose.h>

#include <iostream>

int main() {
  using pluss::Series;
  using pluss::tils::suppose;

  auto values = pluss::make_series({2.0, 4.0, 6.0});

  // A named premise, derived Pluss value, and checked conclusion for this input.
  auto checked = suppose("three observations", values,
      [](const Series& x) { return x.numel() == 3; })
      .then("total is twelve", [](const Series& x) { return x.sum(); },
            [](const Series&, const Series& total) {
              return total.item<double>() == 12.0;
            })
      .conclude("mean is positive", [](const Series& x, const Series& total) {
        return total.item<double>() / x.numel() > 0.0;
      });
  std::cout << "verified: " << std::boolalpha << checked.verified() << '\n';
  if (!checked.verified()) {
    std::cout << "failed step: " << checked.failed_step() << '\n';
    return 1;
  }
  std::cout << "total: " << checked.derived<0>()->item<double>() << '\n';
}

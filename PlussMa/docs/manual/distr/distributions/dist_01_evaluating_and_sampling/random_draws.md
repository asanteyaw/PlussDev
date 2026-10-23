<!-- generated from random_draws.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Random draws

[Back to Evaluating and Sampling](../dist_01_evaluating_and_sampling.md)


```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace distr = pluss::subsp::stats::distr;
using pluss::Series;

template <class D>
void check(const char* name, const D& d, double threshold) {
  const Series draws = d.sample({200000}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 20261022ULL);
  const auto v = draws.template to_vec<double>();
  double mean = 0.0;
  for (double a : v) mean += a;
  mean /= static_cast<double>(v.size());
  double var = 0.0, below = 0.0;
  for (double a : v) {
    var += (a - mean) * (a - mean);
    if (a <= threshold) below += 1.0;
  }
  var /= static_cast<double>(v.size());
  const double p = d.cdf(pluss::make_series(std::vector<double>{threshold}, {1}).to(pluss::ScalarType::Float64)).template item<double>();
  std::cout << std::left << std::setw(20) << name << std::right << std::setw(10) << mean << std::setw(10) << d.cumulant(1).template item<double>() << std::setw(10) << var << std::setw(10)
            << d.cumulant(2).template item<double>() << std::setw(12) << below / static_cast<double>(v.size()) << std::setw(10) << p << "\n";
}
int main() {
  std::cout << std::fixed << std::setprecision(4);
  std::cout << std::left << std::setw(20) << "200000 draws" << std::right << std::setw(10) << "mean" << std::setw(10) << "exact" << std::setw(10) << "variance" << std::setw(10) << "exact"
            << std::setw(12) << "P(x<=t)" << std::setw(10) << "cdf(t)" << "\n";
  check("Normal(0.5, 2)", distr::Normal{0.5, 2.0}, 1.0);
  check("Gamma(2.5, 1.3)", distr::Gamma{2.5, 1.3, 0.0}, 3.0);
  check("Beta(2.5, 4)", distr::Beta{0.0, 1.0, 2.5, 4.0}, 0.4);
  check("Poisson(3.2)", distr::Poisson{3.2}, 3.0);
  check("Weibull(1.7, 2.1)", distr::Weibull{1.7, 2.1}, 2.0);

  // Reproducibility, shapes and dtypes.
  const distr::Normal n{0.0, 1.0};
  const Series a = n.sample({4}, {}, 7ULL), b = n.sample({4}, {}, 7ULL), c = n.sample({4}, {}, 8ULL);
  std::cout << "\nsame seed, same draws:      " << ((a - b).abs().max().item<double>() == 0.0 ? "yes" : "no") << "\n";
  std::cout << "different seed, different:  " << ((a - c).abs().max().item<double>() > 0.0 ? "yes" : "no") << "\n";
  const Series grid = n.sample({2, 3}, {}, 1ULL);
  std::cout << "sample({2, 3}) has shape    [" << grid.sizes()[0] << ", " << grid.sizes()[1] << "]\n";
  const Series like = pluss::series::zeros({5, 2}, pluss::Config{}.with_dtype(pluss::ScalarType::Float32));
  const Series shaped = n.sample_like(like, 3ULL);
  std::cout << "sample_like a 5 x 2 float32 array: shape [" << shaped.sizes()[0] << ", " << shaped.sizes()[1] << "], dtype float32: " << (shaped.scalar_type() == pluss::ScalarType::Float32 ? "yes" : "no") << "\n";
}
```

Output

```text
200000 draws              mean     exact  variance     exact     P(x<=t)    cdf(t)
Normal(0.5, 2)          0.4976    0.5000    4.0087    4.0000      0.5993    0.5987
Gamma(2.5, 1.3)         3.2509    3.2500    4.2246    4.2250      0.5363    0.5354
Beta(2.5, 4)            0.3851    0.3846    0.0316    0.0316      0.5545    0.5558
Poisson(3.2)            3.1964    3.2000    3.2088    3.2000      0.6039    0.6025
Weibull(1.7, 2.1)       1.8756    1.8737    1.2838    1.2871      0.6010    0.6016

same seed, same draws:      yes
different seed, different:  yes
sample({2, 3}) has shape    [2, 3]
sample_like a 5 x 2 float32 array: shape [5, 2], dtype float32: yes
```
With 200 000 draws the sample mean agrees with the exact mean to about three decimals (the standard error of a mean is sd / √n: 0.0045 for the first normal), the variance to within a percent, and the fraction of draws below a threshold with the distribution function there. The same seed gives identical draws, a different seed different ones, and `sample_like` returns an array of the same shape and dtype as its template.

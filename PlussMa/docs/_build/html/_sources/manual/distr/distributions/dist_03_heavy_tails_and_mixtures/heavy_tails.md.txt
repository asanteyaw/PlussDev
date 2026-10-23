<!-- generated from heavy_tails.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Heavy tails

[Back to Heavy Tails and Mixtures](../dist_03_heavy_tails_and_mixtures.md)


```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace distr = pluss::subsp::stats::distr;
using pluss::Series;

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const Series x = distr::Nig{1.5, -0.4, 0.1, 0.8}.sample({2000}, cfg, 20261022ULL);

  const auto normal = distr::Normal{}.MLE(x);
  const auto nig = distr::Nig{}.MLE(x);
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "true NIG(1.5, -0.4, 0.1, 0.8)\n";
  std::cout << "fitted NIG   alpha " << nig.alpha << "  beta " << nig.beta << "  location " << nig.location << "  scale " << nig.scale << "\n";
  const auto se = nig.stderr(x).to_vec<double>();
  std::cout << "std errors        " << std::setw(6) << se[0] << "       " << std::setw(6) << se[1] << "            " << std::setw(6) << se[2] << "         " << std::setw(6) << se[3] << "\n\n";

  const double ll_n = normal.loglik(x).item<double>(), ll_g = nig.loglik(x).item<double>();
  std::cout << std::setprecision(2) << std::setw(10) << "model" << std::setw(14) << "loglik" << std::setw(10) << "AIC" << "\n";
  std::cout << std::setw(10) << "normal" << std::setw(14) << ll_n << std::setw(10) << 2 * 2 - 2 * ll_n << "\n";
  std::cout << std::setw(10) << "NIG" << std::setw(14) << ll_g << std::setw(10) << 2 * 4 - 2 * ll_g << "\n\n";

  // Probability of a fall below -3 (about three standard deviations): empirical, normal and NIG.
  const double cut = -3.0;
  const auto v = x.to_vec<double>();
  double below = 0.0;
  for (double a : v) if (a < cut) below += 1.0;
  const Series t = pluss::make_series(std::vector<double>{cut}, {1}).to(pluss::ScalarType::Float64);
  std::cout << std::setprecision(5) << "P(x < " << cut << ")   sample " << below / static_cast<double>(v.size()) << "   normal fit " << normal.model().cdf(t).item<double>() << "   NIG fit "
            << nig.model().cdf(t).item<double>() << "\n";
}
```

Output

```text
true NIG(1.5, -0.4, 0.1, 0.8)
fitted NIG   alpha 1.7454  beta -0.5035  location 0.1308  scale 0.8595
std errors        0.1829       0.0998            0.0421         0.0677

     model        loglik       AIC
    normal      -2261.36   4526.73
       NIG      -2160.58   4329.16

P(x < -3.00000)   sample 0.00350   normal fit 0.00006   NIG fit 0.00364
```
The sample comes from an NIG with a heavy left tail. The fit recovers it (alpha 1.75 for a true 1.5, within one and a half standard errors; beta −0.50 against −0.4). Against a normal with the same mean and variance the log-likelihood rises by about 100 for two extra parameters (Akaike's criterion falls from 4527 to 4329). Most telling is the tail: the sample has a fraction 0.0035 of values below −3, the normal fit assigns that region a probability of 0.00006, sixty times too small, and the NIG fit 0.00364.

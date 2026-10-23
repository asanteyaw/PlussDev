<!-- generated from the_closed_form_families.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The closed-form families

[Back to Maximum Likelihood](../dist_02_maximum_likelihood.md)


```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace distr = pluss::subsp::stats::distr;
using pluss::Series;

void row(const char* label, const Series& s) {
  std::cout << std::left << std::setw(14) << label << std::right;
  for (double v : s.reshape({s.numel()}).to_vec<double>()) std::cout << std::setw(11) << v;
  std::cout << "\n";
}
int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const double n = 500.0;
  std::cout << std::fixed << std::setprecision(4);

  const Series xn = distr::Normal{1.5, 2.0}.sample({500}, cfg, 20261022ULL);
  const auto nf = distr::Normal{}.MLE(xn);
  std::cout << "Normal sample (true mean 1.5, sd 2.0)\n";
  std::cout << "  estimates     mean " << nf.mean << "   stddev " << nf.stddev << "\n";
  row("  stderr", nf.stderr(xn));
  std::cout << "  exact         " << std::setw(11) << nf.stddev / std::sqrt(n) << std::setw(11) << nf.stddev / std::sqrt(2 * n) << "\n";
  row("  95% interval", nf.confint(xn));
  row("  score", nf.score(xn));
  row("  vcov", nf.vcov(xn));
  std::cout << "  log-likelihood " << nf.loglik(xn).item<double>() << "   per observation " << nf.loglik(xn).item<double>() / n << "\n\n";

  const Series xe = distr::Exponential{0.7}.sample({500}, cfg, 3ULL);
  const auto ef = distr::Exponential{}.MLE(xe);
  std::cout << "Exponential sample (true rate 0.7)\n  rate " << ef.rate << "   stderr " << ef.stderr(xe).item<double>() << "   exact " << ef.rate / std::sqrt(n) << "\n";
  row("  95% interval", ef.confint(xe));

  const Series xp = distr::Poisson{3.2}.sample({500}, cfg, 4ULL);
  const auto pf = distr::Poisson{}.MLE(xp);
  std::cout << "\nPoisson sample (true lambda 3.2)\n  lambda " << pf.lambda << "   stderr " << pf.stderr(xp).item<double>() << "   exact " << std::sqrt(pf.lambda / n) << "\n";
  row("  95% interval", pf.confint(xp));
}
```

Output

```text
Normal sample (true mean 1.5, sd 2.0)
  estimates     mean 1.4956   stddev 1.9274
  stderr           0.0862     0.0610
  exact              0.0862     0.0610
  95% interval     1.3266     1.6645     1.8080     2.0469
  score            0.0000    -0.0000
  vcov             0.0074    -0.0000    -0.0000     0.0037
  log-likelihood -1037.5579   per observation -2.0751

Exponential sample (true rate 0.7)
  rate 0.6825   stderr 0.0305   exact 0.0305
  95% interval     0.6227     0.7423

Poisson sample (true lambda 3.2)
  lambda 3.2540   stderr 0.0807   exact 0.0807
  95% interval     3.0959     3.4121
```
The estimates are close to the true values (a mean of 1.4956 for a true 1.5, a standard deviation of 1.9274 for a true 2.0), and the standard errors computed from the Hessian agree with the exact formulas to all printed digits (`σ/√n = 0.0862` for the mean, `σ/√(2n) = 0.0610` for the standard deviation). The score is zero at the estimate; the covariance of the normal is diagonal (mean and variance are uncorrelated). Each 95% interval is the estimate ± 1.96 standard errors and contains the true value here.

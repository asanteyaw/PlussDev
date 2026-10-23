<!-- generated from numerical_estimates_correlation_and_fixed_parame.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Numerical estimates, correlation and fixed parameters

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
  std::cout << std::left << std::setw(16) << label << std::right;
  for (double v : s.reshape({s.numel()}).to_vec<double>()) std::cout << std::setw(11) << v;
  std::cout << "\n";
}
int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  std::cout << std::fixed << std::setprecision(4);

  const Series xg = distr::Gamma{2.5, 1.3, 0.0}.sample({500}, cfg, 20261022ULL);
  const auto gf = distr::Gamma{1.0, 1.0, 0.0}.MLE(xg);
  std::cout << "Gamma sample (true shape 2.5, scale 1.3, location 0)\n";
  std::cout << "  shape " << gf.shape << "   scale " << gf.scale << "   location (fixed) " << gf.location << "\n";
  row("  stderr", gf.stderr(xg));
  row("  95% intervals", gf.confint(xg));
  const auto v = gf.vcov(xg).to_vec<double>();
  std::cout << "  correlation of shape and scale " << v[1] / std::sqrt(v[0] * v[4]) << "   (vcov is 3 x 3, the location row and column are zero)\n";
  std::cout << "  log-likelihood " << gf.loglik(xg).item<double>() << "\n";

  const Series xb = distr::Beta{0.0, 1.0, 2.5, 4.0}.sample({500}, cfg, 7ULL);
  const auto bf = distr::Beta{0.0, 1.0, 1.0, 1.0}.MLE(xb);
  std::cout << "\nBeta sample on [0, 1] (true alpha 2.5, beta 4)\n  alpha " << bf.alpha << "   beta " << bf.beta << "   (low " << bf.low << ", high " << bf.high << " fixed)\n";
  row("  stderr", bf.stderr(xb));
  // The same data on [-1, 3]: telling the object the support is how the family is fitted there.
  std::vector<double> shifted;
  for (double a : xb.to_vec<double>()) shifted.push_back(-1.0 + 4.0 * a);
  const auto bf2 = distr::Beta{-1.0, 3.0, 1.0, 1.0}.MLE(pluss::make_series(shifted, {500}).to(pluss::ScalarType::Float64));
  std::cout << "  the same data moved to [-1, 3]: alpha " << bf2.alpha << "   beta " << bf2.beta << "\n";

  const Series xn = distr::NegativeBinomial{3.5, 0.4}.sample({500}, cfg, 11ULL);
  const auto nf = distr::NegativeBinomial{1.0, 0.5}.MLE(xn);
  std::cout << "\nNegative binomial sample (true r 3.5, p 0.4)\n  r " << nf.r << "   p " << nf.p << "\n";
  row("  stderr", nf.stderr(xn));
  row("  95% intervals", nf.confint(xn));
  const auto nv = nf.vcov(xn).to_vec<double>();
  std::cout << "  correlation of r and p " << nv[1] / std::sqrt(nv[0] * nv[3]) << "\n";

  // Counts that are not over-dispersed (variance below the mean) have no finite maximum: r grows without bound (the Poisson limit); the fit reports a large r.
  const auto flat = distr::NegativeBinomial{1.0, 0.5}.MLE(distr::Binomial{10, 0.5}.sample({400}, cfg, 13ULL));
  std::cout << "\nunder-dispersed counts (a binomial sample): r = " << std::scientific << std::setprecision(1) << flat.r << "\n";
}
```

Output

```text
Gamma sample (true shape 2.5, scale 1.3, location 0)
  shape 2.4422   scale 1.3758   location (fixed) 0.0000
  stderr             0.1452     0.0908     0.0000
  95% intervals      2.1577     2.7267     1.1979     1.5536     0.0000     0.0000
  correlation of shape and scale -0.9010   (vcov is 3 x 3, the location row and column are zero)
  log-likelihood -1016.7271

Beta sample on [0, 1] (true alpha 2.5, beta 4)
  alpha 2.4846   beta 4.0644   (low 0.0000, high 1.0000 fixed)
  stderr             0.0000     0.0000     0.1490     0.2534
  the same data moved to [-1, 3]: alpha 2.4846   beta 4.0644

Negative binomial sample (true r 3.5, p 0.4)
  r 3.6458   p 0.4093
  stderr             0.4118     0.0283
  95% intervals      2.8388     4.4529     0.3539     0.4647
  correlation of r and p 0.9655

under-dispersed counts (a binomial sample): r = 1.0e+08
```
The gamma shape and scale are estimated together; their estimates are almost perfectly negatively correlated (−0.90): a larger shape goes with a smaller scale, because the mean `shape × scale` is determined much more precisely than either. The standard errors printed come from the full information matrix, so they are the exact values (they agree with the closed-form Fisher information of the gamma, `ψ′(shape)` and `1/scale` per observation). The location of the gamma is fixed at the object's value and reported with zero uncertainty. The beta is fitted on the support stored in the object: the same data moved to [−1, 3] and fitted with a beta on [−1, 3] give the same parameters. The negative binomial's `r` and `p` are strongly correlated too (0.97). Counts from a binomial are under-dispersed, so `r` is returned at its upper limit.

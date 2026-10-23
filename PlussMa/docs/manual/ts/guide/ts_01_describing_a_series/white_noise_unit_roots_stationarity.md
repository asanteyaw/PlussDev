<!-- generated from white_noise_unit_roots_stationarity.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# White noise, unit roots, stationarity

[Back to Describing a Series](../ts_01_describing_a_series.md)


```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
// A reproducible pseudo-random sequence in (-0.5, 0.5): e_t = frac(sin(12.9898 t + 78.233) * 43758.5453) - 0.5, t = 1, 2, ...
std::vector<double> noise(int n) {
  std::vector<double> e(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) {
    double u = std::sin(12.9898 * (i + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    e[static_cast<std::size_t>(i)] = u - 0.5;
  }
  return e;
}
// AR(1): x_t = phi x_{t-1} + e_t
std::vector<double> ar1(int n, double phi) {
  auto e = noise(n);
  std::vector<double> x(static_cast<std::size_t>(n));
  double carry = 0.0;
  for (int i = 0; i < n; ++i) { carry = phi * carry + e[static_cast<std::size_t>(i)]; x[static_cast<std::size_t>(i)] = carry; }
  return x;
}
std::vector<double> running_sum(const std::vector<double>& e) {
  std::vector<double> w(e.size());
  double s = 0.0;
  for (std::size_t i = 0; i < e.size(); ++i) { s += e[i]; w[i] = s; }
  return w;
}
void print_row(const char* label, const Series& s, int precision = 4) {
  std::cout << std::left << std::setw(16) << label << std::fixed << std::setprecision(precision);
  for (double v : s.to_vec<double>()) std::cout << " " << std::setw(8) << v;
  std::cout << "\n";
}
int main() {
  const auto e = noise(200);
  const Series stationary = vec(ar1(200, 0.7)), walk = vec(running_sum(e));
  std::cout << std::fixed << std::setprecision(4);

  ts::LjungBoxOptions lb;
  lb.lags = {5, 10};
  const auto q = ts::ljung_box(stationary, lb);
  const auto stat = q.stat.to_vec<double>(), p = q.pvalue.to_vec<double>();
  std::cout << "Ljung-Box on the AR(1): Q(5) = " << stat[0] << " (p = " << std::scientific << std::setprecision(2) << p[0] << std::fixed << std::setprecision(4) << "), Q(10) = " << stat[1] << " (p = "
            << std::scientific << std::setprecision(2) << p[1] << ")\n";
  lb.lags = {10};
  lb.model_df = 1;
  const auto q1 = ts::ljung_box(stationary, lb);
  std::cout << std::fixed << std::setprecision(4) << "  with one fitted parameter: " << q1.dof.to_vec<double>()[0] << " degrees of freedom\n\n";

  std::cout << std::left << std::setw(26) << "ADF (2 lags)" << std::setw(12) << "statistic" << std::setw(12) << "p-value" << std::setw(12) << "5% critical" << "decision\n";
  for (auto model : {ts::AdfModel::AR, ts::AdfModel::ARD, ts::AdfModel::TS}) {
    const char* name = model == ts::AdfModel::AR ? "no constant" : (model == ts::AdfModel::ARD ? "constant" : "constant and trend");
    for (int which = 0; which < 2; ++which) {
      ts::AdfOptions o;
      o.lags = 2;
      o.model = model;
      const auto r = ts::adf_test(which == 0 ? stationary : walk, o);
      std::cout << std::left << std::setw(26) << (std::string(name) + (which == 0 ? ", AR(1)" : ", walk")) << std::setw(12) << r.stat << std::setw(12) << r.pvalue << std::setw(12) << r.critical
                << (r.reject ? "reject unit root" : "keep unit root") << "\n";
    }
  }
  std::cout << "\n" << std::left << std::setw(26) << "KPSS (4 lags)" << std::setw(12) << "statistic" << std::setw(12) << "p-value" << std::setw(12) << "5% critical" << "decision\n";
  for (bool trend : {false, true}) {
    for (int which = 0; which < 2; ++which) {
      ts::KpssOptions o;
      o.lags = 4;
      o.trend = trend;
      const auto r = ts::kpss_test(which == 0 ? stationary : walk, o);
      std::cout << std::left << std::setw(26) << std::string(trend ? "trend, " : "level, ") + (which == 0 ? "AR(1)" : "walk") << std::setw(12) << r.stat << std::setw(12) << r.pvalue << std::setw(12) << r.critical
                << (r.reject ? "reject stationarity" : "keep stationarity") << "\n";
    }
  }
}
```

Output

```text
Ljung-Box on the AR(1): Q(5) = 200.1557 (p = 2.63e-41), Q(10) = 244.4494 (p = 7.97e-47)
  with one fitted parameter: 9.0000 degrees of freedom

ADF (2 lags)              statistic   p-value     5% critical decision
no constant, AR(1)        -4.1684     0.0000      -1.9424     reject unit root
no constant, walk         -0.1754     0.6229      -1.9424     keep unit root
constant, AR(1)           -4.2053     0.0006      -2.8763     reject unit root
constant, walk            -1.6540     0.4550      -2.8763     keep unit root
constant and trend, AR(1) -4.1844     0.0047      -3.4330     reject unit root
constant and trend, walk  -1.5970     0.7936      -3.4330     keep unit root

KPSS (4 lags)             statistic   p-value     5% critical decision
level, AR(1)              0.2421      0.1000      0.4630      keep stationarity
level, walk               0.7988      0.0100      0.4630      reject stationarity
trend, AR(1)              0.1489      0.0476      0.1460      reject stationarity
trend, walk               0.6160      0.0100      0.1460      reject stationarity
```
The Ljung–Box statistics for the AR(1) series are enormous (p-values of 10⁻⁴¹ and 10⁻⁴⁷): the series is far from white noise, as it must be. When the series is the residual of a model with `k` fitted ARMA coefficients, set `model_df = k` so the degrees of freedom are reduced (9 instead of 10 here); Chapter 2 uses this.

The ADF test on the AR(1) series is strongly rejected in all three specifications (p = 4×10⁻⁵, 6×10⁻⁴, 5×10⁻³), while for the random walk it is not (p = 0.62, 0.45, 0.79). The p-values and critical values come from MacKinnon's response surfaces, which depend on the sample size (the 5% critical value for the constant model is −2.8763 for 197 observations, against −2.86 asymptotically). The KPSS test agrees: stationarity is kept for the AR(1) series around a level (p = 0.10, the largest value the tables give) and rejected for the walk (p = 0.01, the smallest). With a trend term the AR(1) series is rejected narrowly (statistic 0.1489 against a critical value of 0.146, p = 0.048); a small sample and a borderline statistic can do that, which is why one test is not a verdict.

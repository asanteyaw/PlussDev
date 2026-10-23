<!-- generated from the_transforms.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The transforms

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
  const Series x = vec({3, 5, 4, 8, 7, 9, 12, 10});
  print_row("x", x, 0);
  print_row("diff", ts::diff(x));
  print_row("diff(2)", ts::diff(x, 2));
  print_row("returns", ts::returns(x));
  print_row("log_returns", ts::log_returns(x));
  print_row("lag", ts::lag(x), 0);
  print_row("lead", ts::lead(x), 0);
  print_row("rolling_mean(3)", ts::rolling_mean(x, 3));
  print_row("rolling_std(3)", ts::rolling_std(x, 3));
  print_row("rolling_std(3,3)", ts::rolling_std(x, 3, 3));
  print_row("rolling_std ddof0", ts::rolling_std(x, 3, 1, 0));
  print_row("rolling_max(3)", ts::rolling_max(x, 3), 0);
  print_row("ema(0.3)", ts::ema(x, 0.3));
  print_row("ewm_std(0.3)", ts::ewm_std(x, 0.3));
  print_row("expanding_mean", ts::expanding_mean(x));
  print_row("expanding_std", ts::expanding_std(x));
  print_row("zscore_rolling(3)", ts::zscore_rolling(x, 3));
}
```

Output

```text
x                3        5        4        8        7        9        12       10      
diff             nan      2.0000   -1.0000  4.0000   -1.0000  2.0000   3.0000   -2.0000 
diff(2)          nan      nan      1.0000   3.0000   3.0000   1.0000   5.0000   1.0000  
returns          nan      0.6667   -0.2000  1.0000   -0.1250  0.2857   0.3333   -0.1667 
log_returns      nan      0.5108   -0.2231  0.6931   -0.1335  0.2513   0.2877   -0.1823 
lag              nan      3        5        4        8        7        9        12      
lead             5        4        8        7        9        12       10       nan     
rolling_mean(3)  3.0000   4.0000   4.0000   5.6667   6.3333   8.0000   9.3333   10.3333 
rolling_std(3)   nan      1.4142   1.0000   2.0817   2.0817   1.0000   2.5166   1.5275  
rolling_std(3,3) nan      nan      1.0000   2.0817   2.0817   1.0000   2.5166   1.5275  
rolling_std ddof0 0.0000   1.0000   0.8165   1.6997   1.6997   0.8165   2.0548   1.2472  
rolling_max(3)   3        5        5        8        8        9        12       12      
ema(0.3)         3.0000   3.6000   3.7200   5.0040   5.6028   6.6220   8.2354   8.7648  
ewm_std(0.3)     0.0000   0.9165   0.7884   2.0693   1.9581   2.2600   3.1063   2.7218  
expanding_mean   3.0000   4.0000   4.0000   5.0000   5.4000   6.0000   6.8571   7.2500  
expanding_std    nan      1.4142   1.0000   2.1602   2.0736   2.3664   3.1320   3.1053  
zscore_rolling(3) nan      0.7071   0.0000   1.1209   0.3203   1.0000   1.0596   -0.2182 
```
Read the `NaN` pattern: `diff(2)` loses two values, a rolling statistic with `min_periods` equal to the window loses the start of the series, and a sample standard deviation is undefined for a single observation (the population one is 0). `returns` and `log_returns` agree closely for small changes and differ for large ones (0.6667 against 0.5108 for the move from 3 to 5).

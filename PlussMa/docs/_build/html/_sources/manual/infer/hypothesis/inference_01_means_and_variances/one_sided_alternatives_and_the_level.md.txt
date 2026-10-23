<!-- generated from one_sided_alternatives_and_the_level.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# One-sided alternatives and the level

[Back to Means and Variances](../inference_01_means_and_variances.md)


```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace hyp = pluss::subsp::stats::inference::hypothesis;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
Series table(std::vector<double> v, std::int64_t rows, std::int64_t cols) { return pluss::make_series(v, {rows, cols}).to(f64); }
const char* yes_no(bool b) { return b ? "reject" : "keep"; }
// two samples used throughout: cup fill volumes (cl) from two filling machines
const std::vector<double> machine_a{5.1, 4.9, 6.2, 5.8, 6.0, 5.5, 5.3, 6.1};
const std::vector<double> machine_b{4.8, 5.2, 5.0, 4.7, 5.6, 4.9, 5.1, 4.6, 5.3};
int main() {
  const Series a = vec(machine_a);
  std::cout << std::fixed << std::setprecision(4);
  std::cout << std::left << std::setw(10) << "tail" << std::setw(10) << "alpha" << std::setw(12) << "t" << std::setw(12) << "p-value" << std::setw(12) << "critical" << "decision\n";
  struct Case { const char* name; hyp::Tail tail; };
  const Case cases[] = {{"Both", hyp::Tail::Both}, {"Right", hyp::Tail::Right}, {"Left", hyp::Tail::Left}};
  for (const auto& c : cases) {
    for (double alpha : {0.05, 0.01}) {
      hyp::TestOptions o;
      o.tail = c.tail;
      o.alpha = alpha;
      const auto t = hyp::ttest(a, 5.0, o);
      std::cout << std::left << std::setw(10) << c.name << std::setw(10) << alpha << std::setw(12) << t.stat << std::setw(12) << t.pvalue << std::setw(12) << t.critical << yes_no(t.reject) << "\n";
    }
  }
}
```

Output

```text
tail      alpha     t           p-value     critical    decision
Both      0.0500    3.5696      0.0091      2.3646      reject
Both      0.0100    3.5696      0.0091      3.4995      reject
Right     0.0500    3.5696      0.0046      1.8946      reject
Right     0.0100    3.5696      0.0046      2.9980      reject
Left      0.0500    3.5696      0.9954      -1.8946     keep
Left      0.0100    3.5696      0.9954      -2.9980     keep
```
With `Tail::Right` the alternative is "the mean is larger than 5.0": the p-value is half the two-sided one when the statistic points the right way, and the critical value is smaller in absolute terms (1.89 rather than 2.36 at 5%), so the test is more powerful against that alternative. With `Tail::Left` the statistic points the wrong way: the p-value is near 1 and the test cannot reject, however large the statistic. Choose the tail before looking at the data, from the question you are asking. Lowering `alpha` from 0.05 to 0.01 raises the critical value (2.36 to 3.50) and makes the test harder to pass; the p-value does not depend on `alpha`.

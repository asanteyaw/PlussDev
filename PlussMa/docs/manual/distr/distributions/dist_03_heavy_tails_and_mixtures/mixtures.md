<!-- generated from mixtures.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Mixtures

[Back to Heavy Tails and Mixtures](../dist_03_heavy_tails_and_mixtures.md)

Some data come from a population that is not one thing but several: heights of a mixed group of adults, returns in calm and in turbulent markets, response times of fast and slow users. The histogram is then bumpy, skewed or has two humps, and no single bell-shaped family describes it. A **mixture** builds the shape from simple parts. With weights `w_j` that add to one, the density is

> `f(x) = Σ_j w_j · N(x; m_j, s_j²)`:

with probability `w_j` an observation is drawn from the `j`-th normal component. `GaussianMixture{weights, means, stddevs}` is that family (each field a `Series` with one entry per component), with all the methods of Chapter 1 (`pdf`, `cdf`, `sample`) and a fit.

`MLE(x)` fits the weights, means and standard deviations by the **expectation-maximisation** (EM) algorithm. It starts from the values stored in the object (their number is the number of components), then alternates two steps: compute for each observation the probability that each component produced it (the *responsibilities*), and re-estimate each component's parameters from the observations weighted by those probabilities. It stops when the log-likelihood no longer changes (a relative change below 10⁻¹³). Each pass can only increase the likelihood, so the algorithm climbs to a maximum, but a mixture's likelihood can have several local maxima, so *where you start can matter*.

The programs use `namespace distr = pluss::subsp::stats::distr;`, and a sample of 600 observations from a two-component mixture (weights 0.3 and 0.7, means −2.0 and 1.5, standard deviations 0.8 and 1.2).

## Fitting a mixture

The first program fits the mixture from two very different starting points and prints the estimates with their standard errors. The standard errors are for the natural parameters in the order weights, means, standard deviations, computed from the Hessian in unconstrained coordinates (the logits of the weights and the logarithms of the standard deviations) by the delta method, so that the constraint that the weights sum to one is respected.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <vector>

namespace distr = pluss::subsp::stats::distr;
namespace dst = distr::dst;
using namespace pluss::tils;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const distr::GaussianMixture truth{pluss::make_series({0.3, 0.7}, f64), pluss::make_series({-2.0, 1.5}, f64), pluss::make_series({0.8, 1.2}, f64)};
  const auto x = truth.sample({600}, f64, 77ULL);
  out.writeln("true          weights 0.3 0.7   means -2.0 1.5   standard deviations 0.8 1.2\n");

  for (const std::vector<double> start : {std::vector<double>{-1.0, 1.0}, std::vector<double>{-3.0, 3.0}}) {
    const auto fit = distr::GaussianMixture{pluss::make_series({0.5, 0.5}, f64), pluss::make_series(start, f64), pluss::make_series({1.0, 1.0}, f64)}.MLE(x);
    out.writeln("start at the means", start, PrintOptions{}.fixed(1));
    out.writeln("  weights        ", fit.weights.host<double>());
    out.writeln("  means          ", fit.means.host<double>());
    out.writeln("  stddevs        ", fit.stddevs.host<double>());
    out.writeln("  log-likelihood ", fit.loglik(x).item<double>());
  }

  const auto fit = distr::GaussianMixture{pluss::make_series({0.5, 0.5}, f64), pluss::make_series({-1.0, 1.0}, f64), pluss::make_series({1.0, 1.0}, f64)}.MLE(x);
  const auto ci = fit.confint(x).host<double>();  // one row per parameter: lower, upper
  out.writeln("\nstandard errors of (w1, w2, m1, m2, s1, s2)", fit.stderr(x).host<double>());
  out.writeln("95% interval of the first mean", ci[4], "to", ci[5]);
}
```

Output

```text
true          weights 0.3 0.7   means -2.0 1.5   standard deviations 0.8 1.2

start at the means [-1.0, 1.0]
  weights         [0.2797, 0.7203]
  means           [-2.0551, 1.4474]
  stddevs         [0.8873, 1.2661]
  log-likelihood  -1211.7021
start at the means [-3.0, 3.0]
  weights         [0.2797, 0.7203]
  means           [-2.0551, 1.4474]
  stddevs         [0.8873, 1.2661]
  log-likelihood  -1211.7021

standard errors of (w1, w2, m1, m2, s1, s2) [0.0451, 0.0451, 0.2089, 0.1389, 0.1354, 0.0945]
95% interval of the first mean -2.4645 to -1.6456
```

The two starts, which differ greatly, reach the same fit with the same log-likelihood: the maximum is found reliably when the components are well separated. The weights, means and standard deviations are within about one standard error of the generating values. The weights have *equal* standard errors for two components: they sum to one, so an error in one is the mirror image of the error in the other, and their covariance matrix is singular. That is not a defect; it is the constraint showing through.

## How many components?

The number of components is the number of entries you put in the starting object, and the data do not choose it for you. The usual approach is to fit several and compare by an information criterion: Akaike's `2k − 2 loglik` or Schwarz's `k log n − 2 loglik`, with `k = 3c − 1` parameters for `c` components. The program fits one, two, three and four components to the same two-component sample, with starting values spread over the range of the data.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <vector>

namespace distr = pluss::subsp::stats::distr;
namespace dst = distr::dst;
using namespace pluss::tils;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2));
  const distr::GaussianMixture truth{pluss::make_series({0.3, 0.7}, f64), pluss::make_series({-2.0, 1.5}, f64), pluss::make_series({0.8, 1.2}, f64)};
  const auto x = truth.sample({600}, f64, 77ULL);
  const double lo = x.min().item<double>(), hi = x.max().item<double>();
  const double n = static_cast<double>(x.numel());

  out.writeln("components  k   loglik      AIC      BIC");
  for (int c = 1; c <= 4; ++c) {
    // Spread the starting means over the range of the data.
    const auto means = pluss::series::linspace(lo + (hi - lo) * 0.5 / c, hi - (hi - lo) * 0.5 / c, c, f64);
    const auto fit = distr::GaussianMixture{pluss::series::ones({c}, f64) * (1.0 / c), means, pluss::series::ones({c}, f64)}.MLE(x);
    const double ll = fit.loglik(x).item<double>();
    const int k = 3 * c - 1;
    out.writeln(c, "  ", k, "  ", ll, "  ", 2 * k - 2 * ll, "  ", k * std::log(n) - 2 * ll);
  }
}
```

Output

```text
components  k   loglik      AIC      BIC
1    2    -1255.49    2514.98    2523.78
2    5    -1211.70    2433.40    2455.39
3    8    -1201.93    2419.87    2455.04
4    11    -1200.93    2423.85    2472.22
```

One component is far worse: the log-likelihood rises by 44 units from one component to two (−1255.5 to −1211.7), and both criteria fall decisively (AIC from 2515 to 2433, BIC from 2524 to 2455). The third component is the instructive part. It raises the log-likelihood by another 9.8 units, and Akaike's criterion prefers it (2420 against 2433) even though the data were generated by two components, while Schwarz's criterion, with its heavier penalty, calls it a tie (2455.04 against 2455.39). A fourth component is rejected by both. This is the usual behaviour of the two criteria: Akaike's is generous with parameters and will often add a component that is only bending the fit towards noise (a sample of 600 from two normals is never exactly two normals), and Schwarz's is stricter. When they disagree, prefer the smaller number, here two, and ask whether a third component *means* anything about the data before accepting it. A mixture needs many observations per component to identify it.

## Which component did each observation come from?

A fitted mixture also answers a classification question: given an observation, what is the probability that it came from each component? By Bayes' rule it is `w_j N(x; m_j, s_j²) / f(x)`, a calculation from the fitted parameters alone. This is the clustering use of a mixture. The program draws labelled data (so the truth is known), fits the mixture from an unlabelled start, classifies each observation to the more probable component, and reports the accuracy against the true labels, and against the best any method could do (the Bayes accuracy computed with the *true* parameters).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <vector>

namespace distr = pluss::subsp::stats::distr;
namespace dst = distr::dst;
using namespace pluss::tils;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const int n0 = 180, n1 = 420;  // 30% and 70%
  const auto x = distr::Normal{-2.0, 0.8}.sample({n0}, f64, 5ULL).unite({distr::Normal{1.5, 1.2}.sample({n1}, f64, 6ULL)}, 0);

  const auto fit = distr::GaussianMixture{pluss::make_series({0.5, 0.5}, f64), pluss::make_series({-1.0, 1.0}, f64), pluss::make_series({1.0, 1.0}, f64)}.MLE(x);
  const auto w = fit.weights.host<double>(), m = fit.means.host<double>(), s = fit.stddevs.host<double>();
  const int a = m[0] < m[1] ? 0 : 1, b = 1 - a;  // component order is arbitrary: "a" is the lower-mean component

  // P(first component | x) by Bayes' rule, from the fitted and from the true parameters.
  const auto fitted_a = distr::Normal{m[a], s[a]}.pdf(x) * w[a], fitted_b = distr::Normal{m[b], s[b]}.pdf(x) * w[b];
  const auto true_a = distr::Normal{-2.0, 0.8}.pdf(x) * 0.3, true_b = distr::Normal{1.5, 1.2}.pdf(x) * 0.7;
  const auto p_fit = fitted_a / (fitted_a + fitted_b), p_true = true_a / (true_a + true_b);

  const auto label = pluss::series::ones({n0}, f64).unite({pluss::series::zeros({n1}, f64)}, 0);  // 1 = first component
  const double acc_fit = ((p_fit > 0.5).to(pluss::ScalarType::Float64) - label).abs().mean().item<double>();
  const double acc_true = ((p_true > 0.5).to(pluss::ScalarType::Float64) - label).abs().mean().item<double>();
  const double unsure = ((p_fit > 0.1).to(pluss::ScalarType::Float64) * (p_fit < 0.9).to(pluss::ScalarType::Float64)).sum().item<double>();

  out.writeln("classification accuracy, fitted mixture:  ", 1.0 - acc_fit);
  out.writeln("classification accuracy, true parameters: ", 1.0 - acc_true, "  (the best any method can do)");
  out.writeln("observations whose component is uncertain (probability between 0.1 and 0.9):", unsure, "of", x.numel(), PrintOptions{}.fixed(0));

  const auto probe = pluss::make_series({-3.0, -1.5, -0.5, 0.0, 0.5, 1.5}, f64);
  const auto pa = distr::Normal{m[a], s[a]}.pdf(probe) * w[a], pb = distr::Normal{m[b], s[b]}.pdf(probe) * w[b];
  out.writeln("\nx                       ", probe.host<double>());
  out.writeln("P(first component | x)  ", (pa / (pa + pb)).host<double>());
}
```

Output

```text
classification accuracy, fitted mixture:   0.965
classification accuracy, true parameters:  0.963   (the best any method can do)
observations whose component is uncertain (probability between 0.1 and 0.9): 53 of 600

x                        [-3.000, -1.500, -0.500, 0.000, 0.500, 1.500]
P(first component | x)   [0.998, 0.944, 0.293, 0.042, 0.004, 0.000]
```

The fitted mixture classifies as well as the one built from the true parameters (96.5% against 96.3%; the fitted one is ahead by chance, since the truth is the best rule only on average). Estimation costs almost nothing here. Even the true parameters are not perfect, because the components overlap: 53 of the 600 observations have a component probability between 0.1 and 0.9, and the last table shows the posterior probability handing over from the first component to the second across the region from about −1.5 to 0, from 0.998 at −3 to 0.000 at 1.5. The *probability*, not just the winning label, is the useful output: it says which observations are classified with confidence and which are a toss-up.

## Starting values and local maxima

EM climbs to the nearest maximum, and a mixture likelihood can have more than one. Starting values that make the components identical (the same mean for both) leave the algorithm with no reason to separate them; widely different values reach the same solution. The program starts the two-component fit from four different places and prints where each ends, with its log-likelihood, to show what a good and a poor start look like.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <vector>

namespace distr = pluss::subsp::stats::distr;
namespace dst = distr::dst;
using namespace pluss::tils;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const distr::GaussianMixture truth{pluss::make_series({0.3, 0.7}, f64), pluss::make_series({-2.0, 1.5}, f64), pluss::make_series({0.8, 1.2}, f64)};
  const auto x = truth.sample({600}, f64, 77ULL);

  out.writeln("starting point                  w1      m1      m2      s1      s2   loglik");
  const struct { const char* name; std::vector<double> means, sds; } starts[] = {
      {"means -1, 1               ", {-1.0, 1.0}, {1.0, 1.0}}, {"means -3, 3               ", {-3.0, 3.0}, {1.0, 1.0}},
      {"both means at 0.5         ", {0.5, 0.5}, {1.0, 2.0}}, {"means 4, 5 (both far right)", {4.0, 5.0}, {1.0, 1.0}}};
  for (const auto& st : starts) {
    const auto fit = distr::GaussianMixture{pluss::make_series({0.5, 0.5}, f64), pluss::make_series(st.means, f64), pluss::make_series(st.sds, f64)}.MLE(x);
    const auto w = fit.weights.host<double>(), m = fit.means.host<double>(), s = fit.stddevs.host<double>();
    out.writeln(st.name, w[0], m[0], m[1], s[0], s[1], fit.loglik(x).item<double>());
  }
}
```

Output

```text
starting point                  w1      m1      m2      s1      s2   loglik
means -1, 1                0.280 -2.055 1.447 0.887 1.266 -1211.702
means -3, 3                0.280 -2.055 1.447 0.887 1.266 -1211.702
both means at 0.5          0.720 1.447 -2.055 1.266 0.887 -1211.702
means 4, 5 (both far right) 0.280 -2.055 1.447 0.887 1.266 -1211.702
```

All four starts reach the same maximum, a log-likelihood of −1211.702 with the same weights, means and standard deviations (the third comes back with the two components listed in the opposite order, which is the arbitrary labelling noted below). So for these well-separated data the likelihood has one dominant maximum and EM finds it from anywhere, including a start with both means on the same side of the data. Do not count on that in general: with overlapping components, many components or few observations, different starts can end at different maxima, and the symptom of a poor one is a *lower* log-likelihood than another start reaches, or components that are nearly identical or of tiny weight. The remedy is the recipe in the model-selection program: fit from several starts spread over the data range and keep the one with the highest log-likelihood.

## Decisions and pitfalls

- **Choose the number of components by a criterion.** One component against two, then two against three, using Akaike's or Schwarz's; prefer the smaller number when they disagree.
- **Try several starts.** Spread the starting means over the data range and keep the highest log-likelihood.
- **Components are unordered.** The fit may return them in either order; sort by the mean before comparing with the truth.
- **A mixture needs data.** Each component must be identified from its own share of the observations; with a few dozen in total the fit is unstable.
- **Beware of components that collapse.** A component with a tiny weight or standard deviation fits a few points exactly; it inflates the likelihood without describing anything.
- **Read the weights' standard errors with their constraint in mind.** They sum to one, so for two components they are equal.

That completes the distributions guide. The API reference documents every family and method.

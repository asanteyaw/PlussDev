<!-- generated from zero_coupon_bonds.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Zero-coupon bonds and yield curves

[Back to Interest Rates](../finance_07_interest_rates.md)

A zero-coupon bond pays 1 at maturity {math}`T` and nothing before. Its price today, the **discount factor** {math}`P(0,T)`, is the building block of fixed income: every coupon bond, swap and curve is a combination of discount factors. In a short-rate model the price depends on the current short rate {math}`r` and the maturity {math}`\tau=T-t`, and for the three models of the library it has a closed form that is exponential in the rate,

```{math}
P(\tau,r)=A(\tau)\,e^{-B(\tau)\,r},\qquad y(\tau)=-\frac{\ln P(\tau,r)}{\tau},
```

with {math}`y` the continuously compounded **yield** for maturity {math}`\tau`. The functions {math}`A` and {math}`B` depend on the model:

```{math}
\begin{aligned}
\textbf{Vasicek:}\quad & B=\frac{1-e^{-\kappa\tau}}{\kappa},\qquad \ln A=\Big(\theta-\frac{\sigma^2}{2\kappa^2}\Big)(B-\tau)-\frac{\sigma^2B^2}{4\kappa},\\[4pt]
\textbf{CIR:}\quad & \gamma=\sqrt{\kappa^2+2\sigma^2},\quad B=\frac{2(e^{\gamma\tau}-1)}{(\gamma+\kappa)(e^{\gamma\tau}-1)+2\gamma},\quad A=\Big[\frac{2\gamma e^{(\kappa+\gamma)\tau/2}}{(\gamma+\kappa)(e^{\gamma\tau}-1)+2\gamma}\Big]^{2\kappa\theta/\sigma^2},\\[4pt]
\textbf{Hull-White:}\quad & \ln P=-rB-\theta_0(\tau-B)-\theta_{\text{slope}}\Big(\tfrac{\tau^2}{2}-\tfrac{\tau-B}{\kappa}\Big)+\frac{\sigma^2}{2\kappa^2}(\tau-B)-\frac{\sigma^2B^2}{4\kappa},\quad B=\frac{1-e^{-\kappa\tau}}{\kappa}.
\end{aligned}
```

The Hull-White price is the Gaussian one: {math}`\ln P=-\mathbb{E}[\int_0^\tau r]+\tfrac12\operatorname{Var}[\int_0^\tau r]`, with the mean of the integral computed from the moving level. With `theta_slope = 0` it is the Vasicek price.

```cpp
double rates::vasicek_zcb(double maturity, double short_rate, const rates::VasicekParams &params);
double rates::cir_zcb(double maturity, double short_rate, const rates::CirParams &params);
double rates::hull_white_zcb(double maturity, double short_rate, const rates::HullWhiteParams &params);
double rates::hjm_zcb_mc(double maturity, const rates::HjmParams &params, std::int64_t paths = 20000, std::int64_t steps = 252, rates::SimOptions options = {}, Config cfg = {});
```

| Argument | Meaning |
|---|---|
| `maturity` | time to maturity {math}`\tau` in years; 0 gives price 1; negative is an error |
| `short_rate` | the current short rate {math}`r`; for CIR it must be at least `rate_floor` |
| `params` | the parameters of the model, as for simulation; invalid ones (for example a non-positive `kappa`) are an error naming the field |

## Program 1: discount factors and yield curves

The program prices zero-coupon bonds from three months to thirty years with the three models, all started at {math}`r_0=5\%` with a long-run level of 3%, and prints the price and the yield. It then compares the 500-year yield with the limits of the formulas: for Vasicek {math}`y(\infty)=\theta-\sigma^2/(2\kappa^2)`, for CIR {math}`y(\infty)=2\kappa\theta/(\gamma+\kappa)`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace rt = pluss::subsp::math::finance::rates;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const rt::VasicekParams v{.kappa = 0.7, .theta = 0.03, .sigma = 0.01, .r0 = 0.05};
  const rt::CirParams c{.kappa = 0.7, .theta = 0.03, .sigma = 0.08, .r0 = 0.05};
  const rt::HullWhiteParams h{.kappa = 0.7, .sigma = 0.01, .r0 = 0.05, .theta0 = 0.03, .theta_slope = 0.0};
  out.writeln("maturity   Vasicek P   y         CIR P      y         Hull-White P   y");
  for (const double T : {0.25, 1.0, 2.0, 5.0, 10.0, 30.0}) {
    const double pv = rt::vasicek_zcb(T, v.r0, v), pc = rt::cir_zcb(T, c.r0, c), ph = rt::hull_white_zcb(T, h.r0, h);
    out.writeln(T, pv, -std::log(pv) / T, pc, -std::log(pc) / T, ph, -std::log(ph) / T);
  }
  const double kappa = v.kappa, sigma = v.sigma;
  out.writeln("long yield, Vasicek: theta - sigma^2 / (2 kappa^2) =", v.theta - sigma * sigma / (2.0 * kappa * kappa), " at 500 years", -std::log(rt::vasicek_zcb(500.0, v.r0, v)) / 500.0);
  const double gamma = std::sqrt(c.kappa * c.kappa + 2.0 * c.sigma * c.sigma);
  out.writeln("long yield, CIR: 2 kappa theta / (gamma + kappa) =", 2.0 * c.kappa * c.theta / (gamma + c.kappa), " at 500 years", -std::log(rt::cir_zcb(500.0, c.r0, c)) / 500.0);
  out.writeln("maturity 0 gives", rt::vasicek_zcb(0.0, 0.05, v));
}
```

Output

```text
maturity   Vasicek P   y         CIR P      y         Hull-White P   y
0.25000 0.98799 0.04835 0.98799 0.04834 0.98799 0.04835
1.00000 0.95660 0.04437 0.95662 0.04435 0.95660 0.04437
2.00000 0.92176 0.04074 0.92184 0.04069 0.92176 0.04074
5.00000 0.83744 0.03548 0.83779 0.03540 0.83744 0.03548
10.00000 0.72055 0.03277 0.72120 0.03268 0.72055 0.03277
30.00000 0.39624 0.03086 0.39733 0.03077 0.39624 0.03086
long yield, Vasicek: theta - sigma^2 / (2 kappa^2) = 0.02990  at 500 years 0.02996
long yield, CIR: 2 kappa theta / (gamma + kappa) = 0.02981  at 500 years 0.02986
maturity 0 gives 1.00000
```

The curve is downward-sloping, from a yield of 4.84% at three months to 3.09% at thirty years: the short rate, 5%, is above its level of 3%, and reverts down. The long end is below the long-run level of 3% because of convexity: {math}`\theta-\sigma^2/(2\kappa^2)=0.0299` for Vasicek and 0.0298 for CIR, and the yield at 500 years (0.02996, 0.02986) approaches them with an error of order {math}`1/\tau`. The three models give almost the same curve here, since the volatilities are small; the Hull-White column coincides with Vasicek because the slope is zero. A maturity of 0 returns 1.

## Program 2: the closed forms against simulation

The formulas can be checked by simulating the model and averaging the discount factor along the paths, {math}`P\approx\frac1N\sum_i\exp(-\int_0^Tr^{(i)}_s\,ds)`, with the trapezoid rule for the integral. The program does this for a five-year bond under the three models with 200,000 paths of 100 steps of the `Transition` scheme, and prints each estimate, its standard error and the difference from the formula in standard errors. The Hull-White check uses a level that rises, with `theta_slope = 0.004`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace rt = pluss::subsp::math::finance::rates;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const double T = 5.0;
  const std::int64_t steps = 100, paths = 200000;
  const double dt = T / steps;
  rt::SimOptions options;
  options.scheme = rt::Scheme::Transition;
  options.seed = 17;
  // E[exp(-integral of r)] along each path, with the trapezoid rule
  const auto discount = [&](const pluss::Series &rate) {
    const auto integral = (rate.sum(0, false) - (rate.select(0, 0) + rate.select(0, steps)) * 0.5) * dt;
    return (integral * -1.0).exp();
  };
  const auto report = [&](const std::string &name, const pluss::Series &d, double closed) {
    const double mean = d.mean().item<double>(), se = d.std().item<double>() / std::sqrt(static_cast<double>(paths));
    out.writeln(name, mean, " +-", se, " closed form", closed, " difference in standard errors", (mean - closed) / se);
  };
  const rt::VasicekParams v{.kappa = 0.7, .theta = 0.03, .sigma = 0.01, .r0 = 0.05};
  report("Vasicek     ", discount(rt::vasicek_paths(steps, paths, dt, v, options, as_f64()).rate), rt::vasicek_zcb(T, v.r0, v));
  const rt::CirParams c{.kappa = 0.7, .theta = 0.03, .sigma = 0.08, .r0 = 0.05};
  report("CIR         ", discount(rt::cir_paths(steps, paths, dt, c, options, as_f64()).rate), rt::cir_zcb(T, c.r0, c));
  const rt::HullWhiteParams h{.kappa = 0.7, .sigma = 0.01, .r0 = 0.05, .theta0 = 0.03, .theta_slope = 0.004};
  report("Hull-White  ", discount(rt::hull_white_paths(steps, paths, dt, h, options, as_f64()).rate), rt::hull_white_zcb(T, h.r0, h));
}
```

Output

```text
Vasicek      0.837439  +- 0.000046  closed form 0.837438  difference in standard errors 0.024778
CIR          0.837831  +- 0.000071  closed form 0.837788  difference in standard errors 0.608886
Hull-White   0.813338  +- 0.000045  closed form 0.813220  difference in standard errors 2.643927
```

The Vasicek estimate, 0.837439, is 0.02 standard errors from the formula, 0.837438, and the CIR estimate is 0.6 standard errors from it. For Hull-White with a rising level (price 0.813220) the estimate is 0.813339, 2.7 standard errors, 1.2×10⁻⁴ away. That residual is of the order of the discretisation of the quadrature and of the moving level over steps of 0.05 years, not an error in the formula. The formula and the simulation are two independent routes to the same price, and checking one against the other is how a new model or a new scheme should be tested.

## Program 3: duration, DV01 and convexity

A bond price falls when the short rate rises. The sensitivity is the **duration** of the model, {math}`-\partial\ln P/\partial r=B(\tau)`, because {math}`\ln P=\ln A-Br`. For a zero-coupon bond this is the exact first derivative, and the second is the **convexity**, {math}`\partial^2P/\partial r^2\,/P=B^2`:

```{math}
\frac{\Delta P}{P}\;\approx\;-B\,\Delta r+\tfrac12B^2\,\Delta r^2 .
```

The `DV01` is the change in value for a one basis point rise in the rate, {math}`\partial P/\partial r\times10^{-4}=-BP\times10^{-4}`. The program computes the duration and the DV01 by bumping the short rate in the closed form, and compares both with the exact expressions; it then reprices a five-year bond after shocks of ±1 and ±2 percentage points and compares with the duration approximation and the second-order one.

Note that this duration is a sensitivity to the *short rate* and is not the maturity of the bond: with mean reversion a shock to the short rate decays, so the sensitivity is {math}`B(\tau)=(1-e^{-\kappa\tau})/\kappa`, which tends to {math}`1/\kappa=1.43` as the maturity grows, however long the bond.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace rt = pluss::subsp::math::finance::rates;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const rt::VasicekParams v{.kappa = 0.7, .theta = 0.03, .sigma = 0.01, .r0 = 0.05};
  out.writeln("maturity  B(T)       -dlnP/dr (bump)   dP/dr per basis point (DV01 of 1)   exact B P / 10000");
  for (const double T : {1.0, 5.0, 10.0}) {
    const double h = 1e-5;
    const double p = rt::vasicek_zcb(T, v.r0, v);
    const double duration = -(std::log(rt::vasicek_zcb(T, v.r0 + h, v)) - std::log(rt::vasicek_zcb(T, v.r0 - h, v))) / (2.0 * h);
    const double dv01 = (rt::vasicek_zcb(T, v.r0 + 1e-4, v) - rt::vasicek_zcb(T, v.r0 - 1e-4, v)) / 2.0;
    const double b = (1.0 - std::exp(-v.kappa * T)) / v.kappa;
    out.writeln(T, b, duration, dv01, -b * p / 1.0e4);
  }
  const double T = 5.0, p0 = rt::vasicek_zcb(T, v.r0, v), b = (1.0 - std::exp(-v.kappa * T)) / v.kappa;
  out.writeln("shock    exact repricing   duration only   duration + convexity");
  for (const double shock : {-0.02, -0.01, 0.01, 0.02}) {
    const double exact = rt::vasicek_zcb(T, v.r0 + shock, v);
    out.writeln(shock, exact / p0 - 1.0, -b * shock, -b * shock + 0.5 * b * b * shock * shock);
  }
}
```

Output

```text
maturity  B(T)       -dlnP/dr (bump)   dP/dr per basis point (DV01 of 1)   exact B P / 10000
1.000000 0.719164 0.719164 -0.000069 -0.000069
5.000000 1.385432 1.385432 -0.000116 -0.000116
10.000000 1.427269 1.427269 -0.000103 -0.000103
shock    exact repricing   duration only   duration + convexity
-0.020000 0.028096 0.027709 0.028093
-0.010000 0.013951 0.013854 0.013950
0.010000 -0.013759 -0.013854 -0.013758
0.020000 -0.027328 -0.027709 -0.027325
```

The bumped duration equals {math}`B(\tau)` to the six digits shown (0.719, 1.385, 1.427 for one, five and ten years), and the DV01 equals the exact {math}`-BP\times10^{-4}`: for the five-year bond one basis point costs 0.000116 per unit of face value. Duration grows with maturity but saturates near {math}`1/\kappa=1.43`, which is the effect of mean reversion. For the shocks, the first-order estimate is good to a few per cent of the move (for +2 points, −2.771% against −2.733%), and adding the convexity term brings it to within 0.0003 percentage points of the exact repricing (−2.7325% against −2.7328%).

## Program 4: a coupon bond, and its value in a year

A coupon bond is a portfolio of zero-coupon bonds, one per payment, so its price is {math}`\sum_i c_iP(t_i)+F\,P(T)`. The program prices a five-year bond with a 4% annual coupon and a face value of 100. It then looks one year ahead: it simulates the short rate over the year, and in each scenario reprices the four payments that remain with the closed form at the simulated rate, adding the coupon just paid. This is the standard way to turn a rate model into a distribution of portfolio values: simulate the risk factor to the horizon, then revalue in closed form, with no nested simulation. From the 100,000 values it computes the mean, the 5% quantile (the one-year 95% **value at risk**) and the average of the 5% worst outcomes (the **expected shortfall**).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace rt = pluss::subsp::math::finance::rates;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const rt::VasicekParams v{.kappa = 0.7, .theta = 0.03, .sigma = 0.01, .r0 = 0.05};
  const double coupon = 4.0, face = 100.0;
  const std::vector<double> times{1.0, 2.0, 3.0, 4.0, 5.0};

  const auto value_at = [&](double now, double short_rate) {  // value of the coupons still to come, at time `now`
    double value = 0.0;
    for (const double t : times) {
      if (t > now) {
        value += (coupon + (t == times.back() ? face : 0.0)) * rt::vasicek_zcb(t - now, short_rate, v);
      }
    }
    return value;
  };
  out.writeln("price today of the 5-year 4% annual bond", value_at(0.0, v.r0));

  // one year from now: simulate the short rate, reprice the bond that remains, add the coupon just paid
  const std::int64_t steps = 52, paths = 100000;
  rt::SimOptions options;
  options.scheme = rt::Scheme::Transition;
  options.seed = 4;
  const auto rate_in_a_year = rt::vasicek_paths(steps, paths, 1.0 / steps, v, options, as_f64()).rate.select(0, steps).items<double>();
  std::vector<double> value(rate_in_a_year.size());
  for (std::size_t i = 0; i < value.size(); ++i) {
    value[i] = coupon + value_at(1.0, rate_in_a_year[i]);
  }
  std::sort(value.begin(), value.end());
  double mean = 0.0;
  for (const double x : value) mean += x;
  mean /= static_cast<double>(value.size());
  const double today = value_at(0.0, v.r0), quantile = value[value.size() / 20];
  double tail = 0.0;
  for (std::size_t i = 0; i < value.size() / 20; ++i) tail += value[i];
  tail /= static_cast<double>(value.size() / 20);
  out.writeln("value in a year (with the coupon): mean", mean, " standard deviation", std::sqrt(std::inner_product(value.begin(), value.end(), value.begin(), 0.0) / static_cast<double>(value.size()) - mean * mean));
  out.writeln("5% quantile", quantile, " average of the worst 5%", tail, " lowest", value.front(), " highest", value.back());
  out.writeln("one-year 95% value at risk, against today's value", today - quantile, " against the expected value", mean - quantile);
  out.writeln("expected shortfall against the expected value", mean - tail);
}
```

Output

```text
price today of the 5-year 4% annual bond 101.6270
value in a year (with the coupon): mean 106.2400  standard deviation 0.9766
5% quantile 104.6484  average of the worst 5% 104.2446  lowest 102.2620  highest 110.7768
one-year 95% value at risk, against today's value -3.0213  against the expected value 1.5916
expected shortfall against the expected value 1.9954
```

The bond is worth 101.627 today and 106.24 on average in a year, including the coupon. Its standard deviation is 0.98. The expected value rises above today's because the coupon is received and because the model expects the short rate to fall from 5% towards 3%, which raises bond prices; that is why the 95% worst case, 104.65, is still above today's value and the value at risk against today's value, −3.02, is a gain. A loss figure is therefore measured against the expected value: the 95% value at risk is 1.59 and the expected shortfall 2.00, the loss, below the mean, in the worst 5% and the average of the worst 5% of outcomes respectively. The 100,000 outcomes range from 102.26 to 110.78.

## Checklist

1. Check a closed-form price against a simulation of the same model, in standard errors, before using either.
2. Quote a yield as {math}`-\ln P/\tau`; this is continuously compounded, so convert it before comparing with a quoted annual rate.
3. Remember that the model duration saturates at {math}`1/\kappa`; it is the sensitivity to the short rate, not the maturity.
4. For a risk distribution, simulate the factor to the horizon and revalue in closed form.
5. Measure a loss against the expected value when the position accrues, and say which you report.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="short_rate_models.html">
    <span class="label">Previous</span>
    <span class="title">Short-rate models</span>
  </a>
  <a class="next-link" href="forward_rate_models.html">
    <span class="label">Next</span>
    <span class="title">Forward-rate models</span>
  </a>
</div>
<!-- apiref:nav:end -->

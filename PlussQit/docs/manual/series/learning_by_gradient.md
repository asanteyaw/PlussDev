<!-- generated from learning_by_gradient.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Learning by gradient

[Back to Series by example](../series.md)

Many problems have no closed-form answer but a loss you can differentiate. This page fits a line by gradient descent, letting the library compute the gradient, and checks the result against the exact least-squares solution.

## The mental model

A `Series` can **track slope**: after `tracks_slope_(true)`, every operation built from it is recorded. Calling `reverse()` on the final scalar walks the record backwards and stores, on each tracked series, its **sensitivity**: how much the final value changes per unit change of that series. For a loss and its parameters that sensitivity is the gradient. A training loop is then four lines: track, forward, reverse, step. The updated parameters start a fresh record on the next pass.

## Fitting a line

One hundred points from `y = 4 - 1.5 x` plus noise. The parameters start at zero; each pass computes the mean squared error, its gradient, and a step of a tenth of the gradient.

```cpp
#include <pluss/pluss.h>

#include <iostream>

int main() {
  using pluss::ScalarType;
  // Data from a known line y = 4 - 1.5 x plus a little noise.
  const std::int64_t n = 100;
  auto x = pluss::series::linspace(-2.0, 2.0, n, ScalarType::Float64);
  auto y = x.mul(-1.5).add(4.0).add(pluss::series::randn({n}, ScalarType::Float64, 3).mul(0.2));

  auto w = pluss::make_series({0.0, 0.0});   // parameters: intercept, slope
  const double learning_rate = 0.1;
  for (int step = 0; step <= 100; ++step) {
    // 1. Track the parameters so the loss can be differentiated with respect to them.
    auto param = w.to(ScalarType::Float64);
    param.tracks_slope_(true);
    // 2. Forward pass and mean squared error.
    auto prediction = param.select(0, 0).add(param.select(0, 1).mul(x));
    auto loss = prediction.sub(y).square().mean();
    // 3. Reverse pass: the sensitivity of the loss to each parameter is its gradient.
    loss.reverse();
    auto gradient = param.sensitivity();
    if (step % 20 == 0)
      std::cout << "step " << step << "  loss " << loss.item<double>() << "  intercept " << param.select(0, 0).item<double>()
                << "  slope " << param.select(0, 1).item<double>() << "\n";
    // 4. Step against the gradient; the new parameters start a fresh graph on the next pass.
    w = param.detach().sub(gradient.mul(learning_rate));
  }
  auto closed = x.unsqueeze(1);
  std::cout << "least-squares answer for comparison: ";
  auto design = pluss::series::ones({n, 1}, ScalarType::Float64).unite({closed}, 1);
  auto beta = design.t().matmul(design).linearalg().inverse().matmul(design.t().matmul(y.unsqueeze(1)));
  auto coefficients = beta.squeeze(1);
  std::cout << coefficients.select(0, 0).item<double>() << ", " << coefficients.select(0, 1).item<double>() << "\n";
}
```

Output

```text
step 0  loss 19.0113  intercept 0  slope 0
step 20  loss 0.0390125  intercept 3.95463  slope -1.47464
step 40  loss 0.0368762  intercept 4.00023  slope -1.47722
step 60  loss 0.0368759  intercept 4.00075  slope -1.47722
step 80  loss 0.0368759  intercept 4.00076  slope -1.47722
step 100  loss 0.0368759  intercept 4.00076  slope -1.47722
least-squares answer for comparison: 4.00076, -1.47722
```
The loss falls from 19.0 to 0.039 in twenty steps and then settles at `0.0369`, the noise floor. The parameters reach an intercept of `4.0008` and a slope of `-1.4772`, which is exactly what the normal equations give on the last line: for this convex problem gradient descent and least squares agree, so the check is a real test of the gradient. The slope is `-1.477`, not `-1.5`, because the data contain noise.

## Optimising a portfolio

The same four steps optimise any objective you can write as `Series` arithmetic. Here the unknowns are the weights of four assets, the objective is expected return minus a penalty on variance, and the constraint that weights are positive and sum to one is built in: the program optimises unconstrained numbers `theta` and uses `softmax(theta)` as the weights, so every step is valid by construction.

```cpp
#include <pluss/pluss.h>

#include <iostream>

static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  using pluss::ScalarType;
  // Four assets: expected annual return and the covariance of returns.
  auto mu = pluss::make_series({0.06, 0.09, 0.12, 0.04});
  auto sigma = pluss::make_series({{0.0400, 0.0060, 0.0120, 0.0000},
                                   {0.0060, 0.0900, 0.0300, 0.0000},
                                   {0.0120, 0.0300, 0.1600, 0.0000},
                                   {0.0000, 0.0000, 0.0000, 0.0025}});
  const double risk_aversion = 3.0;

  // Parameters theta are unconstrained; the weights w = softmax(theta) are positive and sum to one.
  auto theta = pluss::series::zeros({4}, ScalarType::Float64);
  const double learning_rate = 5.0;
  for (int step = 0; step <= 2000; ++step) {
    auto param = theta.clone();
    param.tracks_slope_(true);
    auto w = param.softmax(0);
    // Maximise return minus a penalty on variance: minimise the negative.
    auto expected = w.mul(mu).sum();
    auto variance = w.unsqueeze(0).matmul(sigma).matmul(w.unsqueeze(1)).squeeze(1).squeeze(0);
    auto loss = variance.mul(0.5 * risk_aversion).sub(expected);
    loss.reverse();
    if (step % 400 == 0)
      std::cout << "step " << step << "  objective " << -loss.item<double>() << "  return " << expected.item<double>() << "  variance " << variance.item<double>() << "\n";
    theta = param.detach().sub(param.sensitivity().mul(learning_rate));
  }
  auto weights = theta.softmax(0);
  show("optimal weights", weights);
  std::cout << "weights sum to " << weights.sum().item<double>() << "\n";

  // Check: at a constrained optimum the marginal utility mu - lambda Sigma w is equal across every asset that is held.
  auto marginal = mu.sub(sigma.matmul(weights.unsqueeze(1)).squeeze(1).mul(risk_aversion));
  show("marginal utility per asset", marginal);
}
```

Output

```text
step 0  objective 0.0410781  return 0.0775  variance 0.0242813
step 400  objective 0.0483949  return 0.0611167  variance 0.00848124
step 800  objective 0.0483949  return 0.0611152  variance 0.00848024
step 1200  objective 0.0483949  return 0.0611152  variance 0.00848023
step 1600  objective 0.0483949  return 0.0611152  variance 0.00848023
step 2000  objective 0.0483949  return 0.0611152  variance 0.00848023
optimal weights: Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.1395, 0.1460, 0.1379, 0.5767]
weights sum to 1
marginal utility per asset: Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.0357, 0.0357, 0.0357, 0.0357]
```
The objective rises from `0.0411` to `0.0484` and stops changing: the optimiser has converged. Return falls from 7.75 % to 6.11 % while the variance falls from `0.0243` to `0.0085`, the trade the risk aversion asked for, and the low-risk fourth asset takes 58 % of the weights. The last lines are an independent check that does not use the gradient at all. At an optimum with every asset held, the marginal utility `mu - lambda Sigma w` is the same for all of them, and it is, to four decimals, `0.0357` for each. A property like this, derived from the problem rather than from the optimiser, is the right way to test an optimisation.

## Fitting a nonlinear curve with Adam

Plain gradient descent needs a learning rate tuned to the problem. Adam adapts a step size to each parameter from running averages of the gradient and of its square, which makes it far less sensitive to the scale of the parameters; here the three parameters of a logistic growth curve differ by two orders of magnitude (`K` near 50, `r` near 0.8). The optimiser is a few lines of `Series` arithmetic, with the moments `m` and `v` held in ordinary `Series` between steps.

```cpp
#include <pluss/pluss.h>

#include <cmath>
#include <iostream>

static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  using pluss::ScalarType;
  // Observations of a logistic curve  y = K / (1 + exp(-r (t - t0)))  with K = 50, r = 0.8, t0 = 6 and a little noise.
  const std::int64_t n = 40;
  auto t = pluss::series::linspace(0.0, 12.0, n, ScalarType::Float64);
  auto truth = t.sub(6.0).mul(-0.8).exp().add(1.0).reciprocal().mul(50.0);
  auto y = truth.add(pluss::series::randn({n}, ScalarType::Float64, 31).mul(1.0));

  // Parameters (K, r, t0) start away from the answer. Adam keeps a running mean of the gradient and of its square.
  auto p = pluss::make_series({30.0, 0.3, 4.0});
  auto m = pluss::series::zeros({3}, ScalarType::Float64);
  auto v = pluss::series::zeros({3}, ScalarType::Float64);
  const double lr = 0.1, beta1 = 0.9, beta2 = 0.999, eps = 1e-8;
  for (int step = 1; step <= 600; ++step) {
    auto param = p.clone();
    param.tracks_slope_(true);
    auto curve = t.sub(param.select(0, 2)).mul(param.select(0, 1).neg()).exp().add(1.0).reciprocal().mul(param.select(0, 0));
    auto loss = curve.sub(y).square().mean();
    loss.reverse();
    auto g = param.sensitivity();
    m = m.mul(beta1).add(g.mul(1.0 - beta1));
    v = v.mul(beta2).add(g.square().mul(1.0 - beta2));
    auto m_hat = m.div(1.0 - std::pow(beta1, step));
    auto v_hat = v.div(1.0 - std::pow(beta2, step));
    p = param.detach().sub(m_hat.div(v_hat.sqrt().add(eps)).mul(lr));
    if (step == 1 || step % 100 == 0) std::cout << "step " << step << "  loss " << loss.item<double>() << "\n";
  }
  show("fitted (K, r, t0)", p);
  std::cout << "true   (K, r, t0): 50, 0.8, 6\n";
  auto fitted = t.sub(p.select(0, 2)).mul(p.select(0, 1).neg()).exp().add(1.0).reciprocal().mul(p.select(0, 0));
  std::cout << "residual standard deviation " << fitted.sub(y).std().item<double>() << " (noise 1.0)\n";
}
```

Output

```text
step 1  loss 215.677
step 100  loss 31.4532
step 200  loss 7.9661
step 300  loss 2.40672
step 400  loss 1.13522
step 500  loss 0.853606
step 600  loss 0.796082
fitted (K, r, t0): Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[49.8513, 0.8051, 5.9696]
true   (K, r, t0): 50, 0.8, 6
residual standard deviation 0.902904 (noise 1.0)
```
The loss falls from `216` to `0.80` and the fitted parameters, `K = 49.85`, `r = 0.805` and `t0 = 5.970`, recover the true `50`, `0.8` and `6` that generated the data. The residual standard deviation of `0.90` is the noise level of `1.0` seen through a fit that used some of the freedom of the data. Note how little code changed from the straight-line fit: only the forward expression and the update rule.

## Decisions

- **Learning rate:** too large diverges, too small crawls; a tenth suits a problem whose features are of order one. Standardise inputs first (see [Cleaning data](cleaning_data.md)).
- **When to stop:** stop when the loss stops changing; the loop above prints every twenty steps so you can see it.
- **Fresh record per pass:** `detach()` the updated parameters so the history of earlier steps is not carried along.
- **Closed form available:** use it, and use gradient descent as the check or when the model is not linear.
- **Constraints:** build them into the parametrisation (`softmax` for weights that sum to one, `exp` for positive values) so that no step can leave the feasible set.
- **Plain gradient or Adam:** plain gradient descent with a standardised problem; Adam when parameters live on different scales.
- **Check an optimum:** test a property the answer must have, such as equal marginal utilities or a vanishing gradient, not only that the loss stopped falling.

## Where next

Reference pages: [`tracks_slope_`](../../sphinx/api/series/tracks_slope_.md), [`reverse`](../../sphinx/api/series/reverse.md), [`sensitivity`](../../sphinx/api/series/sensitivity.md), [`detach`](../../sphinx/api/series/detach.md), [`square`](../../sphinx/api/series/square.md), [`mean`](../../sphinx/api/series/mean.md), [`select`](../../sphinx/api/series/select.md), [`softmax`](../../sphinx/api/series/softmax.md), [`matmul`](../../sphinx/api/series-linearalg/matmul.md), [`sqrt`](../../sphinx/api/series/sqrt.md). Back to [Series by example](../series.md).

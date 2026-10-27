<!-- generated from several_sources_of_noise.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Several sources of noise

[Back to Differentials and Ito's Lemma](../stochastic_02_ito_lemma.md)


With `m` independent Wiener processes, `dX = μ dt + Σ σ_k dW_k`. Pass `sigma` with a trailing axis of length `m`. The second-order term becomes `½ g'' Σ σ_k²` and the diffusion has one column per source.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const pluss::Series x = pluss::make_series(std::vector<double>{0.5, 1.0, -0.3}, {3}).to(f64);
  const pluss::Series mu = pluss::make_series(std::vector<double>{0.1, 0.2, -0.1}, {3}).to(f64);
  // two factors: row i is (sigma_i1, sigma_i2)
  const pluss::Series sigma = pluss::make_series(std::vector<double>{0.3, 0.1, 0.5, 0.2, 0.2, 0.4}, {3, 2}).to(f64);

  const auto d = x.calc.diff.ito([](const pluss::Series& s) { return s.mul(s); }, mu, sigma);
  const auto drift = d.drift.host<double>();
  const auto diffusion = d.diffusion.host<double>();
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "diffusion has shape [" << d.diffusion.size(0) << ", " << d.diffusion.size(1) << "]\n";
  const std::vector<double> xv = {0.5, 1.0, -0.3}, mv = {0.1, 0.2, -0.1}, sg = {0.3, 0.1, 0.5, 0.2, 0.2, 0.4};
  for (int i = 0; i < 3; ++i) {
    const double variance = sg[2 * i] * sg[2 * i] + sg[2 * i + 1] * sg[2 * i + 1];
    std::cout << "state " << i << ": drift " << drift[i] << " = 2 x mu + (sigma_1^2 + sigma_2^2) = "
              << 2 * xv[i] * mv[i] + variance << ";  diffusion (" << diffusion[2 * i] << ", " << diffusion[2 * i + 1]
              << ") = 2 x sigma = (" << 2 * xv[i] * sg[2 * i] << ", " << 2 * xv[i] * sg[2 * i + 1] << ")\n";
  }
  return 0;
}
```

Output

```text
diffusion has shape [3, 2]
state 0: drift 0.2000 = 2 x mu + (sigma_1^2 + sigma_2^2) = 0.2000;  diffusion (0.3000, 0.1000) = 2 x sigma = (0.3000, 0.1000)
state 1: drift 0.6900 = 2 x mu + (sigma_1^2 + sigma_2^2) = 0.6900;  diffusion (1.0000, 0.4000) = 2 x sigma = (1.0000, 0.4000)
state 2: drift 0.2600 = 2 x mu + (sigma_1^2 + sigma_2^2) = 0.2600;  diffusion (-0.1200, -0.2400) = 2 x sigma = (-0.1200, -0.2400)
```
The diffusion has shape `[3, 2]`: three states, two sources. For `g = x²` each row matches `2xμ + σ₁² + σ₂²` for the drift and `2xσ_k` for the diffusion, to the printed digits.

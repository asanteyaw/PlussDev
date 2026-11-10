<!-- generated from fourier.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Fourier series and transforms

Expansions of a function in trigonometric functions: half-range cosine and sine series on an interval, full Fourier series on a period, and the Fourier, cosine and sine transforms on the real line. Every coefficient is a definite integral built through the integral operator, so the results are analytic functions waiting for values.

## The Surface

| Symbol | What it is |
|---|---|
| [`coefficients`](fourier/coefficients.md) | The full trigonometric Fourier coefficients of a function on a symmetric period `[−L, L]`. |
| [`cosine_coefficients`](fourier/cosine_coefficients.md) | The cosine-series coefficients `A₀ … A_{terms−1}` of a function on an interval, as a vector function. |
| [`cosine_series`](fourier/cosine_series.md) | The truncated half-range cosine series of a function, as a function of the original variable. |
| [`cosine_transform`](fourier/cosine_transform.md) | The Fourier cosine transform over the half line `[0, ∞)`. |
| [`inverse_transform`](fourier/inverse_transform.md) | The inverse Fourier transform: recovers a function of the original variable from its transform. |
| [`series`](fourier/series.md) | The truncated full Fourier series of a function on a symmetric period `[−L, L]`, as a function of the original variable. |
| [`sine_coefficients`](fourier/sine_coefficients.md) | The sine-series coefficients `B₁ … B_terms` of a function on an interval, as a vector function. |
| [`sine_series`](fourier/sine_series.md) | The truncated half-range sine series of a function, as a function of the original variable. |
| [`sine_transform`](fourier/sine_transform.md) | The Fourier sine transform over the half line `[0, ∞)`. |
| [`transform`](fourier/transform.md) | The Fourier transform of a function over the real line, as a function of a new frequency variable. |

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

fourier/coefficients
fourier/cosine_coefficients
fourier/cosine_series
fourier/cosine_transform
fourier/inverse_transform
fourier/series
fourier/sine_coefficients
fourier/sine_series
fourier/sine_transform
fourier/transform
```
<!-- apiref:toctree:end -->

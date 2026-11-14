<!-- generated from convolution_and_correlation.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Convolution and correlation

[Back to Signals 1: Windows and Convolution](../signal_01_windows_and_convolution.md)

You want to apply a short set of weights along a signal: smooth it with a moving average, take a difference, apply a finite-impulse-response filter, or find where one signal best matches another. All of these are a sliding weighted sum. `conv` computes it with the kernel reversed, `corr` computes it without reversing (and conjugates complex kernels), and `fir` is `conv` cut to the length of the input so it can be used as a causal filter.

## The mental model

For an input `x` of length `N` and a kernel `h` of length `K`, the **full** convolution has length `N + K - 1` and entry

```{math}
y[n] = \sum_{k} h[k]\,x[n-k],
```

with samples outside the input taken as zero. The kernel is *flipped* as it slides, which is what makes convolution commutative and makes the output of a filter equal to its impulse response when the input is a single 1.

Correlation slides the kernel *without* flipping. It is convolution with the kernel reversed and conjugated, and it measures how well the kernel matches the signal at each shift. The `Same` and `Valid` modes are crops of the full result:

| Mode | Length | What it keeps |
|---|---|---|
| `Full` | `N + K - 1` | every shift at which the kernel and the input overlap at all |
| `Same` | `N` | the central `N` entries, aligned with the input |
| `Valid` | `max(N, K) - min(N, K) + 1` | only shifts where the shorter sequence lies entirely inside the longer, so no zero padding is used |

The sample axis is the last axis unless you pass `dim`. All other axes are batch axes: each lane is processed independently, and a shared kernel is applied to every lane.

## The three modes

The first program convolves `x = [1, 2, 3, 4, 5]` with the difference kernel `h = [1, 0, -1]` in each mode. The kernel computes `x[n] - x[n-2]`, so the interior entries are all `2`, and the ends show the zero padding of `Full`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace sig = pluss::subsp::eng::signal;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series x = pluss::series::make(std::vector<double>{1, 2, 3, 4, 5});
  const pluss::Series h = pluss::series::make(std::vector<double>{1, 0, -1});
  out.writeln("full ", sig::conv(x, h).items<double>());
  out.writeln("same ", sig::conv(x, h, -1, sig::ConvMode::Same).items<double>());
  out.writeln("valid", sig::conv(x, h, -1, sig::ConvMode::Valid).items<double>());
  out.writeln("arguments swapped, full ", sig::conv(h, x).items<double>());
  out.writeln("arguments swapped, valid", sig::conv(h, x, -1, sig::ConvMode::Valid).items<double>());
}
```

Output

```text
full  [1.0000, 2.0000, 2.0000, 2.0000, 2.0000, -4.0000, -5.0000]
same  [2.0000, 2.0000, 2.0000, 2.0000, -4.0000]
valid [2.0000, 2.0000, 2.0000]
arguments swapped, full  [1.0000, 2.0000, 2.0000, 2.0000, 2.0000, -4.0000, -5.0000]
arguments swapped, valid [2.0000, 2.0000, 2.0000]
```

`Full` has `5 + 3 - 1 = 7` entries; the first, `1`, is `x[0] h[0]` and the last, `-5`, is `x[4] h[2]`. `Same` keeps entries 1 to 5 of the full result, the five that line up with the input (for an even kernel length the crop starts at `(K - 1) / 2`, rounded down). `Valid` keeps entries 2 to 4: the three positions where the kernel fits entirely inside the input. Swapping the arguments changes nothing, because convolution is commutative, and `Valid` is symmetric too: it uses the shorter operand as the kernel automatically.

## Smoothing a step

A moving average is convolution with `K` equal weights summing to one. Applied to a step from 0 to 1, it turns the jump into a ramp as long as the kernel. `Full` shows the whole response, including the build-up and the run-down at the ends; `Same` is aligned with the input, which is what you want when the result is plotted against the same time axis.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace sig = pluss::subsp::eng::signal;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series step = pluss::series::make(std::vector<double>{0, 0, 0, 1, 1, 1, 1, 1});
  const pluss::Series average = pluss::series::make(std::vector<double>{1.0 / 3, 1.0 / 3, 1.0 / 3});
  out.writeln("full", sig::conv(step, average).items<double>());
  out.writeln("same", sig::conv(step, average, -1, sig::ConvMode::Same).items<double>());
}
```

Output

```text
full [0.0000, 0.0000, 0.0000, 0.3333, 0.6667, 1.0000, 1.0000, 1.0000, 0.6667, 0.3333]
same [0.0000, 0.0000, 0.3333, 0.6667, 1.0000, 1.0000, 1.0000, 0.6667]
```

The step rises over three samples, `1/3`, `2/3`, `1`, the length of the kernel. In the `Same` result the ramp begins one sample earlier than in `Full` because the crop removes the first `(K - 1) / 2 = 1` entry, which centres the smoothing on the sample it describes.

## Correlation

`corr` slides the kernel across the input without flipping it. For real signals it equals `conv` with the kernel reversed; for complex signals it also conjugates the kernel, which is what makes `corr(x, x)` at zero shift equal the energy of `x` instead of a complex number. The first program shows the real identity; the second shows what the conjugation does for complex data, next to the same expression with the flip but without the conjugate.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>

using namespace pluss::tils;
namespace sig = pluss::subsp::eng::signal;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series a = pluss::series::make(std::vector<double>{1, 2, 3});
  const pluss::Series b = pluss::series::make(std::vector<double>{0, 1, 0.5});
  out.writeln("corr(a, b)          ", sig::corr(a, b).items<double>());
  out.writeln("conv(a, flip(b))    ", sig::conv(a, b.flip(0)).items<double>());

  using C = std::complex<double>;
  const pluss::Series ca = pluss::make_series(std::vector<C>{{1, 1}, {0, 2}}, {2});
  const pluss::Series cb = pluss::make_series(std::vector<C>{{0, 1}, {1, 0}}, {2});
  out.writeln("complex corr(ca, cb)             ", sig::corr(ca, cb).items<C>());
  out.writeln("complex conv(ca, flip(cb))       ", sig::conv(ca, cb.flip(0)).items<C>());
}
```

Output

```text
corr(a, b)           [0.5000, 2.0000, 3.5000, 3.0000, 0.0000]
conv(a, flip(b))     [0.5000, 2.0000, 3.5000, 3.0000, 0.0000]
complex corr(ca, cb)              [(1,1), (1,1), (2,0)]
complex conv(ca, flip(cb))        [(1,1), (-1,3), (-2,0)]
```

For real data the two lines are identical. For complex data only the first line is a correlation: the entries differ from the second line in the last two positions. The last entry of `corr`, `2`, is `ca[1] * conj(cb[0])` = `2i * (-i)` = `2`: the conjugate turns the product of two imaginary numbers into a real one, which is how a signal correlated with itself gets a real, positive value at zero shift.

## Finding a delay

Cross-correlation of a received signal with a known pulse peaks where the pulse sits. The program builds a short pulse `[0, 1, 2, 1, 0, ...]` and a copy delayed by 4 samples, correlates, and finds the peak. In the full correlation of length `2N - 1`, zero shift falls at index `N - 1`, so the delay is `peak index - (N - 1)`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace sig = pluss::subsp::eng::signal;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series pulse = pluss::series::make(std::vector<double>{0, 1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0});
  const pluss::Series delayed = pluss::series::make(std::vector<double>{0, 0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0});
  const std::vector<double> r = sig::corr(delayed, pulse).items<double>();
  std::size_t best = 0;
  for (std::size_t i = 1; i < r.size(); ++i) {
    if (r[i] > r[best]) best = i;
  }
  out.writeln("correlation length", r.size(), " peak value", r[best], " peak index", best);
  out.writeln("estimated delay", static_cast<long>(best) - static_cast<long>(pulse.size(0)) + 1);
}
```

Output

```text
correlation length 23  peak value 6.0000  peak index 15
estimated delay 4
```

The peak is `1 + 4 + 1 = 6`, the energy of the pulse, at index 15, and `15 - 11 = 4` is the delay. If the received signal were delayed by a negative amount, the peak would lie before index `N - 1`. The estimate is whole samples; to resolve a fraction of a sample, interpolate around the peak.

## Batches and the sample axis

Anything that is not the sample axis is a batch axis. A one-dimensional kernel is applied to every lane. A kernel with the same rank as the input is applied lane by lane, and a batch axis of size 1 in either operand is broadcast. `dim` chooses the sample axis, so a signal stored with time down the rows is processed in place, with no transpose. The program uses a `3 x 4` batch.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace sig = pluss::subsp::eng::signal;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series batch = pluss::series::make(std::vector<double>{1, 2, 3, 4, 10, 20, 30, 40, 0, 0, 1, 0}).reshape({3, 4});

  const pluss::Series pair = pluss::series::make(std::vector<double>{1, 1});
  const pluss::Series shared = sig::conv(batch, pair);
  out.writeln("one kernel for every lane: shape", shared.sizes(), " lane 1", shared.select(0, 1).items<double>());

  const pluss::Series per_lane = pluss::series::make(std::vector<double>{1, 0, 0, 1, 0, 0}).reshape({3, 2});
  const pluss::Series own = sig::conv(batch, per_lane);
  out.writeln("a kernel per lane:         shape", own.sizes(), " lane 1", own.select(0, 1).items<double>());

  const pluss::Series by_rows = batch.transpose(0, 1);
  const pluss::Series difference = pluss::series::make(std::vector<double>{1, -1});
  const pluss::Series along_rows = sig::conv(by_rows, difference, 0);
  out.writeln("time down the rows (dim 0): shape", along_rows.sizes(), " lane 1", along_rows.select(1, 1).items<double>());
}
```

Output

```text
one kernel for every lane: shape [3, 5]  lane 1 [10.0000, 30.0000, 50.0000, 70.0000, 40.0000]
a kernel per lane:         shape [3, 5]  lane 1 [0.0000, 10.0000, 20.0000, 30.0000, 40.0000]
time down the rows (dim 0): shape [5, 3]  lane 1 [10.0000, 10.0000, 10.0000, 10.0000, -40.0000]
```

The shared kernel `[1, 1]` adds neighbouring samples, so lane 1, `[10, 20, 30, 40]`, becomes `[10, 30, 50, 70, 40]`. The per-lane kernels are `[1, 0]` for lane 0, `[0, 1]` for lane 1 (a one-sample delay: `[0, 10, 20, 30, 40]`) and `[0, 0]` for lane 2. With `dim = 0` the same lane read down the rows gives the first difference with the boundary terms `10` and `-40`.

## FIR filtering

An FIR filter is convolution with its coefficients, but the output of a filter has the same length as its input, and sample `n` depends only on input samples up to `n` (it is **causal**). `fir` is exactly that: the first `N` entries of the full convolution.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace sig = pluss::subsp::eng::signal;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series x = pluss::series::make(std::vector<double>{1, 2, 3, 4, 5});
  const pluss::Series taps = pluss::series::make(std::vector<double>{0.5, 0.5});
  out.writeln("fir  ", sig::fir(x, taps).items<double>());
  out.writeln("conv ", sig::conv(x, taps).items<double>());
}
```

Output

```text
fir   [0.5000, 1.5000, 2.5000, 3.5000, 4.5000]
conv  [0.5000, 1.5000, 2.5000, 3.5000, 4.5000, 2.5000]
```

The first output sample is `0.5`, not `1`: the filter has not yet seen two samples, and the missing sample before the start is taken as zero. The extra last entry of `conv`, `2.5`, is the tail that a filter of finite length leaves after the input stops; `fir` drops it.

## Errors

Every call checks its arguments before computing and names the operation. The program shows an integer signal (the signal types are floating point or complex), a kernel that is empty, an input that has no sample axis, a `dim` out of range, and batch axes that cannot be broadcast.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace sig = pluss::subsp::eng::signal;

int main() {
  OutputEngine out;
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const std::exception &e) {
      std::string text(e.what());
      if (const auto cut = text.find("Triggered"); cut != std::string::npos) text.erase(cut);
      while (!text.empty() && text.back() == '\n') text.pop_back();
      out.writeln(text);
    }
  };
  const pluss::Series x = pluss::series::make(std::vector<double>{1, 2, 3, 4});
  const pluss::Series k = pluss::series::make(std::vector<double>{1, 1});
  report([&] { (void)sig::conv(pluss::make_series(std::vector<int>{1, 2, 3}, {3}), k); });
  report([&] { (void)sig::conv(x, pluss::series::zeros({0})); });
  report([&] { (void)sig::conv(pluss::make_series(2.0), k); });
  report([&] { (void)sig::conv(x, k, 3); });
  const pluss::Series three_lanes = pluss::series::zeros({3, 4});
  const pluss::Series two_lanes = pluss::series::zeros({2, 2});
  report([&] { (void)sig::conv(three_lanes, two_lanes); });
}
```

Output

```text
eng::signal::conv: input must have a floating or complex dtype
eng::signal::conv: sample axes must be non-empty
eng::signal::conv: input must have at least one dimension
eng::signal::conv: dim out of range
eng::signal::conv: non-sample axes are not broadcastable
```

## Decisions

- **A linear filter whose output you plot against the input:** `conv` with `Same`, or `fir` if it must be causal.
- **Finding the shift between two records:** `corr`, then the peak.
- **Averaging, differencing, smoothing:** `conv` with `Valid` when you do not want values computed from padded zeros; `Same` when you need the output the length of the input.
- **A recursive filter (output depends on past outputs):** not convolution; see [Filters and streaming](../signal_02_recursive_filters/filters_and_streaming.md).

## Pitfalls

The program shows three mistakes with their outputs. First, `conv` is not `corr`: with the kernel `[0, 0, 1]`, `conv` delays the signal by two samples while `corr` advances it. Second, `Valid` with a kernel longer than the input does not return nothing: it swaps roles and returns `K - N + 1` entries. Third, `Same` with an even-length kernel is not centred: a single impulse in the middle, smoothed by `[1, 1]`, comes out shifted one place to the right.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace sig = pluss::subsp::eng::signal;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  out.writeln("conv, kernel [0 0 1]", sig::conv(vec({1, 2, 3}), vec({0, 0, 1})).items<double>());
  out.writeln("corr, kernel [0 0 1]", sig::corr(vec({1, 2, 3}), vec({0, 0, 1})).items<double>());
  out.writeln("Valid, 3 samples with a 5-tap kernel: length", sig::conv(vec({1, 2, 3}), vec({1, 1, 1, 1, 1}), -1, sig::ConvMode::Valid).size(0));
  out.writeln("Same, [1 1] on an impulse at the middle", sig::conv(vec({0, 0, 1, 0, 0}), vec({1, 1}), -1, sig::ConvMode::Same).items<double>());
}
```

Output

```text
conv, kernel [0 0 1] [0.0000, 0.0000, 1.0000, 2.0000, 3.0000]
corr, kernel [0 0 1] [1.0000, 2.0000, 3.0000, 0.0000, 0.0000]
Valid, 3 samples with a 5-tap kernel: length 3
Same, [1 1] on an impulse at the middle [0.0000, 0.0000, 1.0000, 1.0000, 0.0000]
```

`conv` gives `0, 0, 1, 2, 3` (the signal delayed by two), `corr` gives `1, 2, 3, 0, 0`. `Valid` returns `5 - 3 + 1 = 3` entries. The impulse at position 2 of `Same` comes out spread over positions 2 and 3, not 1 and 2.

Two more mistakes to avoid:

- **The edges are computed from zeros.** `Full` and `Same` entries near the ends mix real samples with the zero padding; trust only `Valid` entries when the padding would bias the result.
- **A recursive filter is not a convolution.** A feedback filter has an infinite impulse response; see [Filters and streaming](../signal_02_recursive_filters/filters_and_streaming.md).

Reference: [`conv`](../../../../api/eng/signal/filtering/conv.md), [`ConvMode`](../../../../api/eng/signal/filtering/ConvMode.md), [`corr`](../../../../api/eng/signal/filtering/corr.md), [`fir`](../../../../api/eng/signal/filtering/fir.md).

Where next: [Filters and streaming](../signal_02_recursive_filters/filters_and_streaming.md) adds feedback, so that the output depends on previous outputs and an impulse response can last forever.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="windows.html">
    <span class="label">Previous</span>
    <span class="title">Windows</span>
  </a>
</div>
<!-- apiref:nav:end -->

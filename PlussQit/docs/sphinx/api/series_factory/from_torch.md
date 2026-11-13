<!-- pluss-hand-authored -->

# pluss::series::from_torch

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
Series pluss::series::from_torch(const torch::Tensor& t)
```

**Availability.** This factory exists only when the library is built with
`-DPLUSS_ENABLE_TORCH=ON`. In a build without LibTorch it is not declared and
`<torch/torch.h>` is not pulled in.

## What It Builds

A `Series` that **views** a CPU `torch::Tensor` — no data is copied. The series
shares the tensor's storage, adopts its `dtype`, shape, and strides, and holds
a reference that keeps the tensor's memory alive for the series' lifetime.

The result is not a restricted interop wrapper. It is an ordinary `Series`:
you can reshape it, filter it, aggregate it, combine it with other series,
move it to a Pluss accelerator, or make it the leaf of a new Pluss reverse-mode
graph. The only information that does not cross the boundary is the source
library's autodiff graph.

It is the inverse of the [`Series::tensor()`](../series/tensor.md) method. The
pair is deliberately asymmetric: you *construct* a series from a tensor with
this factory, and once you have a series the way back is a method call, not
another free function. There is no `pluss::series::to_torch`.

The contract:

- the tensor must be on the **CPU**
- its layout must be **non-overlapping and dense** (contiguous, or a plain
  strided view such as a transpose or a slice) — a stride-0 broadcast is
  rejected
- the dtype must have a pluss equivalent (all the real and integer dtypes,
  plus `Complex64` / `Complex128`)

## Verified Examples

> These examples require a `-DPLUSS_ENABLE_TORCH=ON` build.

Example 1. Wrap a contiguous tensor.

```cpp
torch::Tensor t = torch::arange(6, torch::kFloat64).reshape({2, 3});
pluss::series::print(std::cout, pluss::series::from_torch(t));
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡ 0.0000 1.0000 2.0000 ⎤
⎣ 3.0000 4.0000 5.0000 ⎦
```

Example 2. The series is a live view — mutating the tensor is visible through
it.

```cpp
torch::Tensor t = torch::arange(6, torch::kFloat64).reshape({2, 3});
auto s = pluss::series::from_torch(t);
t[0][0] = 99.0;

pluss::series::print(std::cout, s);
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡ 99.0000 1.0000 2.0000 ⎤
⎣  3.0000 4.0000 5.0000 ⎦
```

Example 3. A strided view (here a transpose) is wrapped without a copy — the
result is non-contiguous.

```cpp
torch::Tensor t = torch::arange(6, torch::kFloat64).reshape({2, 3});
pluss::series::print(std::cout, pluss::series::from_torch(t.transpose(0, 1)));
```

### Output

```text
Series(shape=[3,2], strides=[1,3], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=
⎡ 0.0000 3.0000 ⎤
⎢ 1.0000 4.0000 ⎥
⎣ 2.0000 5.0000 ⎦
```

Example 4. Start a Pluss reverse-mode computation from imported values.

```cpp
torch::Tensor source = torch::tensor({1.0, 2.0, 3.0}, torch::kFloat64);
auto values = pluss::series::from_torch(source);

values.tracks_slope_(true);
auto loss = values.square().sum();
loss.reverse();

auto sensitivity = values.sensitivity().host<double>();
std::cout << sensitivity[0] << ", "
          << sensitivity[1] << ", "
          << sensitivity[2] << "\n";
```

### Output

```text
2, 4, 6
```

`from_torch()` deliberately does not copy the source tensor's autodiff state.
Calling `tracks_slope_(true)` says that the imported values are leaves in a
**new Pluss graph**. Here the objective is

\[
L = x_0^2 + x_1^2 + x_2^2,
\]

so reverse propagation stores \(2x = [2, 4, 6]\) as the imported series'
sensitivity.

Example 5. An overlapping layout (a stride-0 broadcast) is rejected.

```cpp
try {
  pluss::series::from_torch(torch::ones({1, 3}, torch::kFloat64).expand({4, 3}));
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
from_torch: tensor must be non-overlapping and dense (or contiguous)
```

Example 6. Integer dtypes carry across unchanged.

```cpp
torch::Tensor t = torch::tensor({1, 2, 3}, torch::kInt32);
pluss::series::print(std::cout, pluss::series::from_torch(t));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int32, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 3]
```

## Notes

- The result is a **view**, not a copy, but it is not a fragile borrowed
  pointer. The series retains a tensor handle internally, so its storage stays
  alive even if the source variable leaves scope. Treat the tensor and series
  as two lifetime-safe handles on one CPU buffer.
- A GPU tensor raises `from_torch: CPU only`. Move it to the CPU first.
- No autodiff crosses the boundary: a tensor with `requires_grad` produces a
  plain series. Call `tracks_slope_(true)` when the imported values should be
  leaves in a new Pluss graph. Returning with `Series::tensor()` likewise
  starts a fresh LibTorch graph rather than reconnecting the graph that existed
  before import.
- The reverse trip is [`Series::tensor()`](../series/tensor.md).

## Entering Pluss Computation

`from_torch()` is the handoff into Pluss. On the CPU, that handoff is
zero-copy, and the imported series can immediately use the complete CPU
operation surface:

```cpp
auto observations = pluss::series::from_torch(cpu_tensor);
auto standardized = observations
    .sub(observations.stat().mean())
    .div(observations.stat().std());
```

To continue on a Pluss-supported accelerator, move the imported series with
the same `to()` method used by every other series:

```cpp
auto cpu_values = pluss::series::from_torch(cpu_tensor);
auto device_values = cpu_values.to(pluss::pCUDA);

device_values.tracks_slope_(true);
auto loss = device_values.square().mean();
loss.reverse();
```

The first line adopts the tensor's CPU storage without copying. The second
line allocates Pluss-managed CUDA storage and transfers the values. From that
point onward, `device_values` is a normal CUDA `Series`, and supported Pluss
operations and reverse propagation run under the CUDA backend.

If the source tensor already lives on an accelerator, first obtain a CPU
tensor before calling `from_torch()`, then select the desired Pluss platform:

```cpp
auto values = pluss::series::from_torch(source_tensor.to(torch::kCPU));
auto device_values = values.to(pluss::pCUDA);
```

This explicit staging keeps ownership and backend selection unambiguous. The
temporary CPU tensor remains alive because the returned series retains its
storage handle.

## Returning To LibTorch

Importing a tensor does not trap the data in pluss. After analysis or
preparation, call `Series::tensor()` to obtain a LibTorch tensor over the
result. The returned CPU tensor can be consumed directly by a CPU model or
transferred to the model's device:

```cpp
auto cpu_batch = pluss::series::from_torch(raw_batch);
auto prepared = prepare_for_training(cpu_batch);

auto training_batch = prepared.tensor().to(model_device);
auto prediction = model->forward(training_batch);
```

If `model_device` is the CPU, the view remains zero-copy. If it is an
accelerator, `.to(model_device)` creates an owning device allocation. In both
cases, the resulting tensor is valid training input; only cross-library
autodiff history is intentionally absent.

## Summary

`from_torch()` is a full entry point into Pluss:

- it creates a lifetime-safe, zero-copy CPU `Series`
- the result supports ordinary Pluss computation immediately
- `to(pluss::pCUDA)` or another supported platform performs the Pluss-side
  device transfer
- `tracks_slope_(true)` begins a new Pluss reverse-mode graph
- `Series::tensor()` provides the symmetric route back to LibTorch

## See Also

- [`Series::tensor`](../series/tensor.md): the method that goes the other way,
  Series to `torch::Tensor`.
- [`pluss::series::make`](make.md): build a series by **copying** host data
  when a shared view is not what you want.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="polar.html">
    <span class="label">Previous</span>
    <span class="title">polar</span>
  </a>
  <a class="next-link" href="../../series_factory_api_reference.html">
    <span class="label">Next</span>
    <span class="title">Series Factory API Reference</span>
  </a>
</div>

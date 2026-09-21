<!-- pluss-hand-authored -->

# Series::tensor

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
torch::Tensor Series::tensor() const
```

**Availability.** This method exists only when the library is built with
`-DPLUSS_ENABLE_TORCH=ON`. In a build without LibTorch the method is not
compiled and `<torch/torch.h>` is not required.

## What It Does

Exposes the series to LibTorch as a `torch::Tensor` **view** — no data is
copied. The returned tensor shares storage with the series: it sees the same
bytes, keeps that storage alive for as long as it is referenced, and reflects
writes made through either handle.

It is the reverse of [`pluss::series::from_torch`](../series_factory/from_torch.md).
There is no free `to_torch` function — construction from a tensor is a factory
call, and the trip back is this method.

The view carries the series' `dtype`, shape, **and strides**, so a
non-contiguous or offset series exports as a strided tensor rather than being
made contiguous first. If a downstream call needs contiguous input, call
`.contiguous()` on the `torch::Tensor` (or on the `Series` beforehand).

Only CPU series are supported. The dtype mapping is the natural one
(`Float64 <-> kFloat64`, `Int64 <-> kInt64`, `Complex64 <-> kComplexFloat`, and
so on).

## Parameters

None.

## Returns

`torch::Tensor`
    A CPU tensor viewing the series' storage, with matching dtype, shape, and
    strides.

Raises if the series is undefined, if its dtype has no torch equivalent, or if
its layout is overlapping (call `.contiguous()` first).

## Gradient

`tensor()` is a storage bridge, not a differentiable op. The returned
`torch::Tensor` is a fresh LibTorch leaf with `requires_grad == false`; pluss
sensitivities do not flow across the boundary in either direction.

That does **not** prevent training. Training data normally does not require a
gradient: model parameters receive gradients when the exported tensor is used
as an input. If the series' values are themselves the parameters to optimize,
enable gradients on the returned floating-point or complex tensor with
`requires_grad_(true)`.

## Handing A Series To A Training Pipeline

The zero-copy CPU view is already a usable LibTorch tensor. This is the direct
handoff when the model runs on the CPU:

```cpp
auto features = prepared_features.tensor();
auto targets = prepared_targets.tensor();

optimizer.zero_grad();
auto prediction = model->forward(features);
auto loss = torch::mse_loss(prediction, targets);
loss.backward();
optimizer.step();
```

Neither `features` nor `targets` needs `requires_grad`. Backward computes
gradients for the model's registered parameters.

For accelerator training, cross the storage boundary explicitly:

```cpp
torch::Device device(torch::kCUDA);
model->to(device);

auto features = prepared_features.tensor().to(device);
auto targets = prepared_targets.tensor().to(device);

optimizer.zero_grad();
auto prediction = model->forward(features);
auto loss = torch::mse_loss(prediction, targets);
loss.backward();
optimizer.step();
```

`tensor()` itself is CPU-only and zero-copy. The subsequent `.to(device)` is a
LibTorch transfer that allocates device-owned storage. The device tensor no
longer depends on the series' buffer and can outlive every pluss handle.

If the values, rather than a model, are what you intend to optimize, make that
choice explicit:

```cpp
auto parameter = initial_values.tensor().to(device).detach();
parameter.requires_grad_(true);

auto loss = parameter.square().sum();
loss.backward();
std::cout << parameter.grad() << "\n";
```

The call to `detach()` documents that this is the beginning of a new training
graph. On the CPU, omitting `.to(device)` preserves aliasing with the series;
optimizer writes will then change the shared buffer. Use `.clone()` first when
training should not mutate data still owned conceptually by pluss.

## Examples

> These examples require a `-DPLUSS_ENABLE_TORCH=ON` build.

Example 1. Export a series as a tensor view.

```cpp
auto a = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
torch::Tensor t = a.tensor();
std::cout << t << "\n";
```

### Output

```text
 1  2
 3  4
[ CPUDoubleType{2,2} ]
```

Example 2. The view is live: a write through the tensor is visible in the
series.

```cpp
auto a = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
torch::Tensor t = a.tensor();
t[0][1] = 42.0;

pluss::series::print(std::cout, a);
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=
⎡ 1.0000 42.0000 ⎤
⎣ 3.0000  4.0000 ⎦
```

Example 3. An offset view (here a row slice) exports its real data, not the
whole backing buffer.

```cpp
auto big = pluss::series::arange(0, 12).reshape({3, 4});
torch::Tensor rows = big.slice(0, 1, 3).tensor();
std::cout << rows << "\n";
```

### Output

```text
  4   5   6   7
  8   9  10  11
[ CPULongType{2,4} ]
```

Example 4. A round trip aliases the same buffer.

```cpp
torch::Tensor x = torch::ones({4}, torch::kFloat32);
torch::Tensor y = pluss::series::from_torch(x).tensor();
std::cout << "same data_ptr: " << (x.data_ptr() == y.data_ptr() ? "yes" : "no") << "\n";
```

### Output

```text
same data_ptr: yes
```

Example 5. An undefined series raises.

```cpp
try {
  pluss::Series u;
  (void)u.tensor();
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
to_torch: undefined
```

## Gotchas And Failure Modes

- The tensor is a **view**. If a later pluss operation replaces the series'
  storage (many in-place ops allocate fresh storage rather than mutating), a
  tensor captured earlier keeps pointing at the old buffer. Re-export after
  such an operation.
- No autodiff crosses the boundary. A `torch::Tensor` from `tensor()` does not
  carry pluss sensitivities, and `requires_grad` on the torch side does not
  reach back.
- Direct export is CPU-only. Transfer the returned tensor with `.to(device)`
  for accelerator training; that transfer intentionally creates new storage.
- Overlapping layouts (e.g. a stride-0 broadcast) are rejected; materialize
  with `.contiguous()` first.
- The method is absent entirely in a non-torch build — guard call sites with
  `#ifdef PLUSS_ENABLE_TORCH` if your code must compile both ways.

## Summary

`tensor()` bridges a series into LibTorch:

- zero-copy `torch::Tensor` view sharing storage, dtype, shape, and strides
- immediately usable as CPU training input, or transferable to an accelerator
- requires a `-DPLUSS_ENABLE_TORCH=ON` build
- the reverse of [`pluss::series::from_torch`](../series_factory/from_torch.md);
  there is no free `to_torch`
- not an autodiff boundary — grad is managed separately on each side

## See Also

- [`pluss::series::from_torch`](../series_factory/from_torch.md): the other
  direction — build a series from a `torch::Tensor`.
- [`Series::contiguous`](contiguous.md): force a dense layout before export if a
  consumer needs one.
- [`Series::to`](to.md): change dtype before crossing the boundary.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tanh.html">
    <span class="label">Previous</span>
    <span class="title">tanh</span>
  </a>
  <a class="next-link" href="tensor_split.html">
    <span class="label">Next</span>
    <span class="title">tensor_split</span>
  </a>
</div>

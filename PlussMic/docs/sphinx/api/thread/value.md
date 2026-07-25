<!-- pluss-hand-authored -->

# Thread::value

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::value() const -> ThreadValue
```

## What It Does

Wraps `*this` in a `ThreadValue` — a lightweight box around a whole `Thread` that
adds typed scalar-extraction helpers (`to_dbl()`, `to_int()`, `to_str()`, `host()`)
and implicit conversions to the underlying payload types (`pluss::Series`,
`pluss::Note`, `pluss::Timeline`, `pluss::TimeIndex`). `value()` itself does **not**
reduce or require a single-element thread — it wraps the thread as-is, at whatever
length it already has. The reduction methods ([`sum()`](sum.md), [`mean()`](mean.md),
[`min()`](min.md), and the rest) also return `ThreadValue`, typically already
length-1 after their own reduction; `value()` is the direct way to get the same box
type around a thread you already have, without any reduction step.

```{important} Scalar accessors require exactly one element
`to_dbl()`, `to_int()`, `to_str()`, and `host()` all check that the wrapped thread has
`size() == 1`, throwing `"ThreadValue::host: Thread payload must contain exactly one
value"` otherwise. Calling `.value()` on a multi-element thread is legal; calling
`.to_dbl()` on the result of that isn't. See Example 3.
```

## Parameters

`None`

## Returns

`ThreadValue`
    A box around `*this`, same length and kind as the source thread.

## Examples

Example 1. Wrap a single-element thread and pull out its scalar value.

```cpp
using namespace pluss::table;

auto single = Thread(pluss::make_series({42.0}));
auto boxed = single.value();
std::cout << boxed.to_dbl();
```

### Output

```text
42
```

Example 2. `value()` works on `Note`-kind threads too, via `to_str()`.

```cpp
auto label = Thread(pluss::make_note({"Accra"}));
std::cout << label.value().to_str();
```

### Output

```text
Accra
```

Example 3. `value()` on a multi-element thread succeeds — the scalar accessors are
what enforce the single-element requirement, not `value()` itself.

```cpp
auto many = Thread(pluss::make_series({1.0, 2.0, 3.0}));
auto boxed_many = many.value();
std::cout << std::boolalpha << boxed_many.defined() << " " << boxed_many.kind() << "\n";

try {
  auto bad = boxed_many.to_dbl();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
true Series
ThreadValue::host: Thread payload must contain exactly one value
```

## Notes

- `value()` never reduces or validates length — it's a plain wrapper.
- `to_dbl()` / `to_int()` / `to_str()` / `host()` all require the wrapped thread to
  have exactly one element.
- `ThreadValue` also converts implicitly back to `pluss::Series` / `pluss::Note` /
  `pluss::Timeline` / `pluss::TimeIndex`, matching the wrapped thread's `kind()`.

## See Also

- [`Thread::sum`](sum.md) / [`Thread::mean`](mean.md): reductions that return a
  `ThreadValue` already narrowed to a single element.
- [`Thread::kind`](kind.md): the payload kind a `ThreadValue`'s scalar accessor
  dispatches on internally.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="upper.html">
    <span class="label">Previous</span>
    <span class="title">upper</span>
  </a>
  <a class="next-link" href="var.html">
    <span class="label">Next</span>
    <span class="title">var</span>
  </a>
</div>

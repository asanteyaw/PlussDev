<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0292317d45e7 -->

# StreamLabelAccessor::has

[Back to StreamLabelAccessor API Reference](../../stream_lbl_accessor_api_reference)

## Signature

```cpp
bool StreamLabelAccessor::has() const
```

## What It Does

Reports whether the bound stream has an **explicitly set** row label —
`true` only after a prior [`set(...)`](set.md), `false` both before any
label is set and after a [`reset()`](reset.md).

```{important} has() is the accurate existence check — values() is not
[`values()`](values.md) always returns a usable `Stream`, synthesizing a
positional range when unlabeled — it never signals "no label" through its
return type. `has()` is the method that actually distinguishes "a real label
was set" from "nothing was set, here's a fallback."
```

## Parameters

`None`

## Returns

`bool`
    `true` if a label was explicitly set (and not since reset), `false`
    otherwise.

## Examples

Example 1. `has()` toggles across the set/reset lifecycle.

```cpp
using pluss::table::Slate;

auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}));
std::cout << "initial: " << std::boolalpha << stream.lbl().has() << "\n";

auto labeled = stream.lbl().set(Slate(pluss::make_note({"a", "b", "c"})));
std::cout << "after set: " << std::boolalpha << labeled.lbl().has() << "\n";

auto back = labeled.lbl().reset();
std::cout << "after reset: " << std::boolalpha << back.lbl().has();
```

### Output

```text
initial: false
after set: true
after reset: false
```

## Notes

- See the important callout above.
- Every method on this accessor returns a *new* `Stream` (the accessor never
  mutates in place), so `has()` must be re-checked on the result of
  `set()`/`reset()`, not on the original stream.

## See Also

- [`StreamLabelAccessor::values`](values.md): the method whose
  always-succeeds behavior makes `has()` necessary as a separate existence
  check.
- [`StreamLabelAccessor::set`](set.md) / [`reset()`](reset.md): the methods
  that actually change what `has()` reports.
- [`Stream::has_label`](../stream/has_label.md): the plain getter-pair
  equivalent, reached outside this accessor.
- [`Stream::lbl`](../stream/lbl.md): the entry point this accessor is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="align.html">
    <span class="label">Previous</span>
    <span class="title">align</span>
  </a>
  <a class="next-link" href="range.html">
    <span class="label">Next</span>
    <span class="title">range</span>
  </a>
</div>

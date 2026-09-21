<!-- pluss-hand-authored -->

# Series::view

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::view(pluss::RangeView<std::int64_t> new_sizes) const -> Series
```

## What It Does

Reinterprets the same underlying storage under a new shape without copying.

That no-copy rule is not just a performance footnote. It is the main semantic
contract of `view()`:

- same storage
- same values
- same `numel()`
- new logical shape
- fail if aliasing cannot be preserved directly

That is why `view()` and [`reshape()`](reshape.md) are both public APIs instead
of one API with two marketing descriptions:

- `view()` protects aliasing
- `reshape()` protects the requested final shape

If you remember only one thing from this page, remember that split.

## Parameters

**new_sizes** (`pluss::RangeView<std::int64_t>`)

The desired logical output shape.

In practice this is usually passed as:

- a brace literal like `{3, 2}`
- another tensor's [`sizes()`](sizes.md) result

Two rules matter:

- `new_sizes` must preserve total `numel()`
- the current implementation requires the source to be contiguous

So the method is validating both numerical compatibility and direct
view-eligibility.

## Returns

`Series`
    A new handle over the same storage with the requested logical shape.

This is a new object, but still an aliasing object. If one alias mutates the
shared storage, the other alias sees the same underlying values under its own
shape interpretation.

## Autodiff Behavior

`view()` preserves reverse-mode connectivity.

That is exactly the expected rule for a pure shape reinterpretation:

- forward changes metadata
- backward restores the original metadata

There is no arithmetic happening here, only indexing reinterpretation.

## Failure Model

`view()` is intentionally strict.

The main failure classes are:

- non-contiguous source
- incompatible target `numel()`

One current implementation quirk is worth stating explicitly: in the examples
below, those two conceptual failures collapse to the same top-line error text.
So the error string is less specific than the underlying rule.

## Verified Examples

Example 1. Reinterpret a contiguous `2 x 3` series as `3 x 2`.

```cpp
auto mat = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

std::cout << mat.view({3, 2});
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 2],
  [3, 4],
  [5, 6]]
```

This is not a transpose. The flat read order is still `[1, 2, 3, 4, 5, 6]`.
Only the shape used to interpret that order changed.

Example 2. The target shape can be borrowed from another tensor's metadata.

```cpp
auto shape_template = pluss::make_series({
    {10.0, 20.0},
    {30.0, 40.0},
    {50.0, 60.0},
});

std::cout << mat.view(shape_template.sizes());
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 2],
  [3, 4],
  [5, 6]]
```

This is the practical `sizes()` + `view()` pattern: reuse a known shape without
building an owning temporary.

Example 3. `view()` can also flatten when the aliasing contract still holds.

```cpp
std::cout << mat.view({6});
```

### Output

```text
pluss::Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[1, 2, 3, 4, 5, 6]
```

That makes the no-copy rule concrete. You are not asking for a fresh vector
copy. You are asking for the same storage seen as one axis.

Example 4. A non-contiguous source is rejected instead of being normalized.

```cpp
try {
  auto transposed = mat.transpose(0, 1);
  std::cout << transposed.view({3, 2});
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
view: requires contiguous in v0
```

This is the moment where `reshape()` becomes the correct tool instead. Once the
source stops being directly viewable, `view()` refuses to hide that fact behind
a copy.

Example 5. A `numel()` mismatch also fails.

```cpp
try {
  std::cout << mat.view({4, 2});
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
view: requires contiguous in v0
```

Conceptually the problem here is "wrong total element count." Practically, the
current implementation reports the same top-line message as the non-contiguous
case.

## Notes

- `view()` is the strict no-copy shape API.
- Equal `numel()` is mandatory.
- The current implementation requires contiguity.
- If your contract is "final shape first, aliasing if possible," use
  [`reshape()`](reshape.md) instead.

## See Also

- [`Series::reshape`](reshape.md): copy-capable fallback shape change.
- [`Series::transpose`](transpose.md): common source of non-contiguous layouts.
- [`Series::sizes`](sizes.md): borrowed shape metadata for reuse.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sizes.html">
    <span class="label">Previous</span>
    <span class="title">sizes</span>
  </a>
  <a class="next-link" href="vsplit.html">
    <span class="label">Next</span>
    <span class="title">vsplit</span>
  </a>
</div>

<!-- pluss-hand-authored -->

# Series::put_

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::put_(const Series& index, const Series& source, bool accumulate = false) const -> Series&
```

## What It Does

Mutates the series by writing to **flattened linear positions** and returns
`*this`.

`put_()` is the write-side companion to [`take()`](take.md). The addressing
model is exactly the same:

- `self` is treated as one flat logical sequence
- `index` supplies linear positions into that sequence
- writes land at those flat positions regardless of the original rank

This method is therefore about flat scatter semantics, not about row/column
selection or advanced indexing along named dimensions. If your mental model is
"replace these rows," "replace this column," or "write through a bool mask,"
you usually want [`index_put_()`](index_put_.md) or [`extract_put_()`](extract_put_.md)
instead.

One API quirk is worth stating directly: the public method is declared `const`
on the handle even though it mutates the underlying storage. On this surface,
handle constness does not imply value immutability.

## Parameters

**index** (`const Series&`)

The flat linear write positions.

Important rules:

- the index tensor must be on the same platform as `self`
- on CPU, the underlying flat write engine expects `Int64`
- negative indices are allowed and count from the end
- repeated indices are allowed

**source** (`const Series&`)

The replacement data.

`source` is normalized to the destination dtype, then must satisfy one of two
cardinality rules:

- scalar, which broadcasts to every indexed position
- same `numel()` as `index`, which supplies one source value per write

**accumulate** (`bool`)

Controls write mode:

- `false`: replacement semantics
- `true`: additive semantics

This toggle is the most important behavioral switch on the method.

## Returns

`Series&`
    The same mutated series handle.

## Examples

Example 1. A scalar source broadcasts across every indexed flat position.

```cpp
auto put_idx = pluss::make_series(std::vector<std::int64_t>{0, 2, 5}, {3});
auto flat = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4, 5}, {6});

flat.put_(put_idx, pluss::codem::value(-1.0), false);
std::cout << flat;
```

### Output

```text
pluss::Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[-1, 1, -1, 3, 4, -1]
```

Example 2. Negative indices count from the end of the same flattened logical order.

```cpp
auto negative_idx = pluss::make_series(std::vector<std::int64_t>{-1, -3}, {2});
auto negative_values = pluss::make_series(std::vector<double>{99, 77}, {2});

flat.put_(negative_idx, negative_values, false);
std::cout << flat;
```

### Output

```text
pluss::Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[-1, 1, -1, 77, 4, 99]
```

At that point `flat` was already `[-1, 1, -1, 3, 4, -1]`, so:

- `-1` targeted the last element and replaced it with `99`
- `-3` targeted the third-from-last element and replaced it with `77`

Example 3. With `accumulate=false`, repeated indices behave like repeated
replacements, so later writes to the same position win.

```cpp
auto replace_idx = pluss::make_series(std::vector<std::int64_t>{1, 1, 4}, {3});
auto replace_base = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4, 5}, {6});
auto replace_values = pluss::make_series(std::vector<double>{50, 60, 90}, {3});

replace_base.put_(replace_idx, replace_values, false);
std::cout << replace_base;
```

### Output

```text
pluss::Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[0, 60, 2, 3, 90, 5]
```

Position `1` first receives `50`, then `60`, so the final stored value is the
last write.

Example 4. With `accumulate=true`, repeated indices add into the destination
instead of replacing it.

```cpp
auto accum_base = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4, 5}, {6});
auto accum_values = pluss::make_series(std::vector<double>{10, 20, 30}, {3});

accum_base.put_(replace_idx, accum_values, true);
std::cout << accum_base;
```

### Output

```text
pluss::Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[0, 31, 2, 3, 34, 5]
```

Starting from `[0, 1, 2, 3, 4, 5]`:

- index `1` receives `+10` and `+20`, so `1` becomes `31`
- index `4` receives `+30`, so `4` becomes `34`

Example 5. A non-scalar source must either be scalar or match the number of
indexed writes exactly.

```cpp
try {
  auto bad_values = pluss::make_series(std::vector<double>{1, 2}, {2});
  flat.put_(put_idx, bad_values, false);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
put: src must be scalar or same numel as index
```

## Notes

- `put_()` is the flat scatter primitive.
- Negative indices are supported.
- Duplicate positions are legal in both modes; `accumulate=false` keeps last-write semantics, while `accumulate=true` sums.
- If you need axis-aware writes or mask writes, prefer [`index_put_()`](index_put_.md).

## See Also

- [`Series::take`](take.md): flat linear reads.
- [`Series::index_put_`](index_put_.md): explicit advanced-index writes.
- [`Series::extract_put_`](extract_put_.md): selector-language writes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="take.html">
    <span class="label">Previous</span>
    <span class="title">take</span>
  </a>
  <a class="next-link" href="clamp_max.html">
    <span class="label">Next</span>
    <span class="title">clamp_max</span>
  </a>
</div>

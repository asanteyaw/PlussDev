<!-- pluss-hand-authored -->

# Series::is_same_size

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
bool Series::is_same_size(const Series& other) const noexcept
```

## What It Does

Returns `true` when `self` and `other` have the **same rank and the same
extent on every axis** — identical [`sizes()`](sizes.md) — and `false`
otherwise.

It compares **shape only**. Values, dtype, platform, strides, storage
identity, and contiguity are all ignored. Two `Series` of completely
different numbers, on different platforms, in different dtypes, are
`is_same_size()` as long as their shapes match exactly.

It is deliberately **stricter than broadcast compatibility**. Shapes `[3]`
and `[1, 3]` broadcast together happily, but they are not the same size —
different rank. `is_same_size()` is the check for operations that require
shapes to line up element-for-element with no stretching: an elementwise
copy into an existing buffer, a residual add where you refuse to broadcast,
an assertion that a computed shape matches an expected one.

It is also weaker than value equality: matching shape says nothing about
matching contents. For "shape *and* values" use [`equal`](equal.md); for
"same underlying storage and view" use [`is_set_to`](is_set_to.md).

Two undefined `Series` count as the same size (both have "no shape"); a
defined and an undefined one do not.

## Parameters

**other** (`const Series&`): the `Series` to compare shapes with.

## Returns

`bool`
    `true` iff `self.sizes()` equals `other.sizes()` exactly (same length,
    same values).

## Gradient

Not applicable — `is_same_size()` returns a plain `bool`.

## Examples

Example 1. Shape only — dtype, platform, strides, and values do not matter.

```cpp
auto a = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});   // [2,3] float64
auto b = pluss::series::zeros({2, 3}, pluss::ScalarType::Int32);   // [2,3] int32, all zero
auto t = a.transpose(0, 1);                                        // [3,2]
auto row = pluss::make_series({1.0, 2.0, 3.0});                    // [3]
std::cout << std::boolalpha
          << "[2,3] float vs [2,3] int: " << a.is_same_size(b) << "\n"
          << "[2,3] vs [3,2] (T):       " << a.is_same_size(t) << "\n"
          << "[3] vs [1,3]:             " << row.is_same_size(row.unsqueeze(0)) << "\n";
```

### Output

```text
[2,3] float vs [2,3] int: true
[2,3] vs [3,2] (T):       false
[3] vs [1,3]:             false
```

The first pair differ in dtype and in every value but match in shape →
`true`. The transpose has the same `numel()` (`6`) but a different shape →
`false`, which is exactly why `numel()` equality is *not* shape equality.
`[3]` and `[1, 3]` are broadcast-compatible but not the same size.

Example 2. Undefined `Series`.

```cpp
pluss::Series u1, u2;
auto d = pluss::make_series({1.0});
std::cout << std::boolalpha
          << "undef vs undef: " << u1.is_same_size(u2) << "\n"
          << "undef vs def:   " << u1.is_same_size(d) << "\n";
```

### Output

```text
undef vs undef: true
undef vs def:   false
```

Example 3. Same size does not imply same values — that is
[`equal`](equal.md)'s job.

```cpp
auto a = pluss::make_series({1.0, 2.0, 3.0});
auto b = pluss::make_series({9.0, 9.0, 9.0});
std::cout << std::boolalpha
          << "is_same_size: " << a.is_same_size(b) << "\n"
          << "equal:        " << a.equal(b) << "\n";
```

### Output

```text
is_same_size: true
equal:        false
```

Example 4. Same values do not imply same size — a broadcastable pair can
carry identical numbers at different ranks.

```cpp
auto flat = pluss::make_series({1.0, 2.0, 3.0});          // [3]
auto rowed = flat.unsqueeze(0);                           // [1,3], same three values
std::cout << std::boolalpha
          << "is_same_size: " << flat.is_same_size(rowed) << "\n"
          << "equal:        " << flat.equal(rowed) << "\n";
```

### Output

```text
is_same_size: false
equal:        false
```

`equal()` also returns `false` here — it requires matching shape too. Only
an explicit `flat.reshape(rowed.sizes())` or a broadcast makes these
interchangeable.

## Gotchas And Failure Modes

- Same `numel()` is **not** same size — `[2, 3]` and `[3, 2]` both have six
  elements. Compare the shapes, not the counts.
- Broadcast-compatible is **not** same size — `[3]` and `[1, 3]` differ in
  rank. If you want to allow broadcasting, do not use this check.
- The comparison ignores dtype and platform entirely; combine with
  [`dtype`](dtype.md) / [`is_cpu`](is_cpu.md) checks if those must match too.
- `noexcept` and cheap — safe in a hot path or an assertion.

## Summary

`is_same_size(other)` compares shape and nothing else:

- `true` iff `sizes()` matches exactly (same rank, same extents)
- stricter than broadcast compatibility, weaker than
  [`equal`](equal.md)
- ignores dtype, platform, strides, storage, and values
- two undefined `Series` match; a defined/undefined pair does not

## See Also

- [`Series::equal`](equal.md): same shape **and** exact corresponding
  values.
- [`Series::is_set_to`](is_set_to.md): same storage region and view
  geometry.
- [`Series::sizes`](sizes.md) / [`Series::size`](size.md): the shape being
  compared.
- [`Series::broadcast_to`](broadcast_to.md): make two differently-sized
  `Series` line up when you *do* want stretching.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_prime.html">
    <span class="label">Previous</span>
    <span class="title">is_prime</span>
  </a>
  <a class="next-link" href="is_set_to.html">
    <span class="label">Next</span>
    <span class="title">is_set_to</span>
  </a>
</div>

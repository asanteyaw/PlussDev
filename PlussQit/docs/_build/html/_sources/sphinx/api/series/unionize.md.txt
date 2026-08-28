<!-- pluss-hand-authored -->

# Series::unionize

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::unionize(pluss::RangeView<Series> others, bool sorted = true) const -> Series
```

## What It Does

`unionize()` generalizes [`union1d()`](union1d.md) from one extra input to many.

You start from `self`, then provide a list of additional `Series` values. The
method computes the distinct union across the whole collection.

In normal user code, the important thing is not the internal parameter wrapper.
The important thing is the call style:

- brace lists such as `{b, c, d}` work naturally
- ordinary `Series` containers can also be passed when they match the range
  contract

Like `union1d()`, this root method is 1D-set-oriented:

- every input is flattened
- duplicates are removed
- ordering is controlled by `sorted`

## Parameters

**others** (`pluss::RangeView<Series>`)

The additional inputs to fold into the union.

Conceptually, this is "the rest of the series to union with `self`." The method
walks that list from left to right.

**sorted** (`bool`)

Controls output ordering.

- `true`: sort the final distinct union
- `false`: preserve first encounter order while scanning `self`, then each item
  in `others` from left to right

This makes `unionize()` useful both as a true set constructor and as a stable
deduplicating merge over several sources.

## Returns

`Series`
    A flat 1D series containing one copy of each distinct value seen across
    `self` and every input in `others`.

## Gradient

`unionize()` is not differentiable.

It is a set-style data-shaping operation driven by equality and ordering.

## Examples

Example 1. If `others` is empty, `unionize()` reduces to distinct extraction on
`self`.

```cpp
auto base = pluss::make_series({3, 1, 3, 2}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, base.unionize({}, false));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[3, 1, 2]
```

Example 2. `sorted=false` keeps first-seen order across the whole chain.

```cpp
auto b = pluss::make_series({2, 4, 1}, pluss::ScalarType::Int64);
auto c = pluss::make_series({4, 2, 9}, pluss::ScalarType::Int64);

std::cout << base.unionize({b, c}, false);
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=int64, platform=cpu, numel=5, contiguous=true)
data=[3, 1, 2, 4, 9]
```

The scan order is:

- start with `base`
- then scan `b`
- then scan `c`

Each new value is admitted only the first time it appears.

Example 3. The default call returns the sorted distinct union.

```cpp
std::cout << base.unionize({b, c});
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=int64, platform=cpu, numel=5, contiguous=true)
data=[1, 2, 3, 4, 9]
```

Example 4. Stable union order keeps following the sequence of inputs you pass.

```cpp
std::cout << pluss::make_series({5, 1}, pluss::ScalarType::Int64).unionize({
    pluss::make_series({1, 2}, pluss::ScalarType::Int64),
    pluss::make_series({2, 5, 3}, pluss::ScalarType::Int64),
    pluss::make_series({3, 4}, pluss::ScalarType::Int64),
}, false);
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=int64, platform=cpu, numel=5, contiguous=true)
data=[5, 1, 2, 3, 4]
```

That example is worth studying because it shows the method's mental model very
clearly: "walk sources left to right and keep each value once."

## Gotchas And Failure Modes

- `unionize()` is still a flat 1D set-style method. It does not preserve
  higher-rank slice structure.
- `sorted=false` depends on the order of the inputs in `others`.
- Each participating `Series` must share dtype and platform with `self`.
- Empty `others` is not an error; it simply degenerates to distinct extraction.

## Summary

Reach for `unionize()` when the same distinct-union idea needs to span more than
two inputs.

- use brace lists like `{b, c, d}` for the normal call style
- choose `sorted=false` for stable source-order deduplication
- choose the default `sorted=true` when you want value-ordered output
- expect flattening and one-copy-per-distinct-value semantics

## See Also

- [`Series::union1d`](union1d.md): the two-input form.
- [`Series::intersection`](intersection.md): many-input common-value extraction.
- [`Series::comp`](comp.md): subtract `self` from a universe instead of unioning.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="union1d.html">
    <span class="label">Previous</span>
    <span class="title">union1d</span>
  </a>
  <a class="next-link" href="unionize_along.html">
    <span class="label">Next</span>
    <span class="title">unionize_along</span>
  </a>
</div>

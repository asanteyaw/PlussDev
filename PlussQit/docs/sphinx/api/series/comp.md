<!-- pluss-hand-authored -->

# Series::comp

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::comp(const Series& universe) const -> Series
Series::comp(const Series& universe, bool sorted) const -> Series
Series::comp(const Series& universe, bool sorted, bool assume_unique) const -> Series
```

## What It Does

`comp()` computes the complement of `self` with respect to a `universe`.

That wording matters. The method does not ask "what is in `self` but not in
`universe`?" It asks the reverse question:

- start from `universe`
- remove everything that appears in `self`
- return what remains

So `comp()` is best read as "complement inside this universe."

Internally, it uses the same set-difference mechanics as
[`setdiff1d()`](setdiff1d.md), which means:

- both sides are flattened
- output is 1D
- `sorted` controls ordering
- `assume_unique` controls pre-deduplication

## Parameters

**universe** (`const Series&`)

The pool of candidate values from which the complement is taken.

Anything not present in `universe` can never appear in the result, even if it
appears in `self`.

**sorted** (`bool`)

- `true`: sort the complement
- `false`: preserve encounter order inherited from `universe`

**assume_unique** (`bool`)

- `false`: deduplicate before subtraction
- `true`: trust the inputs and skip that work

If you pass `true` on a non-unique `universe`, repeated surviving universe
values can remain.

## Returns

`Series`
    A flat 1D series containing the values from `universe` that are absent from
    `self`.

## Gradient

`comp()` is not differentiable.

It is a universe-membership subtraction.

## Examples

Example 1. Basic complement removes `self` from the provided universe.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({1, 2, 2, 4}, pluss::ScalarType::Int64).comp(
        pluss::make_series({0, 1, 2, 3, 4}, pluss::ScalarType::Int64)));
```

### Output

```text
Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[0, 3]
```

Example 2. `sorted=false` preserves the universe's encounter order.

```cpp
std::cout << pluss::make_series({1, 4}, pluss::ScalarType::Int64).comp(
    pluss::make_series({7, 4, 0, 1, 3}, pluss::ScalarType::Int64),
    false);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[7, 0, 3]
```

Example 3. `assume_unique=true` can preserve repeated surviving universe
values.

```cpp
std::cout << pluss::make_series({1, 2, 4}, pluss::ScalarType::Int64).comp(
    pluss::make_series({0, 0, 1, 2, 3, 3, 4}, pluss::ScalarType::Int64),
    false,
    true);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[0, 0, 3, 3]
```

Example 4. Values that are outside the universe simply do not matter.

```cpp
std::cout << pluss::make_series({1, 2, 9}, pluss::ScalarType::Int64).comp(
    pluss::make_series({0, 1, 2, 3}, pluss::ScalarType::Int64),
    false);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[0, 3]
```

The `9` in `self` has no effect because it is not a member of the universe to
begin with.

## Gotchas And Failure Modes

- `comp()` is universe-relative. Without the right universe, the question is
  ill-posed.
- Encounter-order mode follows the universe, not `self`.
- `assume_unique=true` can preserve duplicate universe survivors.
- The result is flat and 1D.
- Dtype mismatch and platform mismatch fail through the shared set checks.

## Summary

Read `comp(universe)` as "the complement of `self` inside `universe`."

- it subtracts `self` from `universe`, not the other way around
- `sorted=false` preserves universe order
- values outside the universe are irrelevant
- `assume_unique=true` can preserve repeated surviving universe values

## See Also

- [`Series::setdiff1d`](setdiff1d.md): the raw left-minus-right form.
- [`Series::unionize`](unionize.md): grow a universe instead of subtracting from
  one.
- [`Series::intersection`](intersection.md): keep only common members.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="clone.html">
    <span class="label">Previous</span>
    <span class="title">clone</span>
  </a>
  <a class="next-link" href="contiguous.html">
    <span class="label">Next</span>
    <span class="title">contiguous</span>
  </a>
</div>

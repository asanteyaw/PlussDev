<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b0ebabe5fe33 -->

# Stream::invalid

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream Stream::invalid() const
```

## What It Does

Returns a new `Bool`-dtype `Series` stream, same length as the receiver, marking which rows
are invalid — the same shape and, in every case tested here, the same result as
[`isnull()`](isnull.md). Delegates to `Slate::invalid()` (a distinct underlying method from
`Slate::isnull()`), works uniformly across all four payload kinds.

## Parameters

`None`

## Returns

`Stream`
    A new `Series`-kind stream of `bool` values: `true` where the row is invalid, `false`
    otherwise.

## Examples

Example 1. `invalid()` reports the same rows as `isnull()` for a mask-based `NULL`.

```cpp
auto mask = pluss::make_series(std::vector<bool>{true, false, true, false});
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0}), mask);

std::cout << stream.invalid();
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[bool]  records=4
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | false                                                                                                    
1            | true                                                                                                     
2            | false                                                                                                    
3            | true                                                                                                     
========================================================================================================================
```

Example 2. Also agrees with `isnull()` for an unmarked `NaN` in a numeric stream built
with no explicit validity mask.

```cpp
double nan_value = std::numeric_limits<double>::quiet_NaN();
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, nan_value, 3.0}));

std::cout << stream.invalid();
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[bool]  records=3
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | false                                                                                                    
1            | true                                                                                                     
2            | false                                                                                                    
========================================================================================================================
```

## Notes

- Works on every payload kind — no `Series`-only restriction.
- Calls a distinct underlying `Slate::invalid()` method from
  [`isnull()`](isnull.md)/`Slate::isnull()`, but has produced an identical result in every
  case checked here, including both mask-based `NULL`s and unmarked numeric `NaN`.
- The result is always `Series`-kind (`Series[bool]`), regardless of the receiver's own
  kind.

## See Also

- [`Stream::isnull`](isnull.md) / [`Stream::notnull`](notnull.md): the closely related
  validity checks this method agrees with in practice.
- [`Stream::fillnull`](fillnull.md) / [`Stream::dropnull`](dropnull.md): act on the rows
  this method identifies.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="head.html">
    <span class="label">Previous</span>
    <span class="title">head</span>
  </a>
  <a class="next-link" href="isnull.html">
    <span class="label">Next</span>
    <span class="title">isnull</span>
  </a>
</div>

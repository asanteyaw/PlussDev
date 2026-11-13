<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7c483cc49b3c -->

# Stream::notnull

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream Stream::notnull() const
```

## What It Does

Returns a new `Bool`-dtype `Series` stream, same length as the receiver, marking which rows
have a value — the exact logical complement of [`isnull()`](isnull.md). Works uniformly
across all four payload kinds; delegates to `Slate::notnull()`.

## Parameters

`None`

## Returns

`Stream`
    A new `Series`-kind stream of `bool` values: `true` where the row has a value, `false`
    where it's `NULL`.

## Examples

Example 1. `notnull()` is the exact inverse of `isnull()`, row for row.

```cpp
auto mask = pluss::make_series(std::vector<bool>{true, false, true, false});
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0}), mask);

std::cout << stream.notnull();
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[bool]  records=4
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | true                                                                                                     
1            | false                                                                                                    
2            | true                                                                                                     
3            | false                                                                                                    
========================================================================================================================
```

## Notes

- Works on every payload kind — no `Series`-only restriction.
- The result is always `Series`-kind (`Series[bool]`), regardless of the receiver's own
  kind.
- Combine with [`filter()`](filter.md) to select only the non-`NULL` rows:
  `stream.filter(stream.notnull().as_series())` (see [`filter()`](filter.md) for the full
  pattern), or use [`dropnull()`](dropnull.md) directly for that common case.

## See Also

- [`Stream::isnull`](isnull.md): the exact logical complement of this method.
- [`Stream::dropnull`](dropnull.md): removes the rows this method marks `false` for,
  directly.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ndistinct.html">
    <span class="label">Previous</span>
    <span class="title">ndistinct</span>
  </a>
  <a class="next-link" href="nrecords.html">
    <span class="label">Next</span>
    <span class="title">nrecords</span>
  </a>
</div>

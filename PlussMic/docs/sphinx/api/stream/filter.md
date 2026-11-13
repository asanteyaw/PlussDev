<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d5ad18f75c58 -->

# Stream::filter

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream Stream::filter(const pluss::Series& bool_mask) const
```

## What It Does

Returns a new stream containing only the rows where `bool_mask` is `true`, in original
order. `bool_mask` is a raw `pluss::Series`, not a `Stream` — a common pattern is deriving
it from another `Stream`'s boolean result (e.g. [`notnull()`](notnull.md)) via
[`as_series()`](as_series.md).

## Parameters

**bool_mask** (`const pluss::Series&`)
    A boolean mask, same length as the stream. `true` keeps the row; `false` drops it.

## Returns

`Stream`
    A new stream containing only the rows where `bool_mask` is `true`.

## Examples

Example 1. Filter with an explicit boolean mask.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));
auto mask = pluss::make_series(std::vector<bool>{true, false, true});

std::cout << stream.filter(mask);
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=2
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 10                                                                                                       
1            | 30                                                                                                       
========================================================================================================================
```

Example 2. Combine with [`notnull()`](notnull.md) and [`as_series()`](as_series.md) to
drop `NULL` rows manually — the same effect as [`dropnull()`](dropnull.md), spelled out
explicitly.

```cpp
auto mask = pluss::make_series(std::vector<bool>{true, false, true});
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}), mask);

std::cout << stream.filter(stream.notnull().as_series());
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=2
label=no  mask=yes  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 1                                                                                                        
1            | 3                                                                                                        
========================================================================================================================
```

Example 3. A mask whose length doesn't match the stream's length throws.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));
auto short_mask = pluss::make_series(std::vector<bool>{true, false});

try {
  auto out = stream.filter(short_mask);
} catch (const std::exception& e) {
  std::cout << std::string_view(e.what()).substr(0, 39);
}
```

### Output

```text
Slate::filter: bool mask length mismat
```

## Notes

- `bool_mask` must be a raw `pluss::Series`, not a `Stream` — pull one out of a boolean
  `Stream` result with [`as_series()`](as_series.md), as in Example 2.
- Length mismatch throws immediately; see Example 3.
- Note the result at Example 2 shows `mask=yes` — the resulting stream still carries its own
  validity mask (now all-valid, since the `NULL` rows were removed), distinct from the
  boolean filter mask that selected them.

## See Also

- [`Stream::dropnull`](dropnull.md): the direct shorthand for the notnull-filter pattern in
  Example 2.
- [`Stream::take`](take.md): select rows by explicit position instead of a boolean mask.
- [`Stream::isnull`](isnull.md) / [`Stream::notnull`](notnull.md): common sources of the
  mask this method consumes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fillnull.html">
    <span class="label">Previous</span>
    <span class="title">fillnull</span>
  </a>
  <a class="next-link" href="freq.html">
    <span class="label">Next</span>
    <span class="title">freq</span>
  </a>
</div>

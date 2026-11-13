<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: feb3b4126c58 -->

# Stream::cast

[Back to Stream API Reference](../../stream_api_reference)

## Signatures

```cpp
Stream Stream::cast(pluss::table::SlateCastTarget target, std::optional<pluss::codem::ScalarType> series_dtype = std::nullopt) const
Stream Stream::cast(pluss::table::SlateCastTarget target, const pluss::table::SlateCastOptions& options) const
```

## What It Does

Converts the stream to a different payload kind (`Series`, `Note`, `Timeline`,
`TimeIndex`), delegating to `Slate::cast()`; see [`Slate::cast`](../slate/cast.md) for
the full breakdown of what each `SlateCastTarget` means and how `SlateCastOptions`
(dtype selection, parse-failure handling) affects the conversion. The first overload is a
convenience shorthand accepting just a `series_dtype` directly instead of a full
`SlateCastOptions`; the second accepts the full options struct.

```{note} cast() also has a no-argument accessor-returning overload
`Stream::cast()` (no arguments) and `Stream::cast(options)` (options only, no target)
return a `StreamCastAccessor` with kind-named methods (`series()`/`note()`/`timeline()`/
`timeindex()`) instead of taking `target` as an argument. That accessor surface is out of
scope for this page — the two overloads documented here (which take `target` directly) are
the primary, most commonly used path.
```

## Parameters

**target** (`pluss::table::SlateCastTarget`)
    The payload kind to cast to — `Series`, `Note`, `Timeline`, or `TimeIndex`.

**series_dtype** (`std::optional<pluss::codem::ScalarType>`, first overload, defaults to
`std::nullopt`)
    The numeric dtype to use when `target` is `Series`. Shorthand for setting just this one
    field of `SlateCastOptions`.

**options** (`const pluss::table::SlateCastOptions&`, second overload)
    The full conversion policy — `series_dtype` and `invalid` (parse-failure handling). See
    [`Slate::cast`](../slate/cast.md) for the complete breakdown.

## Returns

`Stream`
    A new stream with its payload converted to `target`.

## Examples

Example 1. Cast numeric-looking text to `Series`, letting the dtype be inferred.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"1", "2", "3"}));

std::cout << stream.cast(pluss::table::SlateCastTarget::Series);
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=3
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 1                                                                                                        
1            | 2                                                                                                        
2            | 3                                                                                                        
========================================================================================================================
```

Example 2. Request a specific numeric dtype via the shorthand `series_dtype` argument.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"1", "2", "3"}));

auto out = stream.cast(pluss::table::SlateCastTarget::Series, pluss::codem::ScalarType::Int64);
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[int64]  records=3
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 1                                                                                                        
1            | 2                                                                                                        
2            | 3                                                                                                        
========================================================================================================================
```

Example 3. The full `SlateCastOptions` overload does the same thing, spelled out.

```cpp
pluss::table::SlateCastOptions options;
options.series_dtype = pluss::codem::ScalarType::Int64;

auto stream = pluss::kindle::Stream(pluss::make_note({"1", "2", "3"}));
std::cout << stream.cast(pluss::table::SlateCastTarget::Series, options);
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[int64]  records=3
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 1                                                                                                        
1            | 2                                                                                                        
2            | 3                                                                                                        
========================================================================================================================
```

## Notes

- Follows the same per-target conversion rules as [`Slate::cast`](../slate/cast.md),
  including its documented quirk that `SlateCastOptions::invalid` only actually takes
  effect for the `Timeline` target.
- The no-argument `cast()`/`cast(options)` accessor overloads (returning
  `StreamCastAccessor`) are out of scope for this page — see the note above.
- Requires a defined stream; throws `"Stream: undefined handle"` on a default-constructed
  `Stream()`.

## See Also

- [`Stream::astype`](astype.md): the shorthand for `cast(Series, {.series_dtype = dtype})`.
- [`Slate::cast`](../slate/cast.md): the underlying single-stream conversion this method
  delegates to.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bfill.html">
    <span class="label">Previous</span>
    <span class="title">bfill</span>
  </a>
  <a class="next-link" href="clear_label.html">
    <span class="label">Next</span>
    <span class="title">clear_label</span>
  </a>
</div>

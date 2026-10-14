<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8322e15a5eb7 -->

# Stream::fillnull

[Back to Stream API Reference](../../stream_api_reference)

## Signatures

```cpp
Stream::fillnull(double value) const -> Stream
Stream::fillnull(std::string_view value) const -> Stream
Stream::fillnull(const char* value) const -> Stream
Stream::fillnull(const pluss::Timeline& value) const -> Stream
Stream::fillnull(const pluss::Duration& value) const -> Stream
```

## What It Does

Replaces every `NULL` cell in the stream with `value`, returning a new stream. `value`'s
C++ type must match the stream's own payload kind exactly — `double` for `Series`,
`std::string_view`/`const char*` for `Note`, `pluss::Timeline` for `Timeline`,
`pluss::Duration` for `Duration`. Delegates straight to `Slate::fillnull()`.

```{important} Kind mismatch throws here — unlike Catalog::fillnull()'s silent skip
`Stream::fillnull()` operates on a single, already-typed stream, so a kind mismatch throws
immediately (e.g. calling the `string_view` overload on a `Series`-kind stream throws
`"Slate::fillnull(string) is only valid for Note payload"`). This is the opposite of
[`Catalog::fillnull()`](../catalog/fillnull.md), which silently skips any stream whose kind
doesn't match `value`'s type across the whole collection — there, kind selection is the
point; here, on a single stream, a mismatch is a caller error worth surfacing.
```

## Parameters

**value** (`double` / `std::string_view` / `const char*` / `const pluss::Timeline&` /
`const pluss::Duration&`)
    The fill value. Its C++ type must match the stream's payload kind — see the important
    callout above.

## Returns

`Stream`
    A new stream, same kind and length as the receiver, with every `NULL` cell replaced by
    `value`.

## Examples

Example 1. Fill `NULL` cells in a numeric stream with `0.0`.

```cpp
auto mask = pluss::make_series(std::vector<bool>{true, false, true});
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}), mask);

std::cout << stream.fillnull(0.0);
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
1            | 0                                                                                                        
2            | 3                                                                                                        
========================================================================================================================
```

Example 2. Fill `NULL` cells in a `Note`-kind stream with a placeholder string.

```cpp
auto mask = pluss::make_series(std::vector<bool>{true, false, true});
auto stream = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}), mask);

std::cout << stream.fillnull("Unknown");
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Note  dtype=Note  records=3
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | Accra                                                                                                    
1            | Unknown                                                                                                  
2            | Kumasi                                                                                                   
========================================================================================================================
```

Example 3. Fill a `Duration`-kind stream with a fixed replacement, same delegation
pattern as the `double`/`Note`/`Timeline` overloads above.

```cpp
auto stream = pluss::kindle::Stream(
    pluss::duration::nanos({1'000'000'000LL, pluss::Duration::kNaT, -2'000'000'000LL}));

auto out = stream.fillnull(pluss::duration::nanos(9));
assert(out.to_slate().as_duration().host() ==
       std::vector<std::int64_t>({1'000'000'000LL, 9, -2'000'000'000LL}));
```

Same mechanics as [`Slate::fillnull(Duration)`](../slate/fillnull.md) — it also clears
intrinsic `NaT` lanes, not just validity-mask-flagged ones.

Example 4. Calling the `string_view` overload on a `Series`-kind stream throws — a kind
mismatch is a hard error here, not a silent no-op.

```cpp
auto mask = pluss::make_series(std::vector<bool>{true, false, true});
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}), mask);

try {
  auto out = stream.fillnull("nope");
} catch (const std::exception& e) {
  std::cout << std::string_view(e.what()).substr(0, 51);
}
```

### Output

```text
Slate::fillnull(string) is only valid for Note pay
```

## Notes

- `value`'s C++ type selects both the overload and the required payload kind — see the
  important callout above for what happens on a mismatch.
- Requires a defined stream; throws `"Stream: undefined handle"` on a default-constructed
  `Stream()`.
- Calling `fillnull()` on a stream with no `NULL` cells is a harmless no-op — it returns an
  equivalent stream unchanged.
- The thrown exception's `.what()` appends a native stack trace after the message text
  shown above; examples here show only the leading message for readability.

## See Also

- [`Stream::ffill`](ffill.md) / [`Stream::bfill`](bfill.md): fill using neighboring values
  instead of a fixed scalar.
- [`Stream::dropnull`](dropnull.md): remove `NULL` rows instead of filling them.
- [`Catalog::fillnull`](../catalog/fillnull.md): the whole-collection counterpart, which
  silently skips kind-mismatched streams instead of throwing.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ffill.html">
    <span class="label">Previous</span>
    <span class="title">ffill</span>
  </a>
  <a class="next-link" href="filter.html">
    <span class="label">Next</span>
    <span class="title">filter</span>
  </a>
</div>

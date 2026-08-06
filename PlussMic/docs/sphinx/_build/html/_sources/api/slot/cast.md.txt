<!-- pluss-hand-authored -->

# Slot::cast

[Back to Slot API Reference](../../slot_api_reference)

## Signatures

```cpp
SlotCastAccessor Slot::cast() const
SlotCastAccessor Slot::cast(const pluss::table::ThreadCastOptions& options) const
```

## What It Does

The entry point into **casting** — binds the slot and hands back a
`SlotCastAccessor`, documented in full on its own pages (see
[`SlotCastAccessor` API Reference](../../slot_cast_accessor_api_reference)):
`.series()`, `.note()`, `.timeline()`, `.timeindex()`. Casting behaves the
same regardless of how many values the slot holds, including exactly one
— see Example 3.

## Parameters

**options** (`pluss::table::ThreadCastOptions`, optional)
    `series_dtype` (default `std::nullopt`, meaning "infer"), `format`
    (a parse pattern for text-to-datetime casts), and `invalid` (how to
    handle unparseable input — `ParseInvalid::Raise` by default).

## Returns

`SlotCastAccessor`
    A binder for the current slot — call one of its methods to actually
    cast.

## Examples

Example 1. Cast a text slot of numeric-looking strings to `Series`.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("1"), pluss::make_note("2")});
auto out = slot.cast().series();
std::cout << std::get<pluss::Series>(out.at(0)).to_vec<std::int64_t>()[0];
```

### Output

```text
1
```

Example 2. Pass `options` to force a specific target dtype.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("1"), pluss::make_note("2")});
pluss::table::ThreadCastOptions options;
options.series_dtype = pluss::codem::ScalarType::Float64;
auto out = slot.cast(options).series();
std::cout << std::get<pluss::Series>(out.at(0)).to_vec<double>()[0];
```

### Output

```text
1
```

Example 3. A single-value slot casts the same way a multi-value one does.

```cpp
auto single = pluss::bundle::Slot({pluss::make_series(1)});
auto out = single.cast().note();
std::cout << std::get<pluss::Note>(out.at(0)).at(0);
```

### Output

```text
1
```

## Notes

- `cast()` alone does no work — it only binds the slot; call one of the
  accessor's methods to actually convert.
- The full accessor surface is documented separately — see
  [`SlotCastAccessor` API Reference](../../slot_cast_accessor_api_reference).

## See Also

- [`Slot::astype`](astype.md): a narrower shorthand for numeric-dtype
  casting only, built on `cast()`.
- [`Registry::cast`](../registry/cast.md): the equivalent entry point at
  the `Registry` level.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="at.html">
    <span class="label">Previous</span>
    <span class="title">at</span>
  </a>
  <a class="next-link" href="concat.html">
    <span class="label">Next</span>
    <span class="title">concat</span>
  </a>
</div>

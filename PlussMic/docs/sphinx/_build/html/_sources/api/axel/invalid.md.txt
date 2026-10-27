<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 423a508cf820 -->

# Axel::invalid

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Axel Axel::invalid() const
```

## What It Does

Returns a new axel where every field's values, in every registry frame,
are replaced by a boolean flag for whether each value is invalid — a
broader check than [`isnull()`](isnull.md) that also catches
domain-specific not-a-value markers (like floating-point `NaN`) that
aren't the sentinel null representation. Applies
[`Registry::invalid()`](../registry/invalid.md) to every frame via
[`map()`](map.md), which in turn applies
[`Slot::invalid()`](../slot/invalid.md) to every field.

## Parameters

`None`

## Returns

`Axel`
    A new axel with every field's values replaced by boolean
    invalid-flags.

## Examples

Example 1. A `NaN` value is flagged `invalid()` even though it's not
the sentinel null value `isnull()` looks for.

```cpp
auto r0 = pluss::scribble::Registry({{"id", pluss::scribble::Slot({pluss::make_series(1.0), pluss::make_series(std::nan(""))})}});
auto axel = pluss::scribble::Axel({r0});

auto out = axel.invalid();
auto vals = out.registry(0).fields("id");
std::cout << std::boolalpha << std::get<pluss::Series>(vals.at(0)).host<bool>()[0] << " "
          << std::get<pluss::Series>(vals.at(1)).host<bool>()[0];
```

### Output

```text
false true
```

## Notes

- `invalid()` is a strictly broader check than `isnull()` — every null
  value is also invalid, but not every invalid value (e.g. `NaN`) is a
  null. Full per-kind semantics are documented on
  [`Slot::invalid`](../slot/invalid.md).
- Never mutates the receiver — always returns a new `Axel`.

## See Also

- [`Axel::isnull`](isnull.md): the narrower, sentinel-null-only check.
- [`Axel::dropnull`](dropnull.md): removes nulls, not general invalid
  values.

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

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c81bb364b8f5 -->

# Axel::notnull

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Axel Axel::notnull() const
```

## What It Does

Returns a new axel where every field's values, in every registry frame,
are replaced by a boolean flag for whether each value is **not**
null — the logical complement of [`isnull()`](isnull.md). Applies
[`Registry::notnull()`](../registry/notnull.md) to every frame via
[`map()`](map.md).

## Parameters

`None`

## Returns

`Axel`
    A new axel with every field's values replaced by boolean
    not-null flags.

## Examples

Example 1. A present value reports `true`; a null value reports
`false`.

```cpp
auto r0 = pluss::bundle::Registry({{"id", pluss::bundle::Slot({pluss::make_series(1), pluss::bundle::Slot::Value{std::monostate{}}, pluss::make_series(3)})}});
auto axel = pluss::bundle::Axel({r0});

auto out = axel.notnull();
auto vals = out.registry(0).fields("id");
std::cout << std::boolalpha << std::get<pluss::Series>(vals.at(0)).to_vec<bool>()[0] << " "
          << std::get<pluss::Series>(vals.at(1)).to_vec<bool>()[0];
```

### Output

```text
true false
```

## Notes

- Never throws on a field whose kind doesn't match a fill payload — it
  works identically on every kind.
- Never mutates the receiver — always returns a new `Axel`.

## See Also

- [`Axel::isnull`](isnull.md): the logical complement.
- [`Axel::invalid`](invalid.md): a related but distinct check.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ndistinct.html">
    <span class="label">Previous</span>
    <span class="title">ndistinct</span>
  </a>
  <a class="next-link" href="nt.html">
    <span class="label">Next</span>
    <span class="title">nt</span>
  </a>
</div>

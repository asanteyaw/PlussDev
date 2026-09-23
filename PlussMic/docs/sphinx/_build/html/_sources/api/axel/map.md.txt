<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8443b83887ff -->

# Axel::map

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Axel Axel::map(const std::function<Registry(const Registry&)>& fn) const
```

## What It Does

Applies `fn` to each registry frame independently, returning a new
`Axel` built from the results. Every field-selection, null-handling,
and row-op method on `Axel` ([`fields()`](fields.md),
[`drop()`](drop.md), [`isnull()`](isnull.md), etc.) is itself
implemented as a call to `map()` with a specific per-frame `Registry`
operation — `map()` is the escape hatch for anything not already
covered by a dedicated method.

## Parameters

**fn** (`const std::function<Registry(const Registry&)>&`)
    Called once per frame, in depth order. Must be non-empty. Must
    return a *defined* `Registry` for every frame — an undefined result
    throws.

## Returns

`Axel`
    A new axel with the same depth and labels as the original, each
    frame replaced by `fn`'s result for it.

## Examples

Example 1. Narrow every frame down to just the `"ids"` field using
`map()` directly (the same operation [`fields()`](fields.md) performs
internally).

```cpp
auto axel = pluss::scribble::Axel({
    pluss::scribble::Registry({
        {"ids", pluss::scribble::Slot({pluss::make_series(1)})},
        {"city", pluss::scribble::Slot({pluss::make_note("Accra")})},
    }),
});
auto out = axel.map([](const pluss::scribble::Registry& reg) { return reg.fields({"ids"}); });
std::cout << out.registry(0).nfields();
```

### Output

```text
1
```

Example 2. An empty `fn` throws.

```cpp
auto axel = pluss::scribble::Axel({
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(1)})}}),
});
try {
  std::function<pluss::scribble::Registry(const pluss::scribble::Registry&)> fn;
  axel.map(fn);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Axel::map: fn is empty
```

## Notes

- `fn` must be non-empty (`"Axel::map: fn is empty"`) and must return a
  defined `Registry` for every frame
  (`"Axel::map: function returned undefined Registry"`) — returning a
  default-constructed `Registry{}` from `fn` throws.
- Requires `defined()` — an undefined handle throws
  `"Axel: undefined handle"`.
- Labels are carried through unchanged — `map()` never touches
  [`labels()`](labels.md), only the registry frames themselves.

## See Also

- [`Axel::fields`](fields.md), [`Axel::drop`](drop.md),
  [`Axel::isnull`](isnull.md): dedicated methods built on top of
  `map()` for common per-frame `Registry` operations.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="labels.html">
    <span class="label">Previous</span>
    <span class="title">labels</span>
  </a>
  <a class="next-link" href="ndistinct.html">
    <span class="label">Next</span>
    <span class="title">ndistinct</span>
  </a>
</div>

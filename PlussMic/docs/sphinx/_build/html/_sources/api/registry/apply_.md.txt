<!-- pluss-hand-authored -->

# Registry::apply_

[Back to Registry API Reference](../../registry_api_reference)

## Signatures

```cpp
Registry& Registry::apply_(std::string_view key, const std::function<Slot(const Slot&)>& fn)
Registry& Registry::apply_(std::span<const std::string_view> keys, const std::function<Slot(const Slot&)>& fn)
Registry& Registry::apply_(std::initializer_list<std::string_view> keys, const std::function<Slot(const Slot&)>& fn)
template <pluss::detail::StringViewNameRange NameRange>
Registry& Registry::apply_(const NameRange& keys, const std::function<Slot(const Slot&)>& fn)
```

## What It Does

The in-place counterpart to [`apply()`](apply.md) — runs `fn` over one or more
named fields' `Slot`s and rewrites this registry directly, rather than returning a
new one. Same overload shapes as `apply()`: a single key, or multiple keys via an
initializer list, a `span`, or any string-view-convertible range.

```{important} fn must return a defined Slot
Same requirement as `apply()`: an undefined `Slot` returned from `fn` throws
`"Registry::apply_: function returned undefined Slot"`.
```

## Parameters

**key** (`std::string_view`)
    A single field name to transform in place (single-key overload).

**keys** (`std::span<const std::string_view>` / `std::initializer_list<std::string_view>` / any string-view name range)
    Multiple field names to transform in place, each independently, with the same
    `fn`.

**fn** (`const std::function<Slot(const Slot&)>&`)
    A callback taking the current field's `Slot` and returning its replacement.
    Must return a defined `Slot`.

## Returns

`Registry&`
    A reference to this registry, now holding the transformed field(s).

## Examples

Example 1. Transform a single field in place.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
});

rg.apply_("id", [](const pluss::bundle::Slot& s) {
  return s.transform([](const pluss::bundle::Slot::Value& v) -> pluss::bundle::Slot::Value {
    return pluss::make_series(std::get<pluss::Series>(v).to_vec<std::int64_t>()[0] * 10);
  });
});

auto ids = rg.select("id");
std::cout << "[";
for (std::int64_t i = 0; i < ids.size(); ++i) {
  if (i) std::cout << ", ";
  std::cout << std::get<pluss::Series>(ids.at(i)).to_vec<std::int64_t>()[0];
}
std::cout << "]";
```

### Output

```text
[10, 20, 30]
```

## Notes

- Mutates the registry it's called on and returns `*this` by reference — chain
  further calls directly, or discard the return value if you only care about the
  mutation.
- `apply(key, fn)` is implemented as a copy followed by `apply_(key, fn)` on that
  copy — the two share identical transform semantics; only mutation-vs-copy
  differs.
- Same `fn`-must-return-defined-`Slot` requirement as [`apply()`](apply.md).

## See Also

- [`Registry::apply`](apply.md): the non-mutating counterpart, returning a new
  registry instead of rewriting this one.
- [`Registry::map_`](map_.md): the in-place, apply-to-every-field counterpart.
- [`Catalog::apply_`](../catalog/apply_.md): the equivalent operation on the
  kindle model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="apply.html">
    <span class="label">Previous</span>
    <span class="title">apply</span>
  </a>
  <a class="next-link" href="apply_all.html">
    <span class="label">Next</span>
    <span class="title">apply_all</span>
  </a>
</div>

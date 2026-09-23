<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d90c1404bb4c -->

# Registry::push

[Back to Registry API Reference](../../registry_api_reference)

## Signatures

```cpp
Registry Registry::push(std::string_view key, const Slot::Value& value) const
Registry Registry::push(const Slot::Value& value) const
```

## What It Does

Appends one value to the end of a field's `Slot`. The single-key overload
appends to just that one named field; the value-only overload appends the same
value to **every** field.

## Parameters

**key** (`std::string_view`)
    The field to append to (single-key overload). Throws if it doesn't exist.

**value** (`const Slot::Value&`)
    The value to append.

## Returns

`Registry`
    A new registry with the value appended — either to the named field only, or
    to every field.

## Examples

Example 1. Append to a single named field.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.push("id", pluss::make_series(4));
auto ids = out.fields("id");
std::cout << "[";
for (std::int64_t i = 0; i < ids.size(); ++i) {
  if (i) std::cout << ", ";
  std::cout << std::get<pluss::Series>(ids.at(i)).to_vec<std::int64_t>()[0];
}
std::cout << "]";
```

### Output

```text
[1, 2, 3, 4]
```

Example 2. Appending to an unknown field name throws.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1)})},
});

try {
  auto out = rg.push("zzz", pluss::make_series(4));
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Registry::push: unknown key 'zzz'
```

Example 3. The value-only overload appends the same value to every field.

```cpp
auto rg = pluss::scribble::Registry({
    {"a", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"b", pluss::scribble::Slot({pluss::make_series(10), pluss::make_series(20)})},
});

auto out = rg.push(pluss::make_series(99));
auto a = out.fields("a");
auto b = out.fields("b");
std::cout << std::get<pluss::Series>(a.at(2)).to_vec<std::int64_t>()[0] << " ";
std::cout << std::get<pluss::Series>(b.at(2)).to_vec<std::int64_t>()[0];
```

### Output

```text
99 99
```

## Notes

- The single-key overload throws `"Registry::push: unknown key '<name>'"` for a
  missing field — contrast with [`set()`](set.md), which upserts instead.
- The value-only overload is built on [`map()`](map.md), so it inherits the same
  "runs on every field, regardless of kind" behavior — appending a value of an
  incompatible kind to any field can throw.
- Never mutates the receiver — use [`push_()`](push_.md) for the in-place form.

## See Also

- [`Registry::push_`](push_.md): the in-place counterpart.
- [`Registry::concat`](concat.md): append a whole `Slot`'s worth of values at
  once, rather than a single value.
- [`Registry::set`](set.md): replace a field's entire `Slot`, rather than
  appending to it.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="prune_.html">
    <span class="label">Previous</span>
    <span class="title">prune_</span>
  </a>
  <a class="next-link" href="push_.html">
    <span class="label">Next</span>
    <span class="title">push_</span>
  </a>
</div>

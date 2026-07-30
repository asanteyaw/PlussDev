<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1a8883538442 -->

# Catalog::set_

[Back to Catalog API Reference](../../catalog_api_reference)

## Signatures

```cpp
Catalog::set_(std::string_view key, const Stream& value) -> Catalog&
Catalog::set_(std::initializer_list<std::pair<std::string_view, Stream>> items) -> Catalog&
Catalog::set_(std::initializer_list<NamedStream> items) -> Catalog&
```

## What It Does

The in-place counterpart to [`set()`](set.md) — mutates `*this` directly (adding a new key
or overwriting an existing one) and returns a reference to it, rather than returning a new
`Catalog`. Same upsert semantics and the same `"key"_k = stream` ambiguity caveat as
`set()` apply here unchanged.

## Parameters

**key** (`std::string_view`)
    The stream name to add or overwrite (single-item overload).

**value** (`const Stream&`)
    The stream to store under `key` (single-item overload).

**items** (`std::initializer_list<std::pair<std::string_view, Stream>>` or
`std::initializer_list<NamedStream>`)
    Several `key -> stream` entries to add or overwrite in one call.

## Returns

`Catalog&`
    A reference to `*this`, after the update.

## Examples

Example 1. Add a new stream in place; the original catalog variable reflects the change
immediately.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
});

ct.set_("flag", pluss::kindle::Stream(pluss::make_series({1.0, 0.0, 1.0})));
std::cout << ct;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [id, flag]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 3       | [1, 2, 3]                                        
flag                     | Series       | Series[float64]  | 3       | [1, 0, 1]                                        
========================================================================================================================
```

## Notes

- Same upsert semantics as [`set()`](set.md) — adds a new key or overwrites an existing
  one, never throws on an unknown key.
- Prefer `"key"_k = stream` literals for multi-item calls; plain `{"key", stream}` braces
  are ambiguous between the two multi-item overloads — see [`set()`](set.md)'s important
  callout.
- Mutates the receiver directly — no assignment back is needed, unlike the non-mutating
  form.

## See Also

- [`Catalog::set`](set.md): the non-mutating form, with the full ambiguity explanation.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="set.html">
    <span class="label">Previous</span>
    <span class="title">set</span>
  </a>
  <a class="next-link" href="shape.html">
    <span class="label">Next</span>
    <span class="title">shape</span>
  </a>
</div>

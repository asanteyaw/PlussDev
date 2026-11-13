<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3685d9f2e68b -->

# Archive::defined

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
bool Archive::defined() const noexcept
```

## What It Does

Reports whether the archive owns a concrete payload. A default-constructed `Archive()` has
none — `defined()` returns `false`, and calling most other methods on it (e.g.
[`depth()`](depth.md)) throws `"Archive: undefined handle"`. An archive built with zero
catalogs (`Archive(std::vector<Catalog>{})`) is still defined — it just has
[`depth()`](depth.md) `0`.

## Parameters

`None`

## Returns

`bool`
    `true` if the archive has a concrete payload; `false` for a default-constructed
    `Archive()`.

## Examples

Example 1. An archive built from catalogs is defined.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
});
auto c1 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({4.0, 5.0, 6.0})),
});
auto archive = pluss::kindle::Archive({c0, c1});

std::cout << archive.defined();
```

### Output

```text
1
```

Example 2. A default-constructed archive is not — and calling other methods on it throws.

```cpp
pluss::kindle::Archive undef;
std::cout << "defined: " << undef.defined() << "\n";

try {
  undef.depth();
} catch (const std::exception& e) {
  std::cout << std::string_view(e.what()).substr(0, 25);
}
```

### Output

```text
defined: 0
Archive: undefined handle
```

## Notes

- `noexcept` and safe to call on any `Archive`, including a default-constructed one.
- An archive built with zero catalogs is still defined; `defined()` and
  [`empty()`](empty.md) answer different questions — see [`empty()`](empty.md).

## See Also

- [`Archive::empty`](empty.md): a different check — a *defined* archive with zero depth.
- [`Archive::depth`](depth.md): throws the same `"Archive: undefined handle"` message when
  called on an undefined archive.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="catalogs.html">
    <span class="label">Previous</span>
    <span class="title">catalogs</span>
  </a>
  <a class="next-link" href="depth.html">
    <span class="label">Next</span>
    <span class="title">depth</span>
  </a>
</div>

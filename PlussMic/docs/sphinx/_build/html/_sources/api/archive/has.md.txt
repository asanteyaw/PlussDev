<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 63e705043173 -->

# Archive::has

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive::has(std::string_view key) const -> bool
```

## What It Does

Reports whether `key` exists **in every catalog** in the archive.

```{important} has() is an AND across every catalog, not "at least one"
On a ragged archive where some catalogs have `key` and others don't, `has(key)` returns
`false` — it requires universal presence, not mere existence somewhere in the archive. A
key present in only some catalogs is exactly the situation this method is built to catch.
```

## Parameters

**key** (`std::string_view`)
    The stream name to check for.

## Returns

`bool`
    `true` only if every catalog in the archive has a stream named `key`; `false`
    otherwise.

## Examples

Example 1. A key present in every catalog.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({1.0})) });
auto c1 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({2.0})) });
auto archive = pluss::kindle::Archive({c0, c1});

std::cout << archive.has("id");
```

### Output

```text
1
```

Example 2. A key present in only one of two catalogs — `has()` returns `false`.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra"})),
});
auto c1 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({2.0})) });
auto ragged = pluss::kindle::Archive({c0, c1});

std::cout << "has(\"city\"): " << ragged.has("city") << "\n";
std::cout << "has(\"id\"): " << ragged.has("id");
```

### Output

```text
has("city"): 0
has("id"): 1
```

## Notes

- Requires the key in *every* catalog — see the important callout above.
- Never throws for a missing or empty key; always returns a plain `bool`.
- Requires a defined archive; throws `"Archive: undefined handle"` on a default-constructed
  `Archive()`.

## See Also

- [`Archive::keys`](keys.md): select a key once you've confirmed it exists everywhere.
- [`Archive::minkeys`](minkeys.md) / [`Archive::maxkeys`](maxkeys.md): a broader ragged-key
  diagnostic when several keys might be inconsistent across catalogs.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="freq.html">
    <span class="label">Previous</span>
    <span class="title">freq</span>
  </a>
  <a class="next-link" href="head.html">
    <span class="label">Next</span>
    <span class="title">head</span>
  </a>
</div>

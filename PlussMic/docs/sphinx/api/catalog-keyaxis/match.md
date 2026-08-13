<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0801a50b1e05 -->

# CatalogKeyAxisAccessor::match

[Back to CatalogKeyAxisAccessor API Reference](../../catalog_keyaxis_accessor_api_reference)

## Signature

```cpp
Catalog CatalogKeyAxisAccessor::match(std::string_view pattern) const
```

## What It Does

Returns a new catalog containing only the keys whose **name** matches
`pattern` as an ECMAScript regular expression (`std::regex_search`, so the
pattern needs to match somewhere in the name, not the whole name — anchor
with `^`/`$` if you need a full match).

## Parameters

**pattern** (`std::string_view`)
    An ECMAScript-flavor regular expression to search each key name against.

## Returns

`Catalog`
    A new catalog containing only the keys whose name matches `pattern`.

## Examples

Example 1. Select every key name ending in `_id`.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})),
    "user_id"_k = pluss::kindle::Stream(pluss::make_series({3.0, 4.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema"})),
});
auto out = ct.keyaxis().match("^.*_id$");
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [user_id]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
user_id                  | Series       | Series[float64]  | 2       | [3, 4]                                           
========================================================================================================================
```

Example 2 (edge case). An invalid regex pattern throws.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0})),
});
try {
  auto out = ct.keyaxis().match("(unterminated");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Catalog::keyaxis().match: invalid regex '(unterminated': The expression contained mismatched ( and ).
```

## Notes

- Uses `std::regex_search`, not `std::regex_match` — the pattern needs to
  appear somewhere in the name unless you anchor it with `^`/`$`.
- An invalid regex throws immediately, wrapping the underlying
  `std::regex_error` message.

## See Also

- [`CatalogKeyAxisAccessor::like`](like.md): the simpler, plain-substring
  counterpart.
- [`CatalogKeyAxisAccessor::prefix`](prefix.md) / [`suffix()`](suffix.md):
  anchored substring searches, without needing regex syntax.
- [`Catalog::keyaxis`](../catalog/keyaxis.md): the entry point this accessor
  is reached from.
- [`RegistryFieldAxisAccessor::match`](../registry-fieldaxis/match.md): the
  bundle-model equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="like.html">
    <span class="label">Previous</span>
    <span class="title">like</span>
  </a>
  <a class="next-link" href="note.html">
    <span class="label">Next</span>
    <span class="title">note</span>
  </a>
</div>

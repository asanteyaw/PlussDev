<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5e03d30db172 -->

# RegistryFieldAxisAccessor::match

[Back to RegistryFieldAxisAccessor API Reference](../../registry_fieldaxis_accessor_api_reference)

## Signature

```cpp
Registry RegistryFieldAxisAccessor::match(std::string_view pattern) const
```

## What It Does

Returns a new registry containing only the fields whose **name** matches
`pattern` as an ECMAScript regular expression (`std::regex_search`, so the
pattern needs to match somewhere in the name, not the whole name — anchor with
`^`/`$` if you need a full match).

## Parameters

**pattern** (`std::string_view`)
    An ECMAScript-flavor regular expression to search each field name against.

## Returns

`Registry`
    A new registry containing only the fields whose name matches `pattern`.

## Examples

Example 1. Select every field name ending in `_id`.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"user_id", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})},
});

auto out = rg.fieldaxis().match("^.*_id$");
for (const auto& key : out.fields()) std::cout << key << " ";
```

### Output

```text
user_id
```

Example 2. An invalid regex pattern throws.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1)})},
});

try {
  auto out = rg.fieldaxis().match("(unterminated");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Registry::fieldaxis().match: invalid regex '(unterminated': The expression contained mismatched ( and ).
```

## Notes

- Uses `std::regex_search`, not `std::regex_match` — the pattern needs to
  appear somewhere in the name unless you anchor it with `^`/`$`.
- An invalid regex throws immediately, wrapping the underlying
  `std::regex_error` message.

## See Also

- [`RegistryFieldAxisAccessor::like`](like.md): the simpler, plain-substring
  counterpart.
- [`RegistryFieldAxisAccessor::prefix`](prefix.md) / [`RegistryFieldAxisAccessor::suffix`](suffix.md):
  anchored substring searches, without needing regex syntax.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="like.html">
    <span class="label">Previous</span>
    <span class="title">like</span>
  </a>
  <a class="next-link" href="mixed.html">
    <span class="label">Next</span>
    <span class="title">mixed</span>
  </a>
</div>

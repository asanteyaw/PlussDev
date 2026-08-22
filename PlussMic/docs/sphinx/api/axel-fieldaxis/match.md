<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ee8351438f4a -->

# AxelFieldAxisAccessor::match

[Back to AxelFieldAxisAccessor API Reference](../../axel_fieldaxis_accessor_api_reference)

## Signature

```cpp
Axel AxelFieldAxisAccessor::match(std::string_view pattern) const
```

## What It Does

Returns a new axel where every registry frame is narrowed to only the
fields whose **name** matches `pattern` as an ECMAScript regular
expression (`std::regex_search`, so the pattern needs to match
somewhere in the name, not the whole name — anchor with `^`/`$` if you
need a full match). Applies
[`RegistryFieldAxisAccessor::match`](../registry-fieldaxis/match.md) to
every frame via [`Axel::map`](../axel/map.md).

## Parameters

**pattern** (`std::string_view`)
    An ECMAScript-flavor regular expression to search each field name
    against.

## Returns

`Axel`
    A new axel with every frame narrowed to fields whose name matches
    `pattern`.

## Examples

Example 1. Select every field name ending in `_id`, in every frame.

```cpp
auto r0 = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"user_id", pluss::scribble::Slot({pluss::make_series(3), pluss::make_series(4)})},
    {"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})},
});
auto axel = pluss::scribble::Axel({r0, r0});

auto out = axel.fieldaxis().match("^.*_id$");
for (const auto& key : out.registry(0).fields()) std::cout << key << " ";
```

### Output

```text
user_id
```

Example 2. An invalid regex pattern throws — the same message
[`Registry::fieldaxis().match()`](../registry-fieldaxis/match.md)
would throw for that frame.

```cpp
auto r0 = pluss::scribble::Registry({{"id", pluss::scribble::Slot({pluss::make_series(1)})}});
auto axel = pluss::scribble::Axel({r0});

try {
  axel.fieldaxis().match("(unterminated");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Registry::fieldaxis().match: invalid regex '(unterminated': The expression contained mismatched ( and ).
```

## Notes

- Uses `std::regex_search`, not `std::regex_match` — the pattern needs
  to appear somewhere in the name unless you anchor it with `^`/`$`.
- An invalid regex throws immediately on the first frame it's applied
  to, wrapping the underlying `std::regex_error` message — the thrown
  message still reads `"Registry::fieldaxis().match: ..."` since it
  propagates unchanged from the per-frame call.

## See Also

- [`AxelFieldAxisAccessor::like`](like.md): the simpler, plain-substring
  counterpart.
- [`AxelFieldAxisAccessor::prefix`](prefix.md) / [`suffix()`](suffix.md):
  anchored substring searches, without needing regex syntax.
- [`RegistryFieldAxisAccessor::match`](../registry-fieldaxis/match.md):
  the single-frame operation this method broadcasts.

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

<!-- pluss-hand-authored -->

# Registry::ensure_stream_compatible

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
void Registry::ensure_stream_compatible(std::string_view where = "Registry") const
```

## What It Does

Validates that every field (slot) in the registry holds a single, uniform value
kind — throwing on the first slot that doesn't. This is the same check
[`to_kindle()`](to_kindle.md) and every `save_*` method except
[`save_json()`](save_json.md) perform internally before converting; call it directly
when you want to validate up front, with a custom error-message prefix, rather than
letting a later operation surface the failure.

## Parameters

**where** (`std::string_view`, optional, defaults to `"Registry"`)
    A label prepended to the throw message, identifying which operation triggered
    the check. Passing an empty string falls back to `"Registry"` rather than
    producing a blank prefix.

## Returns

`void`
    Does nothing and returns normally if every field is stream-compatible; throws
    otherwise.

## Examples

Example 1. A registry where every field holds one uniform kind passes silently.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.ensure_stream_compatible("MyOp");
std::cout << "no throw";
```

### Output

```text
no throw
```

Example 2. A field with genuinely mixed value kinds throws, naming both the
supplied label and the offending field.

```cpp
auto mixed = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"mixed", pluss::bundle::Slot({pluss::make_series(10), pluss::make_note("hello"), pluss::make_series(3.5)})},
});

try {
  mixed.ensure_stream_compatible("MyOp");
} catch (const std::exception& e) {
  std::cout << e.what() << "\n";
}

try {
  mixed.ensure_stream_compatible("");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
MyOp: slot 'mixed': slot is not stream-compatible (kind=Mixed)
Registry: slot 'mixed': slot is not stream-compatible (kind=Mixed)
```

## Notes

- Checks fields in field order and throws on the **first** incompatible one — the
  message names only that field, not every incompatible field in the registry.
- An empty `where` string falls back to the same `"Registry"` prefix as the default
  argument, rather than producing a message with no label — see Example 2.
- This is the same check `to_kindle()` and every `save_*` method except
  `save_json()` perform internally; see
  [`Registry::save_csv`](save_csv.md)'s important callout for how the throw message
  looks when triggered indirectly.

## See Also

- [`Registry::is_stream_compatible`](is_stream_compatible.md): the non-throwing
  boolean form of this same check.
- [`Registry::to_kindle`](to_kindle.md): the conversion this check guards.
- [`Slot::ensure_stream_compatible`](../slot/ensure_stream_compatible.md): the
  per-slot check this method calls for each field.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="end.html">
    <span class="label">Previous</span>
    <span class="title">end</span>
  </a>
  <a class="next-link" href="fieldaxis.html">
    <span class="label">Next</span>
    <span class="title">fieldaxis</span>
  </a>
</div>

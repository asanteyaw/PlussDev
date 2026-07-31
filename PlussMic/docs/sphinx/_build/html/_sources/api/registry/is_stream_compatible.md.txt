<!-- pluss-hand-authored -->

# Registry::is_stream_compatible

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
bool Registry::is_stream_compatible() const
```

## What It Does

Reports whether **every** field (slot) in the registry holds a single, uniform
value kind — the non-throwing boolean form of
[`ensure_stream_compatible()`](ensure_stream_compatible.md). A `true` result means
[`to_kindle()`](to_kindle.md) and every `save_*` method except
[`save_json()`](save_json.md) will succeed on this registry; a `false` result means
they'll throw.

## Parameters

`None`

## Returns

`bool`
    `true` if every field is stream-compatible; `false` if any single field holds
    genuinely mixed value kinds.

## Examples

Example 1. A registry where every field holds one uniform kind.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::cout << std::boolalpha << rg.is_stream_compatible();
```

### Output

```text
true
```

Example 2. A field with genuinely mixed value kinds makes the whole registry
report `false`.

```cpp
auto mixed = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"mixed", pluss::bundle::Slot({pluss::make_series(10), pluss::make_note("hello"), pluss::make_series(3.5)})},
});

std::cout << std::boolalpha << mixed.is_stream_compatible();
```

### Output

```text
false
```

## Notes

- `true` here is a precondition for [`to_kindle()`](to_kindle.md),
  [`save_csv()`](save_csv.md), [`save_txt()`](save_txt.md),
  [`save_excel()`](save_excel.md), and [`save_parquet()`](save_parquet.md) to
  succeed without throwing.
- [`save_json()`](save_json.md) is the one method that tolerates `false` — it falls
  back to [`to_json()`](to_json.md)'s per-value output instead of throwing.
- Requires a defined registry — throws `"Registry: undefined handle"` on an
  undefined one.

## See Also

- [`Registry::ensure_stream_compatible`](ensure_stream_compatible.md): the throwing
  form of this same check, with a customizable error-message prefix.
- [`Registry::to_kindle`](to_kindle.md): the conversion this check guards.
- [`Registry::save_json`](save_json.md): the one saver that branches on this check
  instead of throwing when it's `false`.
- [`Slot::is_stream_compatible`](../slot/is_stream_compatible.md): the per-slot
  check this method combines across every field.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="invalid.html">
    <span class="label">Previous</span>
    <span class="title">invalid</span>
  </a>
  <a class="next-link" href="isnull.html">
    <span class="label">Next</span>
    <span class="title">isnull</span>
  </a>
</div>

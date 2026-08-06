<!-- pluss-hand-authored -->

# SlotCastAccessor::timeline

[Back to SlotCastAccessor API Reference](../../slot_cast_accessor_api_reference)

## Signature

```cpp
Slot SlotCastAccessor::timeline() const
```

## What It Does

Casts every value in the bound slot to `Timeline`-kind and returns a new
slot. This behaves identically regardless of how many values the slot
holds — including exactly one — see Example 2.

## Parameters

`None`

## Returns

`Slot`
    A new slot, every value cast to `Timeline`-kind.

## Examples

Example 1. Cast a two-value `Note` slot of date-looking text to
`Timeline`.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("2024-01-01"), pluss::make_note("2024-01-02")});
auto out = slot.cast().timeline();
std::cout << out.to_json();
```

### Output

```text
{
  "_type": "Slot",
  "defined": true,
  "size": 2,
  "values": [
    "Timeline[numel=1]",
    "Timeline[numel=1]"
  ]
}
```

Example 2. A single-value slot converts the same way a multi-value one
does.

```cpp
auto single = pluss::bundle::Slot({pluss::make_note("2024-01-01")});
auto out = single.cast().timeline();
std::cout << out.to_json();
```

### Output

```text
{
  "_type": "Slot",
  "defined": true,
  "size": 1,
  "values": [
    "Timeline[numel=1]"
  ]
}
```

Example 3. On a two-or-more-value slot, unparseable text throws the
intended, documented error.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")});
try {
  auto out = slot.cast().timeline();
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
moment: failed to parse datetime
```

Example 4. A single-value slot with unparseable text throws the same
intended error, not an internal crash.

```cpp
auto single_bad = pluss::bundle::Slot({pluss::make_note("Accra")});
try {
  auto out = single_bad.cast().timeline();
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
moment: failed to parse datetime
```

## Notes

- Unparseable text throws `"moment: failed to parse datetime"` regardless
  of slot size — contrast [`series()`](series.md)'s silent-null behavior
  for the same kind of input.
- Bound via [`Slot::cast()`](../slot/cast.md).

## See Also

- [`SlotCastAccessor::timeindex`](timeindex.md): the related cast target,
  with the same parse-failure behavior.
- [`SlotCastAccessor::series`](series.md): the numeric cast target, with a
  different (silent-null) failure mode.
- [`Slot::cast`](../slot/cast.md): the entry point this accessor is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="timeindex.html">
    <span class="label">Previous</span>
    <span class="title">timeindex</span>
  </a>
</div>

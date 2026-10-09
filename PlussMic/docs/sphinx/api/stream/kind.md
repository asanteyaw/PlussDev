<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6fe1eb126b2e -->

# Stream::kind

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream::kind() const -> StreamKind
```

## What It Does

Returns which payload kind the stream currently stores, as a `StreamKind` enum value:

```cpp
enum class StreamKind : std::uint8_t {
  Series = 0,
  Note = 1,
  Timeline = 2,
  TimeIndex = 3,
};
```

```{important} StreamKind has no operator<< — cast to int or use ptype() for text
`StreamKind` is a plain `enum class` with no stream-insertion operator defined.
`std::cout << stream.kind()` does not compile. Use `static_cast<int>(stream.kind())` for
the numeric value, or [`ptype()`](ptype.md) for a human-readable string ("Series", "Note",
"Timeline", "TimeIndex").
```

## Parameters

`None`

## Returns

`StreamKind`
    The payload kind — one of `Series`, `Note`, `Timeline`, `TimeIndex`.

## Examples

Example 1. Compare `kind()` against a specific enumerator, and print it via `static_cast`.

```cpp
using pluss::kindle::StreamKind;

auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

std::cout << "is Series: " << (stream.kind() == StreamKind::Series) << "\n";
std::cout << "as int: " << static_cast<int>(stream.kind());
```

### Output

```text
is Series: 1
as int: 0
```

Example 2. For a human-readable name, use [`ptype()`](ptype.md) instead of `kind()`.

```cpp
auto note = pluss::kindle::Stream(pluss::make_note({"a", "b"}));

std::cout << "kind as int: " << static_cast<int>(note.kind()) << "\n";
std::cout << "ptype: " << note.ptype();
```

### Output

```text
kind as int: 1
ptype: Note
```

## Notes

- No `operator<<` — see the important callout above; this trips up a direct
  `std::cout << stream.kind()` with a wall of overload-resolution errors.
- Requires a defined stream ([`defined()`](defined.md)); throws `"Stream: undefined
  handle"` otherwise.
- `StreamKind`'s four enumerators map 1:1 with `Slate`'s own payload kinds — a `Stream`
  always wraps exactly one of `Series`/`Note`/`Timeline`/`TimeIndex`.

## See Also

- [`Stream::ptype`](ptype.md): the human-readable string counterpart to this enum.
- [`Stream::dtype`](dtype.md): a more specific string, including numeric sub-type for
  `Series` (e.g. `"Series[float64]"`).
- [`Stream::cast`](cast.md) / [`Stream::astype`](astype.md): convert between kinds.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isnull.html">
    <span class="label">Previous</span>
    <span class="title">isnull</span>
  </a>
  <a class="next-link" href="label.html">
    <span class="label">Next</span>
    <span class="title">label</span>
  </a>
</div>

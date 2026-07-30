<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7f0e0a23d2be -->

# Stream::ptype

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream::ptype() const -> std::string
```

## What It Does

Returns the stream's payload kind as a human-readable string — the same information as
[`kind()`](kind.md), but rendered as text (`"Series"`, `"Note"`, `"Timeline"`,
`"TimeIndex"`) instead of an enum value. Unlike `dtype()`, it never includes a numeric
sub-type: `"Series"`, not `"Series[float64]"`.

## Parameters

`None`

## Returns

`std::string`
    One of `"Series"`, `"Note"`, `"Timeline"`, `"TimeIndex"`.

## Examples

Example 1. `ptype()` for a numeric stream is plain `"Series"`, regardless of numeric
sub-type.

```cpp
auto float_stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0}));
auto int_stream = pluss::kindle::Stream(pluss::make_series(std::vector<std::int64_t>{1, 2, 3}));

std::cout << "float: " << float_stream.ptype() << "\n";
std::cout << "int: " << int_stream.ptype();
```

### Output

```text
float: Series
int: Series
```

Example 2. `ptype()` across the other three payload kinds.

```cpp
auto note = pluss::kindle::Stream(pluss::make_note({"a", "b"}));
auto timeline = pluss::kindle::Stream(pluss::make_timeline({"2024-01-01T00:00:00Z"}));

std::cout << "note: " << note.ptype() << "\n";
std::cout << "timeline: " << timeline.ptype();
```

### Output

```text
note: Note
timeline: Timeline
```

## Notes

- Requires a defined stream ([`defined()`](defined.md)); throws `"Stream: undefined
  handle"` otherwise.
- Never carries numeric sub-type information — both `int64` and `float64` `Series` streams
  report plain `"Series"`; use [`dtype()`](dtype.md) if you need that distinction.

## See Also

- [`Stream::kind`](kind.md): the enum counterpart to this string, with no `operator<<`.
- [`Stream::dtype`](dtype.md): a more specific string, including numeric sub-type for
  `Series` streams.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nrecords.html">
    <span class="label">Previous</span>
    <span class="title">nrecords</span>
  </a>
  <a class="next-link" href="shape.html">
    <span class="label">Next</span>
    <span class="title">shape</span>
  </a>
</div>

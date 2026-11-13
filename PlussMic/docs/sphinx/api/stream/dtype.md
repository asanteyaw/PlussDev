<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1452420d0fec -->

# Stream::dtype

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
std::string Stream::dtype() const
```

## What It Does

Returns a more specific type string than [`ptype()`](ptype.md): for a `Series`-kind
stream, it includes the numeric scalar sub-type (`"Series[float64]"`, `"Series[int64]"`,
...); for the other three kinds, it's identical to `ptype()` (`"Note"`, `"Timeline"`,
`"TimeIndex"`), since those don't carry a further numeric sub-type.

## Parameters

`None`

## Returns

`std::string`
    `"Series[<scalar_dtype>]"` for a `Series`-kind stream; `"Note"`/`"Timeline"`/
    `"TimeIndex"` otherwise.

## Examples

Example 1. `dtype()` distinguishes numeric sub-type where `ptype()` cannot.

```cpp
auto float_stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0}));
auto int_stream = pluss::kindle::Stream(pluss::make_series(std::vector<std::int64_t>{1, 2, 3}));

std::cout << "float: " << float_stream.dtype() << "\n";
std::cout << "int: " << int_stream.dtype();
```

### Output

```text
float: Series[float64]
int: Series[int64]
```

Example 2. For non-`Series` kinds, `dtype()` and `ptype()` agree exactly.

```cpp
auto note = pluss::kindle::Stream(pluss::make_note({"a", "b"}));

std::cout << "dtype: " << note.dtype() << "\n";
std::cout << "ptype: " << note.ptype();
```

### Output

```text
dtype: Note
ptype: Note
```

## Notes

- Requires a defined stream ([`defined()`](defined.md)); throws `"Stream: undefined
  handle"` otherwise.
- Only diverges from [`ptype()`](ptype.md) for `Series`-kind streams, where it appends the
  bracketed scalar dtype.

## See Also

- [`Stream::ptype`](ptype.md): the coarser payload-kind string, without numeric sub-type.
- [`Stream::kind`](kind.md): the enum this string ultimately describes.
- [`Stream::astype`](astype.md): change a `Series` stream's numeric sub-type, which changes
  what this method reports.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dropnull.html">
    <span class="label">Previous</span>
    <span class="title">dropnull</span>
  </a>
  <a class="next-link" href="empty.html">
    <span class="label">Next</span>
    <span class="title">empty</span>
  </a>
</div>

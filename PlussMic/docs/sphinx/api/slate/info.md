<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0427c81c9d26 -->

# Slate::info

[Back to Slate API Reference](../../slate_api_reference)

## Signatures

```cpp
Slate::info() const -> void
Slate::info(std::ostream& os) const -> void
```

## What It Does

Writes the same metadata summary [`info_string()`](info_string.md) builds — name, payload
kind, dtype, row count, valid/null breakdown — straight to a stream, with a trailing
newline appended. `info()` (no arguments) writes to `std::cout`; `info(os)` writes to
whatever stream you provide. Both are pure conveniences over
`os << slate.info_string() << '\n'`.

## Parameters

**os** (`std::ostream&`, `info(os)` overload only)
    The destination stream. Defaults to `std::cout` for the no-argument overload.

## Returns

`void`
    Writes to the stream as a side effect; produces no return value.

## Examples

Example 1. `info()` with no arguments writes the summary to `std::cout`.

```cpp
using namespace pluss::table;

auto scores = Slate(pluss::make_series({10.0, 20.0, 30.0}));

scores.info();
```

### Output

```text
Slate Info
Name: <unnamed>
Kind: Series
Dtype: float64
Cases: 3
Non-Null Cases: 3
Null Cases: 0
```

Example 2. `info(os)` writes to any stream — useful for capturing the summary into a
`std::ostringstream`, a log file stream, or similar, rather than the console.

```cpp
auto scores = Slate(pluss::make_series({10.0, 20.0, 30.0}));

std::ostringstream captured;
scores.info(captured);
std::cout << "[" << captured.str() << "]";
```

### Output

```text
[Slate Info
Name: <unnamed>
Kind: Series
Dtype: float64
Cases: 3
Non-Null Cases: 3
Null Cases: 0
]
```

## Notes

- Both overloads are a thin convenience over
  [`info_string()`](info_string.md): `os << slate.info_string() << '\n'`.
- Unlike [`info_string()`](info_string.md)'s returned string, the stream output *does* end
  with a trailing newline.
- Metadata only — like `info_string()`, this never renders the slate's actual row values.
  Use [`show()`](show.md) for that.

## See Also

- [`Slate::info_string`](info_string.md): the same summary, returned as a `std::string`
  instead of written to a stream — see that page for the exact field-by-field format.
- [`Slate::show`](show.md): render the slate's actual row values to a stream, rather than
  metadata.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="indicate.html">
    <span class="label">Previous</span>
    <span class="title">indicate</span>
  </a>
  <a class="next-link" href="info_string.html">
    <span class="label">Next</span>
    <span class="title">info_string</span>
  </a>
</div>

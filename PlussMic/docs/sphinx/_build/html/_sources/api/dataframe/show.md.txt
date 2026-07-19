<!-- pluss-hand-authored -->

# DataFrame::show

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::show(const display::DisplayOptions& options = {}) const -> void
DataFrame::show(std::ostream& os, const display::DisplayOptions& options = {}) const -> void
```

## What It Does

Prints the frame's rendered text — the exact same box-table text
`operator<<`/[`to_string()`](to_string.md) produce — with a trailing newline
appended. The two overloads differ in where the output goes and, subtly, in whether
plain text is guaranteed at all:

- **`show(os, options)`** (explicit stream): always writes plain text —
  `os << render(*this, options) << '\n'` — to whatever stream you pass. This is the
  deterministic, always-plain-text overload.
- **`show(options)`** (no stream): resolves a display *backend* first. In a plain
  terminal context, this also prints plain text to `std::cout`. But if a GUI/web
  viewer backend is resolved instead (depending on `options.mode`/`options.backend`
  and the environment), it opens a native or HTML viewer window and **does not**
  print anything to `std::cout` at all.

## Parameters

**options** (`const display::DisplayOptions&`, default `{}`)

Rendering and backend options — terminal width/height overrides, footer/case-index
visibility toggles, HTML viewer sizing, and a window title. `options.mode` and
`options.backend` are what decide whether `show(options)` (no stream) prints plain
text or opens a viewer window.

**os** (`std::ostream&`)

The destination stream for the explicit-stream overload. Always receives plain text
regardless of any backend setting.

## Returns

`void`
    Performs the print (or opens a viewer window); no value is returned.

## Examples

Example 1. Print a frame's rendered text to an explicit stream.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",    Thread(pluss::make_series({1, 2, 3}))},
    {"sales", Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city",  Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

std::ostringstream captured;
orders.show(captured);
std::cout << captured.str();
```

### Output

```text
==========================
     | id    sales  city  
--------------------------
0    | 1     100    Accra 
1    | 2     120    Tema  
2    | 3     90     Kumasi
--------------------------
[3 cs x 3 feat]

```

The captured text is identical to what `std::cout << orders` or
`orders.to_string()` would produce, with one trailing newline appended after the
`[3 cs x 3 feat]` footer line.

Example 2. `show()` with no stream is equivalent to `std::cout << orders` in a plain
terminal context.

```cpp
orders.show();  // same rendered text as std::cout << orders, in a terminal context
```

## Notes

- `show(os, options)` is the reliable, deterministic overload for capturing or
  redirecting output — it never opens a viewer window regardless of `options`.
- `show(options)` (no stream) may open a GUI or HTML viewer window instead of
  printing to `std::cout`, depending on the resolved display backend — do not rely on
  it always producing text on `std::cout` in every environment.
- `to_string()` returns the identical rendered text as a `std::string`, without any
  of `show()`'s stream-writing or backend-resolution behavior — reach for it when you
  want the text itself rather than a printing side effect.

## See Also

- [`DataFrame::to_string`](to_string.md): the same rendered text, returned as a
  `std::string` instead of printed.
- [`DataFrame::info`](info.md): a structural summary (dtypes, memory, null counts)
  printed the same way, instead of the full data table.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="shift.html">
    <span class="label">Previous</span>
    <span class="title">shift</span>
  </a>
  <a class="next-link" href="shuffle.html">
    <span class="label">Next</span>
    <span class="title">shuffle</span>
  </a>
</div>

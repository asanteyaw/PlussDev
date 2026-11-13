<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 14cfaba3a439 -->

# DataFrame::to_string

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
std::string DataFrame::to_string(const display::DisplayOptions& options = {}) const
```

## What It Does

Returns the frame's rendered box-table text as a `std::string` — the exact same
text `operator<<` writes when you do `std::cout << frame`, and the same text
[`show()`](show.md)'s explicit-stream overload writes plus a trailing newline. Reach
for `to_string()` specifically when you want the rendered text itself (to log it,
compare it in a test, embed it in another string, and so on) rather than printing it
as a side effect.

## Parameters

**options** (`const display::DisplayOptions&`, default `{}`)

Rendering options — terminal width/height overrides, and toggles for the shape
footer (`show_shape_footer`) and the leftmost case-index column
(`show_case_index`). Backend/window-related fields on `DisplayOptions` have no
effect on this method, since it always returns plain text.

## Returns

`std::string`
    The frame's rendered box-table text, with no trailing newline.

## Examples

Example 1. Get the rendered text and inspect it directly.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",    Slate(pluss::make_series({1, 2, 3}))},
    {"sales", Slate(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city",  Slate(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

std::string rendered = orders.to_string();
std::cout << rendered;
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

`rendered` is identical, character for character, to what `std::cout << orders`
would print directly — `to_string()` just hands you that same text as a value
instead of writing it anywhere.

Example 2. Confirm it matches `operator<<`'s output exactly.

```cpp
std::ostringstream via_stream;
via_stream << orders;
std::cout << (orders.to_string() == via_stream.str()) << "\n";
```

### Output

```text
true
```

## Notes

- `to_string()` never has a trailing newline; `std::cout << frame` also doesn't add
  one, but [`show()`](show.md) does.
- `options`'s backend/window fields (`mode`, `backend`, viewer sizing, window title)
  have no effect here — `to_string()` always produces plain text, unlike
  [`show()`](show.md)'s no-stream overload, which may resolve to a GUI/HTML viewer
  instead.
- [`info_string()`](info_string.md) is the analogous method for the structural
  summary view (dtypes, memory, null counts) instead of the full data table.

## See Also

- [`DataFrame::show`](show.md): prints this same rendered text as a side effect,
  rather than returning it.
- [`DataFrame::info_string`](info_string.md): the structural-summary counterpart to
  this method.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tl.html">
    <span class="label">Previous</span>
    <span class="title">tl</span>
  </a>
  <a class="next-link" href="top.html">
    <span class="label">Next</span>
    <span class="title">top</span>
  </a>
</div>

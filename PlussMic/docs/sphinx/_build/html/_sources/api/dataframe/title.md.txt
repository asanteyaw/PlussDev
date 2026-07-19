<!-- pluss-hand-authored -->

# DataFrame::title

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::title() const -> std::string
DataFrame::title(std::string title) const -> DataFrame
```

## What It Does

Reads or attaches a human-readable title string to the frame. The no-argument
overload reads the current title (an empty string if none was ever set); the
one-argument overload returns a **new** frame carrying `title` — the source frame is
never mutated. [`title_(title)`](title_.md) is the in-place counterpart, for setting
the title without producing a new frame.

A frame's title has no effect on its plain-text box-table rendering
(`operator<<`/[`to_string()`](to_string.md)) at all — it only surfaces as a window
title in [`show()`](show.md)'s GUI/HTML viewer backend, and is carried along
unchanged by most reshaping and selection operations elsewhere in this reference.

## Parameters

**title** (`std::string`, one-argument overload only)

The title text to attach to the returned frame.

## Returns

`std::string`
    (No-argument overload) The frame's current title, or an empty string if none has
    been set.

`DataFrame`
    (One-argument overload) A new frame, identical to the source in every other
    respect, carrying `title` as its title.

## Examples

Example 1. Attach a title to a frame, then read it back.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",    Thread(pluss::make_series({1, 2, 3}))},
    {"sales", Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});

auto titled = orders.title("sales_report");
std::cout << titled.title() << "\n";
```

### Output

```text
sales_report
```

Example 2. The title has no effect on the frame's own printed box table.

```cpp
std::cout << titled;
std::cout << orders.title() << "\n";
```

### Output

```text
==================
     | id    sales
------------------
0    | 1     100  
1    | 2     120  
2    | 3     90   
------------------
[3 cs x 2 feat]

```

`titled`'s box table is identical to what an untitled frame with the same data would
print — the title is metadata carried alongside the frame, not part of the rendered
table itself. `orders` (never titled) reads back an empty string from `title()`,
since `title("sales_report")` returned a new frame rather than mutating `orders`.

## Notes

- Setting a title never changes any value, dtype, feature order, or case label —
  it is pure metadata.
- The one-argument overload is non-mutating, like nearly every other method in this
  reference; use [`title_()`](title_.md) for the in-place equivalent.
- Most reshaping and selection methods elsewhere in this reference carry the
  source frame's title forward onto their result unchanged.

## See Also

- [`DataFrame::title_`](title_.md): the in-place counterpart, mutating the current
  frame's title directly.
- [`DataFrame::show`](show.md): where a frame's title actually becomes visible, as a
  GUI/HTML viewer window title.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tally.html">
    <span class="label">Previous</span>
    <span class="title">tally</span>
  </a>
  <a class="next-link" href="title_.html">
    <span class="label">Next</span>
    <span class="title">title_</span>
  </a>
</div>

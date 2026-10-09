<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 60ec50583ab2 -->

# DataFrame::set_

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame& DataFrame::set_(std::string_view name, const Slate& value)
DataFrame& DataFrame::set_(std::string_view name, const Slate& value, const AlignOptions& options)
DataFrame& DataFrame::set_(pluss::RangeView<std::string_view> names, pluss::RangeView<Slate> values)
```

## What It Does

Creates or overwrites the named feature(s), mutating `*this` directly and returning
a reference to it. This is the direct in-place form when you do not want the
assignment-target style from [`set()`](set.md).

## Parameters

**name** / **names**

The feature name (or names) to create or overwrite in place.

**value** / **values**

The replacement `Slate` (or a matching range of `Slate`s for the multi-name
overload).

**options** (`const AlignOptions&`, optional)

Controls how `value` is aligned to the frame's case axis when it doesn't already
match exactly, identical to [`set()`](set.md)'s `options`.

## Returns

`DataFrame&`
    A reference to `*this`, with the named feature(s) created or overwritten.

## Examples

Example 1. Overwrite an existing feature's values in place.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",    Slate(pluss::make_series({1, 2, 3}))},
    {"sales", Slate(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city",  Slate(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

orders.set_("sales", Slate(pluss::make_series({0.0, 0.0, 0.0})));
std::cout << orders;
```

### Output

```text
==========================
     | id    sales  city  
--------------------------
0    | 1     0      Accra 
1    | 2     0      Tema  
2    | 3     0      Kumasi
--------------------------
[3 cs x 3 feat]
```

`orders` itself has `sales` zeroed out — `set_()` mutated it directly.

Example 2. Create a brand-new feature in place via the multi-name overload.

```cpp
std::vector<std::string_view> names{"profit"};
std::vector<Slate> values{Slate(pluss::make_series({40.0, 35.0, 30.0}))};
orders.set_(pluss::RangeView<std::string_view>(names), pluss::RangeView<Slate>(values));
std::cout << orders;
```

### Output

```text
==================================
     | id    sales  city    profit
----------------------------------
0    | 1     0      Accra   40    
1    | 2     0      Tema    35    
2    | 3     0      Kumasi  30    
----------------------------------
[3 cs x 4 feat]
```

`"profit"` didn't previously exist on `orders`, so it's appended as a new feature —
`set_()` can both create and overwrite in the same call.

## Notes

- `set_()` mutates and returns `*this` by reference.
- Any other `DataFrame` value that shares the same underlying columns (via a prior
  copy) does not see this mutation — `pluss::table::DataFrame`'s copies are
  value-like, not aliased references to shared state.
- See [`set()`](set.md) for the complete parameter breakdown and for the separate
  `LetSlot`/`LetSlots` builder style (`df.set(name) = value`), which also mutates in
  place but is a distinct code path from `set_()`.

## See Also

- [`DataFrame::set`](set.md): the assignment-target builder style for in-place
  assignment via `operator=`.
- [`DataFrame::assign`](assign.md): derive new features from expressions evaluated
  against the frame, rather than assigning pre-built `Slate`s.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="set.html">
    <span class="label">Previous</span>
    <span class="title">set</span>
  </a>
  <a class="next-link" href="shape.html">
    <span class="label">Next</span>
    <span class="title">shape</span>
  </a>
</div>

<!-- pluss-hand-authored -->

# DataFrame::set

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::LetSlot DataFrame::set(std::string_view name)
DataFrame::LetSlot DataFrame::set(std::string_view name, const AlignOptions& options)
DataFrame::LetSlots DataFrame::set(pluss::RangeView<std::string_view> names)
DataFrame::LetSlots DataFrame::set(pluss::RangeView<std::string_view> names, const AlignOptions& options)
DataFrame::LetSlots DataFrame::set(std::initializer_list<std::string_view> names)
DataFrame::LetSlots DataFrame::set(std::initializer_list<const char*> names)
```

## What It Does

Adds a new feature or replaces an existing one's values through assignment-target
helpers. `set(name)` returns a lightweight `LetSlot` proxy bound to `*this`;
assigning a `Thread`, `ThreadValue`, `pluss::Series`/`Note`/`Timeline`/`TimeIndex`,
or a single-feature `DataFrame` to that proxy mutates the calling frame in place.
`set({"a", "b"}) = {...}` is the same idea for several features assigned together
in one batch. A `ThreadValue` right-hand side broadcasts a single scalar to every row.

[`set_(name, value)`](set_.md) is the direct one-line in-place counterpart when you
do not want assignment syntax at the call site.

## Parameters

**name** / **names**

The feature name (or names) to create or overwrite.

**value** / **values**

The replacement data: anything assignable to a `LetSlot` (`Thread`, `ThreadValue`,
`pluss::Series`/`Note`/`Timeline`/`TimeIndex`, or a single-feature `DataFrame`).

**options** (`const AlignOptions&`, optional)

Controls how a right-hand `Thread` is aligned to the frame's case axis when it
doesn't already match exactly.

## Returns

`LetSlot` / `LetSlots`
    A proxy bound to `*this`; assigning to it performs the mutation and creates or
    overwrites the named feature(s) in place.

## Examples

Example 1. The `df.set(name) = value` builder style, mutating the frame in place.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",    Thread(pluss::make_series({1, 2, 3}))},
    {"sales", Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city",  Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

orders.set("profit") = Thread(pluss::make_series({40.0, 35.0, 30.0}));
std::cout << orders;
```

### Output

```text
==================================
     | id    sales  city    profit
----------------------------------
0    | 1     100    Accra   40    
1    | 2     120    Tema    35    
2    | 3     90     Kumasi  30    
----------------------------------
[3 cs x 4 feat]
```

`orders` itself now has a fourth feature, `"profit"` — `set("profit")` returned a
`LetSlot` bound to `orders`, and assigning a `Thread` to it wrote the new column back
into `orders` directly.

Example 2. Assign several new features together in one batch.

```cpp
orders.set({"cost", "region"}) = {
    Thread(pluss::make_series({60.0, 85.0, 60.0})),
    Thread(pluss::make_note({"South", "North", "South"})),
};
std::cout << orders;
```

### Output

```text
================================================
     | id    sales  city    profit  cost  region
------------------------------------------------
0    | 1     100    Accra   40      60    South 
1    | 2     120    Tema    35      85    North 
2    | 3     90     Kumasi  30      60    South 
------------------------------------------------
[3 cs x 6 feat]
```

Both `"cost"` and `"region"` are added to `orders` in one call, matched to their
names positionally against the right-hand `{...}` list.

Example 3. Overwrite an existing feature from a derived single-feature frame.

```cpp
auto rebased = orders.features({"sales"}).div(10.0);
orders.set("sales") = rebased;
std::cout << orders;
```

### Output

```text
==========================
     | id    sales  city  
--------------------------
0    | 1     10     Accra 
1    | 2     12     Tema  
2    | 3     9      Kumasi
--------------------------
[3 cs x 3 feat]
```

Because the right-hand side has exactly one feature, `set("sales") = rebased`
replaces the existing `sales` column directly.

## Notes

- `set(...)` is assignment-target only. To mutate in one call without `=`, use
  [`set_()`](set_.md).
- Assigning a `ThreadValue` to a `LetSlot` broadcasts a single scalar to every row —
  useful for `df.set("flag") = ThreadValue(true)`-style constant columns.
- Batch-assigning with `set({"a", "b"}) = {...}` performs at most one schema
  reindex pass total, rather than one per feature — prefer it over several separate
  single-name assignments when setting many features at once.

## See Also

- [`DataFrame::set_`](set_.md): the direct one-line in-place counterpart to this
  page's assignment-target form.
- [`DataFrame::assign`](assign.md): derive new features from expressions evaluated
  against the frame itself, rather than assigning pre-built `Thread`s.
- [`DataFrame::insert`](insert.md), [`DataFrame::place`](place.md): position a new
  feature at a specific index or next to a named anchor, instead of appending it.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="se.html">
    <span class="label">Previous</span>
    <span class="title">se</span>
  </a>
  <a class="next-link" href="set_.html">
    <span class="label">Next</span>
    <span class="title">set_</span>
  </a>
</div>

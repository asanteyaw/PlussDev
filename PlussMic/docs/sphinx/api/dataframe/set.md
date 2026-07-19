<!-- pluss-hand-authored -->

# DataFrame::set

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
// In-place assignment helpers ("df.set(name) = value" syntax) — mutate *this
DataFrame::LetSlot DataFrame::set(std::string_view name)
DataFrame::LetSlot DataFrame::set(std::string_view name, const AlignOptions& options)
DataFrame::LetSlots DataFrame::set(pluss::RangeView<std::string_view> names)
DataFrame::LetSlots DataFrame::set(pluss::RangeView<std::string_view> names, const AlignOptions& options)
DataFrame::LetSlots DataFrame::set(std::initializer_list<std::string_view> names)
DataFrame::LetSlots DataFrame::set(std::initializer_list<const char*> names)

// Plain functional overloads — return a new frame, *this untouched
DataFrame::set(std::string_view name, const Thread& value) const -> DataFrame
DataFrame::set(std::string_view name, const Thread& value, const AlignOptions& options) const -> DataFrame
DataFrame::set(pluss::RangeView<std::string_view> names, pluss::RangeView<Thread> values) const -> DataFrame
```

## What It Does

Adds a new feature or replaces an existing one's values, in two distinct styles:

- **The `df.set(name) = value` builder style** (the no-`value`-argument overloads):
  `set(name)` returns a lightweight `LetSlot` proxy bound to `*this`; assigning a
  `Thread`, `ThreadValue`, `pluss::Series`/`Note`/`Timeline`/`TimeIndex`, or even a
  single-feature `DataFrame` to that proxy **mutates the calling frame in place** —
  creating `name` if it doesn't already exist, or overwriting it if it does.
  `set({"a", "b"}) = {...}` is the same idea for several features assigned together
  in one batch (at most one reindex pass, rather than one per feature). A
  `ThreadValue` right-hand side broadcasts a single scalar to every row.
- **The plain two-argument overloads** (`set(name, value)` / `set(names, values)`):
  ordinary, non-mutating functions that return a **new** frame with `name`
  created or replaced, leaving `*this` untouched — the same convention as
  every other non-`_`-suffixed method in this reference.

[`set_(name, value)`](set_.md) is the mutating counterpart to the plain two-argument
overloads specifically — a third way to write "mutate in place" alongside the
`LetSlot`/`LetSlots` builder style described above.

## Parameters

**name** / **names**

The feature name (or names) to create or overwrite.

**value** / **values**

The replacement data. For the builder style, anything assignable to a `LetSlot`
(`Thread`, `ThreadValue`, `pluss::Series`/`Note`/`Timeline`/`TimeIndex`, or a
single-feature `DataFrame`); for the plain overloads, a `Thread` (or a matching range
of `Thread`s for the multi-name overload).

**options** (`const AlignOptions&`, optional)

Controls how a right-hand `Thread` is aligned to the frame's case axis when it
doesn't already match exactly.

## Returns

`LetSlot` / `LetSlots`
    A proxy bound to `*this`; assigning to it performs the mutation and creates or
    overwrites the named feature(s) in place.

`DataFrame`
    (Plain two-argument overloads) A new frame with the named feature(s) created or
    replaced.

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

Example 3. The plain, non-mutating overload: overwrite an existing feature's values
without touching the source frame.

```cpp
auto fresh_start = DataFrame({
    {"id",    Thread(pluss::make_series({1, 2, 3}))},
    {"sales", Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city",  Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto rebased = fresh_start.set("sales", Thread(pluss::make_series({0.0, 0.0, 0.0})));
std::cout << rebased;
std::cout << fresh_start;
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
==========================
     | id    sales  city  
--------------------------
0    | 1     100    Accra 
1    | 2     120    Tema  
2    | 3     90     Kumasi
--------------------------
[3 cs x 3 feat]
```

`rebased` has `sales` zeroed out, but this two-argument, non-`_`-suffixed overload
never touches `fresh_start` — it prints exactly as it was constructed.

## Notes

- The `LetSlot`/`LetSlots`-returning overloads (no `value` argument) are the only
  `set(...)` calls that mutate `*this`; every overload that takes `value`/`values`
  directly as an argument is a plain, non-mutating function.
- Assigning a `ThreadValue` to a `LetSlot` broadcasts a single scalar to every row —
  useful for `df.set("flag") = ThreadValue(true)`-style constant columns.
- Batch-assigning with `set({"a", "b"}) = {...}` performs at most one schema
  reindex pass total, rather than one per feature — prefer it over several separate
  single-name assignments when setting many features at once.

## See Also

- [`DataFrame::set_`](set_.md): the mutating counterpart to this page's plain
  two-argument overloads.
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

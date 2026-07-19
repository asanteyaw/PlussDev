<!-- pluss-hand-authored -->

# DataFrame::map

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::map(const std::function<ThreadValue(const ThreadValue&)>& fn, pluss::RangeView<std::string_view> features = {}) const -> DataFrame
```

## What It Does

Applies `fn` to every cell of the selected features, one scalar value at a time, and
returns a new frame with those features replaced. `fn` receives one cell as a
`ThreadValue` and must return one `ThreadValue` back — it is skipped entirely for
missing cells (a `NULL` cell stays `NULL` without `fn` ever being called), and every
non-null cell in one feature's column must map to the **same** output kind (you
cannot have `fn` return a numeric value for one row and text for another row of the
same feature).

Unlike [`transform()`](transform.md), which operates on a whole feature `Thread` at
once (so it can do things like a running total), `map()` only ever sees one value at
a time — it is the right tool for a pure, independent per-cell computation.

## Parameters

**fn** (`const std::function<ThreadValue(const ThreadValue&)>&`)

The per-cell callback. Extract the incoming scalar with `.to_dbl()` / `.to_int()` /
`.to_str()` (matching the feature's kind), compute a new value, and wrap it back into
a `ThreadValue` over a length-1 `Thread` (for example
`ThreadValue(Thread(pluss::make_series({result})))`). The output kind does not have to
match the input kind — mapping `Note` cells to their string length (an `Int64`
result) is a valid, kind-changing use of `map()`.

**features** (`pluss::RangeView<std::string_view>`, default `{}`)

Which features `fn` is applied to. Leave empty (the default) to apply `fn` to every
feature on the frame — note that this only makes sense if `fn` can handle every
feature's kind; pass an explicit subset when different features need different
treatment.

## Returns

`DataFrame`
    A new frame, same shape as the source, with the selected features replaced by
    `fn`'s output. Untargeted features are unchanged, and missing cells stay missing.

## Examples

Example 1. Square every value in one numeric feature.

```cpp
using namespace pluss::table;

auto students = DataFrame({
    {"score", Thread(pluss::make_series({80.0, 90.0, 70.0}))},
    {"city",  Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
std::cout << students;
```

### Output (before)

```text
====================
     | score  city  
--------------------
0    | 80     Accra 
1    | 90     Tema  
2    | 70     Kumasi
--------------------
[3 cs x 2 feat]
```

```cpp
auto squared = students.map(
    [](const ThreadValue& v) {
      const auto x = v.to_dbl();
      return ThreadValue(Thread(pluss::make_series({x * x})));
    },
    {"score"});
std::cout << squared;
```

### Output (after)

```text
====================
     | score  city  
--------------------
0    | 6400   Accra 
1    | 8100   Tema  
2    | 4900   Kumasi
--------------------
[3 cs x 2 feat]
```

Only `score` changed (`80 -> 6400`, `90 -> 8100`, `70 -> 4900`); `city` is left
untouched since it wasn't in `features`.

Example 2. A kind-changing map: replace text with its length.

```cpp
auto with_length = students.map(
    [](const ThreadValue& v) {
      const auto s = v.to_str();
      return ThreadValue(Thread(pluss::make_series({static_cast<std::int64_t>(s.size())})));
    },
    {"city"});
std::cout << with_length;
```

### Output

```text
==================
     | score  city
------------------
0    | 80     5   
1    | 90     4   
2    | 70     6   
------------------
[3 cs x 2 feat]
```

`city` is now an `Int64` `Series` of string lengths (`"Accra"` -> `5`, `"Tema"` -> `4`,
`"Kumasi"` -> `6`) rather than the original `Note` text — `map()` does not require the
output kind to match the input kind, as long as every row of that one feature agrees
on the output kind.

## Notes

- `fn` is never invoked for a missing cell; the cell stays `NULL` in the output
  without needing special-case handling inside `fn` itself.
- Every non-null result within one feature must share the same `ThreadKind`; mixing
  kinds within a single feature's output throws `"Thread::map: output kinds must be
  consistent"`.
- Leaving `features` empty applies `fn` to literally every feature on the frame,
  including features of different kinds — only do this when `fn` is written to handle
  every kind present, typically by branching on `v.kind()` inside the lambda.
- `map()` is non-mutating; it always returns a new frame.

## See Also

- [`DataFrame::transform`](transform.md): apply a function to a whole feature
  `Thread` at once, for computations that need to see more than one cell (running
  totals, shifts, and so on).
- [`DataFrame::cast`](cast.md): change a feature's dtype or payload kind directly,
  when the target kind is fixed rather than computed per-cell.
- [`DataFrame::fold`](fold.md) / [`DataFrame::flow`](flow.md): the whole-frame
  callback hooks, for computations that need more than one feature at a time.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lxor.html">
    <span class="label">Previous</span>
    <span class="title">lxor</span>
  </a>
  <a class="next-link" href="mask.html">
    <span class="label">Next</span>
    <span class="title">mask</span>
  </a>
</div>

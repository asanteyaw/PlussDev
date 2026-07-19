<!-- pluss-hand-authored -->

# DataFrame::flow

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::flow(const std::function<DataFrame(const DataFrame&)>& fn) const -> DataFrame
```

## What It Does

A plain escape hatch for chaining a custom, multi-step transformation inline without
breaking out of a fluent call sequence. `flow()` calls `fn` with the current frame and
returns whatever `DataFrame` `fn` produces — there is no validation, no shape
requirement, and no special handling beyond checking that `fn` is callable and that it
returns a defined frame. Reach for it when a transformation needs several
already-existing `DataFrame` calls chained together (a `filter()` followed by an
`insert()`, say) and you want that whole sequence to read as one step in a larger
pipeline, rather than assigning it to an intermediate variable.

[`fold()`](fold.md) is the equivalent hook for reducing a frame down to a `Thread`
instead of transforming it into another frame — the two share the same "run an
arbitrary callback against the current frame" idea, just with a different return type
and (for `fold()`) an optional result-shape check.

## Parameters

**fn** (`const std::function<DataFrame(const DataFrame&)>&`)

The callback to run. It receives the current frame by `const` reference and must
return a defined `DataFrame`. Throws `"DataFrame::flow: callback is empty"` if `fn`
holds no target, and `"DataFrame::flow: callback returned undefined"` if it returns an
undefined frame.

## Returns

`DataFrame`
    Exactly whatever `fn` returned.

## Examples

Example 1. Chain a filter and a computed column into one inline step.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"units",      Thread(pluss::make_series({2, 5, 1}))},
    {"unit_price", Thread(pluss::make_series({1200.0, 45.5, 1150.0}))},
}, Thread(pluss::make_note({"o1", "o2", "o3"})));
std::cout << orders;
```

### Output (before)

```text
========================
     | units  unit_price
------------------------
o1   | 2      1200      
o2   | 5      45.5      
o3   | 1      1150      
------------------------
[3 cs x 2 feat]
```

```cpp
auto out = orders.flow([](const DataFrame& f) {
  auto filtered = f.filter(f.features("units") >= 2);
  return filtered.insert(filtered.nfeatures(), "revenue",
                          filtered.features("units") * filtered.features("unit_price"));
});
std::cout << out;
```

### Output (after)

```text
=================================
     | units  unit_price  revenue
---------------------------------
o1   | 2      1200        2400   
o2   | 5      45.5        227.5  
---------------------------------
[2 cs x 3 feat]
```

Case `o3` (`units = 1`) is dropped by the inner `filter()`, and the surviving two rows
get a new `revenue` column (`units * unit_price`) appended — all expressed as one
`flow()` call rather than three separate statements with an intermediate variable.

## Notes

- `flow()` performs no shape or content validation beyond requiring `fn` to be
  non-empty and to return a defined frame — unlike [`fold()`](fold.md), there is no
  `ResultShape` parameter, since a `DataFrame` result has no single expected size to
  check against.
- Because `fn` receives the frame by `const` reference and `flow()` itself is `const`,
  the original frame is never modified — only the returned value reflects whatever
  `fn` computed.
- Nothing about `flow()` requires the returned frame to share any structure with the
  input — it can have entirely different features, a different case count, or both.

## See Also

- [`DataFrame::fold`](fold.md): the equivalent hook for reducing to a `Thread` instead
  of transforming into another `DataFrame`.
- [`DataFrame::transform`](transform.md): apply a function per-feature or per-value
  across the frame, for simpler cases that don't need a whole custom pipeline.
- [`DataFrame::map`](map.md): apply a scalar function elementwise, the finer-grained
  sibling of `transform()`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="floordiv.html">
    <span class="label">Previous</span>
    <span class="title">floordiv</span>
  </a>
  <a class="next-link" href="fold.html">
    <span class="label">Next</span>
    <span class="title">fold</span>
  </a>
</div>

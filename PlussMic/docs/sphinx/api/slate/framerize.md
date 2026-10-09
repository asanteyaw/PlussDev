# Slate::framerize

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
Slate::framerize(std::string_view name = {}) -> DataFrame
```

Reshape this Slate into a one-column `DataFrame` -- the 1-D to 2-D counterpart of pandas `Series.to_frame`.

Reach for it at the end of a reducing chain that lands on a Slate you actually want to present or carry as a frame: `df.ndistinct(false).arrange(false)` gives you a Slate, and `.framerize("unique_values")` turns it into a labelled one-column frame that renders with `DataFrame`'s aligned table layout instead of the terser Slate view.

The single output column takes `name`. This Slate's display labels -- its index -- become the frame's cases, so a sorted cardinality Slate produces a frame whose row index is the feature names in sorted order.

## Parameters

**name**

The name of the single output column.

- pass a name for a self-describing column (`"unique_values"`, `"count"`, ...)
- leave it empty to fall back to this Slate's own name, or to `x` if the Slate is unnamed

## Returns

`DataFrame`
    A frame with exactly one feature. Its cases are this Slate's display labels when it has them, otherwise the default `0..n-1` positions. Values and missing-ness are carried across unchanged.

## See Also

- <a href="name.html"><code>Slate::name</code></a>: set the Slate's own name, which <code>framerize()</code> uses as the column name when no argument is given.
- <a href="label.html"><code>Slate::label</code></a>: attach the display labels that become the frame's row index.

## Notes

Column-name resolution: an explicit non-empty `name` wins; otherwise the Slate's own name is used; otherwise the column is labelled `x`.

The Slate's own name is not copied onto the column -- only the resolved column name applies. The index, if present, is moved to the frame's cases rather than duplicated on the column.

`DataFrame::to_frame(...)` is a different operation (contingency / wide reshape of an existing frame). Use `framerize` for the Slate to one-column-frame direction.

## Examples

Example 1. Cardinality report: a reduced Slate becomes a labelled one-column frame.

```cpp
using namespace pluss::table;

auto counts = Slate(pluss::make_series({5.0, 3.0, 3.0, 2.0}))
                  .label().set(Slate(pluss::make_note({"minutes", "branch", "runner", "status"})))
                  .name("ndistinct");

auto out = counts.framerize("unique_values");
```

### Output

```text
=======================
        | unique_values
-----------------------
minutes | 5
branch  | 3
runner  | 3
status  | 2
-----------------------
[4 cs x 1 feat]
```

Example 2. No argument: the column takes the Slate's own name.

```cpp
using namespace pluss::table;

auto counts = Slate(pluss::make_series({5.0, 3.0, 3.0, 2.0}))
                  .label().set(Slate(pluss::make_note({"minutes", "branch", "runner", "status"})))
                  .name("ndistinct");

auto out = counts.framerize();
```

### Output

```text
===================
        | ndistinct
-------------------
minutes | 5
branch  | 3
runner  | 3
status  | 2
-------------------
[4 cs x 1 feat]
```

Example 3. A bare Slate with no name and no index frames to column `x` over default cases.

```cpp
using namespace pluss::table;

auto out = Slate(pluss::make_series({7.0, 8.0})).framerize();
```

### Output

```text
===========
     | x
-----------
0    | 7
1    | 8
-----------
[2 cs x 1 feat]
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fold.html">
    <span class="label">Previous</span>
    <span class="title">fold</span>
  </a>
  <a class="next-link" href="freq.html">
    <span class="label">Next</span>
    <span class="title">freq</span>
  </a>
</div>

# DataFrame::combine

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::combine(const DataFrame& other) -> DataFrame

DataFrame::combine(const DataFrame& other, const CombineSpec& spec) -> DataFrame
```

Combine this frame with another frame using either the default feature-combine route or an explicit combine policy.

This is the lower-level frame composition surface behind update-style and join-like feature stitching.

## Parameters

**other**

Right-hand frame to combine with the current frame.

Both frames are aligned through the combine policy before the result is materialized.

**spec**

Explicit combine policy.

Use this when you need to control:
- whether overlapping values are overwritten,
- how the case axis is matched,
- how duplicate or colliding features are handled.

## Returns

`DataFrame`
    Combined frame produced by the chosen combine policy.

## See Also

- <a href="update.html"><code>DataFrame::update</code></a>: focused overwrite-style combination.
- <a href="unify.html"><code>DataFrame::unify</code></a>: key-based merge surface.
- <a href="conform.html"><code>DataFrame::conform</code></a>: align shape before arithmetic or updates.

## Notes

The default overload appends or disambiguates features as needed so you can keep both sides of overlapping data.

`CombineSpec` is the route to use when you need explicit collision, overwrite, and case-alignment control.

## Examples

Example 1. Keep the existing frame and append non-overlapping features from the right-hand frame.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
    {"region", pluss::make_note({"south", "south", "middle", "north"})},
    {"units", pluss::make_series({10.0, 7.0, 12.0, 5.0})},
    {"price", pluss::make_series({3.0, 4.0, 2.5, 5.0})},
}).caselbl(pluss::make_note({"A100", "B200", "B200", "D400"}));

auto extra = DataFrame({
    {"region", pluss::make_note({"south", "south", "middle", "north"})},
    {"discount", pluss::make_series({0.5, 0.75, 0.25, 1.0})},
}, pluss::make_note({"A100", "B200", "B200", "D400"}));

auto out = sales.combine(extra);
```

### Output

```text
===========================================================
     | city    region  units  price  region_right  discount
-----------------------------------------------------------
A100 | Accra   south   10     3      south         0.5     
B200 | Tema    south   7      4      south         0.75    
B200 | Kumasi  middle  12     2.5    middle        0.25    
D400 | Tamale  north   5      5      north         1       
-----------------------------------------------------------
[4 cs x 6 feat]
```

Example 2. Use `CombineSpec` to update matching rows without overwriting existing left-hand values.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
    {"region", pluss::make_note({"south", "south", "middle", "north"})},
    {"units", pluss::make_series({10.0, 7.0, 12.0, 5.0})},
    {"price", pluss::make_series({3.0, 4.0, 2.5, 5.0})},
}).caselbl(pluss::make_note({"A100", "B200", "B200", "D400"}));

auto patch = DataFrame({
    {"units", pluss::make_series({70.0, 50.0})},
    {"price", pluss::make_series({6.0, 9.0})},
}, pluss::make_note({"B200", "D400"}));

DataFrame::CombineSpec spec;
spec.op = DataFrame::CombineOp::Update;
spec.overwrite = false;
spec.cases = FrameMatchRule::KeepLeft;

auto out = sales.combine(patch, spec);
```

### Output

```text
===================================
     | city    region  units  price
-----------------------------------
A100 | Accra   south   10     3    
B200 | Tema    south   7      4    
B200 | Kumasi  middle  12     2.5  
D400 | Tamale  north   5      5    
-----------------------------------
[4 cs x 4 feat]
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="clip.html">
    <span class="label">Previous</span>
    <span class="title">clip</span>
  </a>
  <a class="next-link" href="compare.html">
    <span class="label">Next</span>
    <span class="title">compare</span>
  </a>
</div>

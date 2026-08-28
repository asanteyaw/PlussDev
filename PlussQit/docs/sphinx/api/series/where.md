<!-- pluss-hand-authored -->

# Series::where

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::where(const Series& condition,
              const Series& other) const -> Series

Series::where(const Series& condition,
              double other) const -> Series
```

## What It Does

`where()` builds a new `Series` by choosing between two branches position by
position.

Most of the time, the public reading is:

`self.where(condition, other)` means:

- keep `self` where `condition` is false
- take `other` where `condition` is true

That is the ordinary numeric-self calling convention.

There is one important second calling style:

- if `self` is boolean and `condition` is not boolean, `self` itself becomes
  the mask
- in that form, `self.where(a, b)` means "pick `a` where `self` is true,
  otherwise pick `b`"

That dual behavior is real and worth learning early because it changes how a
boolean `Series` call site should be read.

## Overload Map

**`where(condition, other_series)`**

The general branch-selection form.

**`where(condition, scalar_other)`**

Convenience overload for scalar fallback or replacement values.

The same broadcast and promotion rules still apply.

## Parameters

**condition** (`const Series&`)

The branch mask for the common numeric-self calling convention.

In that form:

- true means "take `other`"
- false means "keep `self`"

If the condition is numeric, it is converted to boolean truthiness first.

In the boolean-self calling style, this parameter is not the mask. It is the
"true branch" payload instead.

**other** (`const Series&` or `double`)

The alternate branch payload.

In numeric-self calls, this is the replacement branch chosen where
`condition` is true.

In boolean-self calls, this is the "false branch" payload.

Scalar `other` values are turned into scalar-like `Series` values and then
broadcast as needed.

## Returns

`Series`
    A newly assembled `Series`.

The output dtype follows branch promotion rules, so mixed branch dtypes can
promote to a wider result.

## Gradient

`where()` is differentiable only through the branch values that actually win.

At each position:

- selected `self` positions feed gradient into `self`
- selected `other` positions feed gradient into `other`
- the mask is control data, not a differentiable numeric branch

## Examples

Example 1. In the common numeric-self form, true selects `other` and false
keeps `self`.

```cpp
auto self = pluss::make_series({10.0, 20.0, 30.0, 40.0});
auto cond = pluss::make_series({false, true, false, true});

std::cout << self.where(cond, -1.0);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[10, -1, 30, -1]
```

Example 2. The alternate branch can be another full `Series`.

```cpp
auto other = pluss::make_series({100.0, 200.0, 300.0, 400.0});

std::cout << self.where(cond, other);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[10, 200, 30, 400]
```

Example 3. The condition itself can broadcast.

```cpp
auto matrix = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});
auto col_mask = pluss::make_series({true, false, true});

std::cout << matrix.where(col_mask, -1.0);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-1, 2, -1],
  [-1, 5, -1]]
```

Example 4. When `self` is boolean, the call can be read as
"mask picks between two payloads."

```cpp
auto mask = pluss::make_series({true, false, true, false});
auto a = pluss::make_series({1.0, 2.0, 3.0, 4.0});
auto b = pluss::make_series({10.0, 20.0, 30.0, 40.0});

std::cout << mask.where(a, b);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 20, 3, 40]
```

Example 5. Numeric conditions are converted to boolean truthiness.

```cpp
auto numeric_cond = pluss::make_series({0.0, 2.0, 0.0, -5.0});

std::cout << self.where(numeric_cond, other);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[10, 200, 30, 400]
```

Example 6. Mixed branch dtypes can promote the result.

```cpp
auto ints = pluss::make_series({1, 2, 3}, pluss::ScalarType::Int32);
auto halfs = pluss::make_series({0.5, 0.5, 0.5}).to(pluss::ScalarType::Float32);
auto pick = pluss::make_series({true, false, true});

std::cout << ints.where(pick, halfs);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float32, platform=cpu, numel=3, contiguous=true)
data=[0.5, 2, 0.5]
```

Example 7. Gradient flows only through the branch that actually wins at each
position.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
auto y = pluss::make_series({10.0, 20.0, 30.0});
auto grad_mask = pluss::make_series({true, false, true});

x.tracks_slope_(true);
y.tracks_slope_(true);

auto out = x.where(grad_mask, y);
out.sum().reverse();

std::cout << "out=" << out << "\n";
std::cout << "dx=" << x.sensitivity() << "\n";
std::cout << "dy=" << y.sensitivity() << "\n";
```

### Output

```text
out=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[10, 2, 30]
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 1, 0]
dy=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 0, 1]
```

Example 8. Broadcast-incompatible shapes fail.

```cpp
try {
  std::cout << matrix.where(pluss::make_series({true, false}), -1.0) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
broadcast: incompatible shapes
```

## Gotchas And Failure Modes

- On numeric `self`, the common reading is "true selects `other`."
- On boolean `self`, `self` itself becomes the mask.
- Numeric conditions are cast to boolean truthiness.
- Broadcasting rules still apply to the mask and both branches.
- Dtype promotion can widen the output when branch dtypes disagree.

## Summary

Use `where()` for binary branch selection on an existing `Series`.

- on numeric `self`, read it as "replace self where condition is true"
- on boolean `self`, read it as "mask picks between the next two payloads"
- remember that only the chosen branch receives gradient at each position

## See Also

- [`pluss::series::select`](../series_factory/select.md): multi-branch free
  function for ordered conditional construction.
- [`Series::all`](all.md): reduce a mask to one global or axis-wise boolean
  answer.
- [`Series::any`](any.md): test whether at least one value in a lane is truthy.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="view.html">
    <span class="label">Previous</span>
    <span class="title">view</span>
  </a>
  <a class="next-link" href="../../series_stat_accessor_api_reference.html">
    <span class="label">Next</span>
    <span class="title">SeriesStatAccessor</span>
  </a>
</div>

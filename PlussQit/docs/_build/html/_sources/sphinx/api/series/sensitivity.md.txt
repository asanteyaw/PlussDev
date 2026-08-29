<!-- pluss-hand-authored -->

# Series::sensitivity

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::sensitivity() const -> Series
```

## What It Does

`sensitivity()` returns the current reverse-mode sensitivity associated with
this `Series`.

For tracked leaves after a reverse pass, that returned `Series` is the quantity
most people informally call "the gradient." It has the same logical shape as
the source value and stores how the current objective changed with respect to
each element of that source.

There is one subtle but important current-surface behavior to keep in mind:
when no stored sensitivity exists yet, this method returns a same-shaped
zero-valued `Series` rather than throwing.

That is convenient for printing, but it means you should use
[`has_sensitivity()`](has_sensitivity.md) when you need to distinguish "no
stored sensitivity yet" from "a stored sensitivity that happens to be all
zeros."

## Parameters

None.

## Returns

`Series`
    The stored sensitivity, or a same-shaped zero-valued `Series` when no
    stored sensitivity currently exists.

## Shape And Dtype Notes

- the returned sensitivity follows the source shape
- element positions line up one-for-one with the source positions
- the result is meant for numeric interpretation, not as a boolean state probe

For boolean existence checks, prefer [`has_sensitivity()`](has_sensitivity.md).

## Examples

Example 1. After a reverse pass, `sensitivity()` returns the stored
element-aligned sensitivity.

```cpp
auto x = pluss::make_series({
    {1.0, 2.0},
    {3.0, 4.0},
});
x.tracks_slope_(true);

x.mul(3.0).sum().reverse();
std::cout << pluss::series::print(x.sensitivity());
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=
⎡ 3.0000 3.0000 ⎤
⎣ 3.0000 3.0000 ⎦
```

Here every element contributed linearly with factor `3`, so every sensitivity
entry is `3`.

Example 2. Before any reverse pass has populated one, the current public
surface returns zeros.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
x.tracks_slope_(true);

std::cout << x.sensitivity();
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0, 0]
```

This is the most important quirk on the page. Zero values do not by themselves
prove that a reverse pass already happened.

Example 3. Pairing `has_sensitivity()` with `sensitivity()` disambiguates that
state cleanly.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
x.tracks_slope_(true);

std::cout << "has_dx=" << (x.has_sensitivity() ? "true" : "false") << "\n";
std::cout << x.sensitivity();
```

### Output

```text
has_dx=false
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0, 0]
```

Example 4. Clearing sensitivity resets the stored value so a later reverse pass
can populate a fresh one.

```cpp
auto reuse = pluss::make_series({1.0, 2.0, 3.0});
reuse.tracks_slope_(true);

reuse.mul(2.0).sum().reverse();
std::cout << "first=" << reuse.sensitivity() << "\n";

reuse.clear_sensitivity();
reuse.mul(5.0).sum().reverse();
std::cout << "second=" << reuse.sensitivity() << "\n";
```

### Output

```text
first=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2, 2, 2]
second=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[5, 5, 5]
```

## Gotchas And Failure Modes

- `sensitivity()` is a numeric readout, not a presence check.
- On the current verified surface, "no stored sensitivity yet" reads back as a
  zero-valued series instead of an exception.
- If you need to know whether the value was actually populated by reverse-mode,
  call [`has_sensitivity()`](has_sensitivity.md) too.

## Summary

Use `sensitivity()` to read the current reverse-mode sensitivity in series
form, and pair it with `has_sensitivity()` whenever presence-versus-zero is an
important distinction.

## See Also

- [`Series::has_sensitivity`](has_sensitivity.md): boolean presence test.
- [`Series::reverse`](reverse.md): populate the stored sensitivity.
- [`Series::clear_sensitivity`](clear_sensitivity.md): clear the stored value.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="select.html">
    <span class="label">Previous</span>
    <span class="title">select</span>
  </a>
  <a class="next-link" href="setdiff1d.html">
    <span class="label">Next</span>
    <span class="title">setdiff1d</span>
  </a>
</div>

<!-- pluss-hand-authored -->

# pluss::series::print

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
pluss::series::print(const Series& s) -> std::string

pluss::series::print(const Series& s,
                     const PrintOptions& opt) -> std::string

pluss::series::print(std::ostream& os,
                     const Series& s,
                     const PrintOptions& opt = {}) -> std::ostream&
```

## What It Does

`pluss::series::print(...)` renders a `Series` in the readable aligned layout
used throughout these handwritten docs.

This page lives in the factory reference because `print(...)` is a free
function on the public `pluss::series::*` surface, not a `Series` method.

Use it when you want one of two things:

- a printable `std::string`
- direct output to a stream such as `std::cout`

It never mutates the `Series`. It is a presentation tool.

## Overload Map

**`print(s)`**

Returns the default formatted rendering as a `std::string`.

**`print(s, opt)`**

Returns a `std::string` using the caller's formatting policy.

**`print(os, s, opt)`**

Writes directly to a stream and returns that stream so the call chains naturally
with `<< "\n"`.

## Parameters

**os** (`std::ostream&`)

The destination stream for the stream-writing overload.

Use this form when printing is the final side effect rather than an
intermediate string.

**s** (`const Series&`)

The `Series` to render.

The function reads the current shape, dtype, platform, and values without
changing them.

**opt** (`const PrintOptions&`)

Formatting policy.

The option surface is wide, but the usage model is easier if you group the
toggles:

- metadata toggles control whether shape, dtype, platform, strides, and other
  header details are shown
- content toggles decide whether you see header only, values only, or both
- numeric toggles control general/fixed/scientific rendering, precision, and
  trimming
- boolean toggles choose between `true/false` and `1/0`
- pretty-layout toggles control alignment, bracket style, slice labels,
  summarization, and line wrapping

That means `PrintOptions` changes how the data is shown, not what the data is.

## Returns

`std::string`
    For the string-returning overloads.

`std::ostream&`
    For the stream-writing overload.

## Examples

Example 1. The simplest overload returns a formatted string.

```cpp
auto printable = pluss::make_series({
    {1.25, 2.5, 3.75},
    {4.0, 5.5, 6.25},
});

std::cout << pluss::series::print(printable);
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡ 1.2500 2.5000 3.7500 ⎤
⎣ 4.0000 5.5000 6.2500 ⎦
```

Example 2. The stream overload writes directly to `std::cout`.

```cpp
pluss::series::print(std::cout, printable);
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡ 1.2500 2.5000 3.7500 ⎤
⎣ 4.0000 5.5000 6.2500 ⎦
```

Example 3. You can hide the metadata block and keep the data rendering.

```cpp
pluss::PrintOptions opt;
opt.metadata_mode = pluss::codem::PrintMetadataMode::None;

std::cout << pluss::series::print(printable, opt);
```

### Output

```text

data=
⎡ 1.2500 2.5000 3.7500 ⎤
⎣ 4.0000 5.5000 6.2500 ⎦
```

Example 4. Boolean output can be rendered in compact `1/0` form.

```cpp
pluss::PrintOptions opt;
opt.bool_style = pluss::codem::PrintBoolStyle::OneZero;

std::cout << pluss::series::print(
    pluss::make_series({true, false, true, true}, pluss::ScalarType::Bool),
    opt);
```

### Output

```text
Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[1, 0, 1, 1]
```

## Gotchas And Failure Modes

- `print(...)` is for human-readable display, not stable machine interchange.
- Hiding metadata changes presentation only. It does not turn the output into a
  raw numeric dump.
- `std::cout << series` also works, but `pluss::series::print(...)` is the
  explicit public print facility when you want to teach or control formatting.

## Summary

Use `pluss::series::print(...)` when you want the public rendering surface for
`Series`.

- choose `print(s)` when you need a string
- choose `print(os, s, ...)` for direct stream output
- use `PrintOptions` to tune presentation without changing the data

## See Also

- [`pluss::series::make`](make.md): construct a `Series` to print.
- [`pluss::series::select`](select.md): build a new `Series` by conditional
  branch selection before printing it.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="make.html">
    <span class="label">Previous</span>
    <span class="title">make</span>
  </a>
  <a class="next-link" href="select.html">
    <span class="label">Next</span>
    <span class="title">select</span>
  </a>
</div>

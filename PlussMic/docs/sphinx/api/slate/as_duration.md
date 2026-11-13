# Slate::as_duration

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
const pluss::Duration& Slate::as_duration()
```

## What It Does

Converts or reinterprets the current payload as another public Pluss type so downstream operations route through the right text, numeric, or temporal surface.

## Parameters

`None`

## Returns

`const pluss::Duration&`
    Value produced by this call.

## Examples

Example 1. Convert the current object into the `duration` representation before continuing with another type-specific workflow.

```cpp
auto slate = pluss::table::Slate(pluss::make_series({10.0, 20.0, 30.0}));

auto out = slate.as_duration();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="arrange.html">
    <span class="label">Previous</span>
    <span class="title">arrange</span>
  </a>
  <a class="next-link" href="as_note.html">
    <span class="label">Next</span>
    <span class="title">as_note</span>
  </a>
</div>

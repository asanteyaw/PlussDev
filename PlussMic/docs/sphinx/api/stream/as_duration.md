# Stream::as_duration

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream::as_duration() -> const pluss::Duration&
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
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.as_duration();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="abs.html">
    <span class="label">Previous</span>
    <span class="title">abs</span>
  </a>
  <a class="next-link" href="as_note.html">
    <span class="label">Next</span>
    <span class="title">as_note</span>
  </a>
</div>

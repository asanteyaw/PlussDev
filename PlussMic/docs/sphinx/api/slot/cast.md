# Slot::cast

[Back to Slot API Reference](../../slot_api_reference)

## Signatures

```cpp
Slot::cast() -> SlotCastAccessor

Slot::cast(const pluss::table::ThreadCastOptions& options) -> SlotCastAccessor
```

## What It Does

This changes the semantic or numeric type so later operations route through the correct implementation path.

## Parameters

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

## Returns

`SlotCastAccessor`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.cast();
```

### Output

```text
The call completes and returns the contract described under Output.
```
Example 2. Pass an explicit options object to control the operation.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.cast();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="at.html">
    <span class="label">Previous</span>
    <span class="title">at</span>
  </a>
  <a class="next-link" href="concat.html">
    <span class="label">Next</span>
    <span class="title">concat</span>
  </a>
</div>

# AxelFieldAxisAccessor::empty

[Back to AxelFieldAxisAccessor API Reference](../../axel_fieldaxis_accessor_api_reference)

## Signature

```cpp
AxelFieldAxisAccessor::empty() -> Axel
```

## What It Does

Reports whether the current object has no rows, entries, slots, or depth items.

## Parameters

`None`

## Returns

`Axel`
    Value produced by this call.

## Examples

Example 1. Inspect the `empty` property of the current object.

```cpp
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto axel = pluss::bundle::Axel({r0, r1});
auto fieldaxis = axel.fieldaxis();

auto out = fieldaxis.empty();
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="next-link" href="kind.html">
    <span class="label">Next</span>
    <span class="title">kind</span>
  </a>
</div>

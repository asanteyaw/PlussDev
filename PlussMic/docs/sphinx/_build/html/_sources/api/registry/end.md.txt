# Registry::end

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry::end() -> const_iterator
```

## What It Does

Exposes iterator boundaries so the object can be traversed with a range-based loop or manual iteration.

## Parameters

`None`

## Returns

`const_iterator`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

for (const auto& key : rg) {
  (void)key;
}
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="empty.html">
    <span class="label">Previous</span>
    <span class="title">empty</span>
  </a>
  <a class="next-link" href="ensure_stream_compatible.html">
    <span class="label">Next</span>
    <span class="title">ensure_stream_compatible</span>
  </a>
</div>

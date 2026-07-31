# Axel::dropnull

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Axel::dropnull() -> Axel
```

## What It Does

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

## Parameters

`None`

## Returns

`Axel`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto axel = pluss::bundle::Axel({r0, r1});

auto out = axel.dropnull();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="drop.html">
    <span class="label">Previous</span>
    <span class="title">drop</span>
  </a>
  <a class="next-link" href="empty.html">
    <span class="label">Next</span>
    <span class="title">empty</span>
  </a>
</div>

# Slot::type_counts

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot::type_counts() -> std::unordered_map<std::string, std::int64_t>
```

## What It Does

Counts how many slot entries belong to each stored pluss type.

## Parameters

`None`

## Returns

`std::unordered_map<std::string, std::int64_t>`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_note("ok"), pluss::make_series(3)});

auto out = slot.type_counts();
```

### Output

```text
{"Series": 2, "Note": 1}
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="transform.html">
    <span class="label">Previous</span>
    <span class="title">transform</span>
  </a>
  <a class="next-link" href="unique.html">
    <span class="label">Next</span>
    <span class="title">unique</span>
  </a>
</div>

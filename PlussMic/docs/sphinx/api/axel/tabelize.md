# Axel::tabelize

[Back to Axel API Reference](../../axel_api_reference)

## Signatures

```cpp
pluss::table::Atlas Axel::tabelize()

pluss::table::Atlas Axel::tabelize(const pluss::kindle::TabelizePolicy& policy)
```

## What It Does

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

## Parameters

**policy**
    Policy object that changes the structural behavior of this overload.

## Returns

`pluss::table::Atlas`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto r0 = pluss::scribble::Registry({
    {"ids", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::scribble::Registry({
    {"ids", pluss::scribble::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto axel = pluss::scribble::Axel({r0, r1});

auto out = axel.tabelize();
```

### Output

```text
The call completes and returns the contract described under Output.
```
Example 2. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto r0 = pluss::scribble::Registry({
    {"ids", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::scribble::Registry({
    {"ids", pluss::scribble::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto axel = pluss::scribble::Axel({r0, r1});

auto out = axel.tabelize();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="slice.html">
    <span class="label">Previous</span>
    <span class="title">slice</span>
  </a>
  <a class="next-link" href="tail.html">
    <span class="label">Next</span>
    <span class="title">tail</span>
  </a>
</div>

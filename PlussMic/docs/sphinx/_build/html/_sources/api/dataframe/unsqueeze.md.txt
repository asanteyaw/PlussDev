# DataFrame::unsqueeze

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::unsqueeze() -> Atlas

DataFrame::unsqueeze(const Thread& depth_labels) -> Atlas
```

## What It Does

Promotes the frame into a depth-1 `Atlas` so it can participate in multi-frame workflows.

## Parameters

**depth_labels**
    Single depth label to attach to the one-frame atlas produced by `unsqueeze(...)`.

## Returns

`Atlas`
    Value produced by this call.

## Examples

Example 1. Lift one frame into a one-layer atlas.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.unsqueeze();
```

### Output

```text
frame0:
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Lift one frame into a one-layer atlas.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto labels = pluss::table::Thread(pluss::make_note({"train"}));
auto out = df.unsqueeze(labels);
```

### Output

```text
depth labels:
["train"]

frame0:
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

## Practical Behavior

- `unsqueeze()` wraps one frame into a depth-1 `Atlas`.
- The `depth_labels` overload lets you attach exactly one depth label to that one-frame atlas.
- This is useful when downstream APIs expect atlas depth even though you currently have only one frame.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="unify.html">
    <span class="label">Previous</span>
    <span class="title">unify</span>
  </a>
  <a class="next-link" href="unstack.html">
    <span class="label">Next</span>
    <span class="title">unstack</span>
  </a>
</div>

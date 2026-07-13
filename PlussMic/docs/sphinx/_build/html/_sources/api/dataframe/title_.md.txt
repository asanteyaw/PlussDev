# DataFrame::title_

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::title_(std::string title) -> DataFrame&
```

## What It Does

Overwrites the current frame title in place.

## Parameters

**title**
    Human-readable title stamp written back onto the current frame.

## Returns

`DataFrame&`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.title_("sales_report");
```

### Output

```text
df.title():
sales_report

df.head():
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

## Practical Behavior

- `title()` with no argument reads the current frame title.
- `title("...")` returns a titled copy and leaves the source frame unchanged.
- `title_("...")` mutates the current frame title in place.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="title.html">
    <span class="label">Previous</span>
    <span class="title">title</span>
  </a>
  <a class="next-link" href="tl.html">
    <span class="label">Next</span>
    <span class="title">tl</span>
  </a>
</div>

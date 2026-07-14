# DataFrame::rank

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::rank(RankTieRule ties = RankTieRule::Average, RankNullRule nulls = RankNullRule::Keep, bool ascending = true, bool percentage = false) -> DataFrame
```

## What It Does

Assigns numeric ranks to each compatible feature while controlling tie, null, order, and percentage policy.

## Parameters

**ties**
    Tie policy for equal values: `Average`, `Lowest`, `Highest`, `First`, or `Dense`.

**nulls**
    Null ranking policy: `Keep` leaves null ranks missing, `Top` ranks them first, and `Bottom` ranks them last.

**ascending**
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

**percentage**
    When `true`, divide each rank by the number of ranked observations.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.rank();
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rand.html">
    <span class="label">Previous</span>
    <span class="title">rand</span>
  </a>
  <a class="next-link" href="rdiv.html">
    <span class="label">Next</span>
    <span class="title">rdiv</span>
  </a>
</div>

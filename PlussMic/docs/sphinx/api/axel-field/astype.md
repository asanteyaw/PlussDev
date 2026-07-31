# AxelFieldAccessor::astype

[Back to AxelFieldAccessor API Reference](../../axel_field_accessor_api_reference)

## Signature

```cpp
AxelFieldAccessor::astype(pluss::ScalarType dtype) -> Axel
```

## What It Does

This changes the semantic or numeric type so later operations route through the correct implementation path.

## Parameters

**dtype**
    Target scalar dtype used by this conversion overload.

## Returns

`Axel`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto axel = pluss::bundle::Axel({r0, r1});
auto fields = axel.fields({"ids"});

auto out = fields.astype({});
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="next-link" href="cast.html">
    <span class="label">Next</span>
    <span class="title">cast</span>
  </a>
</div>

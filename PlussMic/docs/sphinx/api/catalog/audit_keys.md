# Catalog::audit_keys

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
CatalogKeyAudit Catalog::audit_keys(const Catalog& other, const std::vector<std::string>& left_keys, const std::vector<std::string>& right_keys, const RaggedPolicy& ragged = RaggedPolicy::defaults(OpClass::Relational))
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

**other**
    Second frame, slate, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

**left_keys**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

**right_keys**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

**ragged**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

## Returns

`CatalogKeyAudit`
    Value produced by this call.

## Examples

Example 1. Pass several names together from a standard string container.

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.audit_keys();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="astype.html">
    <span class="label">Previous</span>
    <span class="title">astype</span>
  </a>
  <a class="next-link" href="begin.html">
    <span class="label">Next</span>
    <span class="title">begin</span>
  </a>
</div>

# Catalog::match_keys

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
CatalogMatch Catalog::match_keys(const Catalog& other, const std::vector<std::string>& left_keys, const std::vector<std::string>& right_keys, relational::JoinHow how, std::optional<relational::RelationshipCardinality> validate = std::nullopt, const RaggedPolicy& ragged = RaggedPolicy::defaults(OpClass::Relational))
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

**how**
    Join mode for `DataFrame::unify(...)`.

    Supported values:
    - `common`: Keep only rows whose key values match on both sides.
    - `keep_left`: Preserve left keys and attach right-hand matches where they exist.
    - `keep_right`: Preserve right keys and attach left-hand matches where they exist.
    - `unite`: Keep the stable union of keys from both frames.
    - `cross`: Create the cartesian product; key feature arguments must be empty.
    - `left_only`: Keep left rows whose keys do not occur on the right.
    - `right_only`: Keep right rows whose keys do not occur on the left.

**validate**
    Optional key-cardinality check for `DataFrame::unify(...)`.

    Supported values:
    - `many_to_many`: Allow duplicate key values on both sides. This is the default.
    - `one_to_one`: Require each key to appear at most once on both sides.
    - `one_to_many`: Require unique keys on the left and allow duplicates on the right.
    - `many_to_one`: Allow duplicates on the left and require unique keys on the right.

**ragged**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

## Returns

`CatalogMatch`
    Value produced by this call.

## Examples

Example 1. Pass several names together from a standard string container.

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.match_keys();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="map_.html">
    <span class="label">Previous</span>
    <span class="title">map_</span>
  </a>
  <a class="next-link" href="max.html">
    <span class="label">Next</span>
    <span class="title">max</span>
  </a>
</div>

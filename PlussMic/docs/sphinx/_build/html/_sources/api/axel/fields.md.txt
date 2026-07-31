# Axel::fields

[Back to Axel API Reference](../../axel_api_reference)

## Signatures

```cpp
Axel::fields(const NameRange& keys) -> template <pluss::detail::StringViewNameRange NameRange> AxelFieldAccessor

Axel::fields(std::initializer_list<std::string_view> keys) -> AxelFieldAccessor

Axel::fields(std::span<const std::string_view> keys) -> AxelFieldAccessor
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

**keys**
    Collection of slot or stream names to keep, drop, or rewrite together.

## Returns

`template <pluss::detail::StringViewNameRange NameRange> AxelFieldAccessor`
    Value produced by this call.

`AxelFieldAccessor`
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

auto out = axel.fields();
```

### Output

```text
The call completes and returns the contract described under Output.
```
Example 2. Pass several feature names inline.

```cpp
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto axel = pluss::bundle::Axel({r0, r1});

auto out = axel.fields();
```

### Output

```text
The call completes and returns the contract described under Output.
```
Example 3. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto axel = pluss::bundle::Axel({r0, r1});

auto out = axel.fields();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fieldaxis.html">
    <span class="label">Previous</span>
    <span class="title">fieldaxis</span>
  </a>
  <a class="next-link" href="fillnull.html">
    <span class="label">Next</span>
    <span class="title">fillnull</span>
  </a>
</div>

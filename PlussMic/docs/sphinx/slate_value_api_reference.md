# SlateValue API Reference

This page documents the public scalar-like `SlateValue` wrapper returned by slate and column-value accessors.

## How To Use This Page

- Methods are grouped alphabetically so you can jump quickly from the left rail.
- Each method page section starts with the public signatures, then explains what the method does, how each parameter changes behavior, and what the call returns.
- Overloaded methods include several examples so you can see the supported call shapes instead of reverse-engineering them from signatures alone.
- Each example is followed immediately by verified output captured from the current compiled examples so the result shape is visible before you move to the next overload.

## A

### `as_duration`

#### Signature

```cpp
pluss::Duration SlateValue::as_duration()
```

#### What It Does

Converts or reinterprets the current payload as another public Pluss type so downstream operations route through the right text, numeric, or temporal surface.

#### Parameters

`None`

#### Returns

`pluss::Duration`
    Value produced by this call.

#### Examples

Example 1. Convert the current object into the `duration` representation before continuing with another type-specific workflow.

```cpp
auto slate = pluss::table::Slate(pluss::make_series({10.0}));
auto value = slate.value();

auto out = value.as_duration();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="next-link" href="#as_note">
    <span class="label">Next</span>
    <span class="title">as_note</span>
  </a>
</div>

### `as_note`

#### Signature

```cpp
pluss::Note SlateValue::as_note()
```

#### What It Does

Converts the wrapped scalar-like value into a `pluss::Note`.

#### Parameters

`None`

#### Returns

`pluss::Note`
    Value produced by this call.

#### Examples

Example 1. Convert the current object into the `note` representation before continuing with another type-specific workflow.

```cpp
auto slate = pluss::table::Slate(pluss::make_note({"Accra"}));
auto value = slate.value();

auto out = value.as_note();
```

##### Output

```text
["Accra"]
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#as_duration">
    <span class="label">Previous</span>
    <span class="title">as_duration</span>
  </a>
  <a class="next-link" href="#as_series">
    <span class="label">Next</span>
    <span class="title">as_series</span>
  </a>
</div>

### `as_series`

#### Signature

```cpp
pluss::Series SlateValue::as_series()
```

#### What It Does

Converts the wrapped scalar-like value into a `pluss::Series`.

#### Parameters

`None`

#### Returns

`pluss::Series`
    Value produced by this call.

#### Examples

Example 1. Convert the current object into the `series` representation before continuing with another type-specific workflow.

```cpp
auto slate = pluss::table::Slate(pluss::make_series({10.0}));
auto value = slate.value();

auto out = value.as_series();
```

##### Output

```text
[10.0]
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#as_note">
    <span class="label">Previous</span>
    <span class="title">as_note</span>
  </a>
  <a class="next-link" href="#as_timeline">
    <span class="label">Next</span>
    <span class="title">as_timeline</span>
  </a>
</div>

### `as_timeline`

#### Signature

```cpp
pluss::Timeline SlateValue::as_timeline()
```

#### What It Does

Converts the wrapped scalar-like value into a `pluss::Timeline`.

#### Parameters

`None`

#### Returns

`pluss::Timeline`
    Value produced by this call.

#### Examples

Example 1. Convert the current object into the `timeline` representation before continuing with another type-specific workflow.

```cpp
auto slate = pluss::table::Slate(pluss::make_timeline({"2024-01-01T09:10:00Z"}));
auto value = slate.value();

auto out = value.as_timeline();
```

##### Output

```text
["2024-01-01T09:10:00Z"]
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#as_series">
    <span class="label">Previous</span>
    <span class="title">as_series</span>
  </a>
  <a class="next-link" href="#defined">
    <span class="label">Next</span>
    <span class="title">defined</span>
  </a>
</div>

## D

### `defined`

#### Signature

```cpp
bool SlateValue::defined()
```

#### What It Does

Reports whether the current object owns a concrete payload that can be used safely in later operations.

#### Parameters

`None`

#### Returns

`bool`
    Boolean status or membership result.

#### Examples

Example 1. Inspect the `defined` property of the current object.

```cpp
auto slate = pluss::table::Slate(pluss::make_series({10.0}));
auto value = slate.value();

auto out = value.defined();
```

##### Output

```text
true
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#as_timeline">
    <span class="label">Previous</span>
    <span class="title">as_timeline</span>
  </a>
  <a class="next-link" href="#host">
    <span class="label">Next</span>
    <span class="title">host</span>
  </a>
</div>

## H

### `host`

#### Signature

```cpp
HostAny SlateValue::host()
```

#### What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

#### Parameters

`None`

#### Returns

`HostAny`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto slate = pluss::table::Slate(pluss::make_series({10.0}));
auto value = slate.value();

auto out = value.host();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#defined">
    <span class="label">Previous</span>
    <span class="title">defined</span>
  </a>
  <a class="next-link" href="#kind">
    <span class="label">Next</span>
    <span class="title">kind</span>
  </a>
</div>

## K

### `kind`

#### Signature

```cpp
SlateKind SlateValue::kind()
```

#### What It Does

Returns the underlying slate kind carried by this value wrapper.

#### Parameters

`None`

#### Returns

`SlateKind`
    Value produced by this call.

#### Examples

Example 1. Inspect the `kind` property of the current object.

```cpp
auto slate = pluss::table::Slate(pluss::make_series({10.0}));
auto value = slate.value();

auto out = value.kind();
```

##### Output

```text
Series
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#host">
    <span class="label">Previous</span>
    <span class="title">host</span>
  </a>
  <a class="next-link" href="#payload">
    <span class="label">Next</span>
    <span class="title">payload</span>
  </a>
</div>

## P

### `payload`

#### Signature

```cpp
Payload SlateValue::payload()
```

#### What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

#### Parameters

`None`

#### Returns

`Payload`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto slate = pluss::table::Slate(pluss::make_series({10.0}));
auto value = slate.value();

auto out = value.payload();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#kind">
    <span class="label">Previous</span>
    <span class="title">kind</span>
  </a>
  <a class="next-link" href="#pluss::Duration">
    <span class="label">Next</span>
    <span class="title">pluss::Duration</span>
  </a>
</div>

### `pluss::Duration`

#### Signature

```cpp
SlateValue::operator pluss::Duration() const
```

#### What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

#### Parameters

`None`

#### Returns

`operator`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto slate = pluss::table::Slate(pluss::make_series({10.0}));
auto value = slate.value();

auto out = value.pluss::Duration();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#payload">
    <span class="label">Previous</span>
    <span class="title">payload</span>
  </a>
  <a class="next-link" href="#pluss::Note">
    <span class="label">Next</span>
    <span class="title">pluss::Note</span>
  </a>
</div>

### `pluss::Note`

#### Signature

```cpp
SlateValue::operator pluss::Note() const
```

#### What It Does

Uses the implicit conversion operator to materialize the wrapped value as a `pluss::Note`.

#### Parameters

`None`

#### Returns

`operator`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto slate = pluss::table::Slate(pluss::make_series({10.0}));
auto value = slate.value();

auto out = value.pluss::Note();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#pluss::Duration">
    <span class="label">Previous</span>
    <span class="title">pluss::Duration</span>
  </a>
  <a class="next-link" href="#pluss::Series">
    <span class="label">Next</span>
    <span class="title">pluss::Series</span>
  </a>
</div>

### `pluss::Series`

#### Signature

```cpp
SlateValue::operator pluss::Series() const
```

#### What It Does

Uses the implicit conversion operator to materialize the wrapped value as a `pluss::Series`.

#### Parameters

`None`

#### Returns

`operator`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto slate = pluss::table::Slate(pluss::make_series({10.0}));
auto value = slate.value();

auto out = value.pluss::Series();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#pluss::Note">
    <span class="label">Previous</span>
    <span class="title">pluss::Note</span>
  </a>
  <a class="next-link" href="#pluss::Timeline">
    <span class="label">Next</span>
    <span class="title">pluss::Timeline</span>
  </a>
</div>

### `pluss::Timeline`

#### Signature

```cpp
SlateValue::operator pluss::Timeline() const
```

#### What It Does

Uses the implicit conversion operator to materialize the wrapped value as a `pluss::Timeline`.

#### Parameters

`None`

#### Returns

`operator`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto slate = pluss::table::Slate(pluss::make_series({10.0}));
auto value = slate.value();

auto out = value.pluss::Timeline();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#pluss::Series">
    <span class="label">Previous</span>
    <span class="title">pluss::Series</span>
  </a>
  <a class="next-link" href="#slate">
    <span class="label">Next</span>
    <span class="title">slate</span>
  </a>
</div>

## S

### `slate`

#### Signature

```cpp
const Slate& SlateValue::slate()
```

#### What It Does

Returns the wrapped single-value `Slate` object.

#### Parameters

`None`

#### Returns

`const Slate&`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto slate = pluss::table::Slate(pluss::make_series({10.0}));
auto value = slate.value();

auto out = value.slate();
```

##### Output

```text
[10.0]
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#pluss::Timeline">
    <span class="label">Previous</span>
    <span class="title">pluss::Timeline</span>
  </a>
  <a class="next-link" href="#to_dbl">
    <span class="label">Next</span>
    <span class="title">to_dbl</span>
  </a>
</div>

## T

### `to_dbl`

#### Signature

```cpp
double SlateValue::to_dbl()
```

#### What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

#### Parameters

`None`

#### Returns

`double`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto slate = pluss::table::Slate(pluss::make_series({10.0}));
auto value = slate.value();

auto out = value.to_dbl();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#slate">
    <span class="label">Previous</span>
    <span class="title">slate</span>
  </a>
  <a class="next-link" href="#to_int">
    <span class="label">Next</span>
    <span class="title">to_int</span>
  </a>
</div>

### `to_int`

#### Signature

```cpp
std::int64_t SlateValue::to_int()
```

#### What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

#### Parameters

`None`

#### Returns

`std::int64_t`
    Integer count, size, or positional result.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto slate = pluss::table::Slate(pluss::make_series({10.0}));
auto value = slate.value();

auto out = value.to_int();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#to_dbl">
    <span class="label">Previous</span>
    <span class="title">to_dbl</span>
  </a>
  <a class="next-link" href="#to_str">
    <span class="label">Next</span>
    <span class="title">to_str</span>
  </a>
</div>

### `to_str`

#### Signature

```cpp
std::string SlateValue::to_str()
```

#### What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

#### Parameters

`None`

#### Returns

`std::string`
    Rendered string result.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto slate = pluss::table::Slate(pluss::make_series({10.0}));
auto value = slate.value();

auto out = value.to_str();
```

##### Output

```text
Series
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#to_int">
    <span class="label">Previous</span>
    <span class="title">to_int</span>
  </a>
</div>

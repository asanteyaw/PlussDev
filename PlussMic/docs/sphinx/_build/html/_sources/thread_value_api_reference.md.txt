# ThreadValue API Reference

This page documents the public scalar-like `ThreadValue` wrapper returned by thread and column-value accessors.

## How To Use This Page

- Methods are grouped alphabetically so you can jump quickly from the left rail.
- Each method page section starts with the public signatures, then explains what the method does, how each parameter changes behavior, and what the call returns.
- Overloaded methods include several examples so you can see the supported call shapes instead of reverse-engineering them from signatures alone.
- Each example is followed immediately by verified output captured from the current compiled examples so the result shape is visible before you move to the next overload.

## A

### `as_note`

#### Signature

```cpp
ThreadValue::as_note() -> pluss::Note
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
auto thread = pluss::table::Thread(pluss::make_note({"Accra"}));
auto value = thread.value();

auto out = value.as_note();
```

##### Output

```text
["Accra"]
```
<div class="pluss-inline-prev-next">
  <a class="next-link" href="#as_series">
    <span class="label">Next</span>
    <span class="title">as_series</span>
  </a>
</div>

### `as_series`

#### Signature

```cpp
ThreadValue::as_series() -> pluss::Series
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
auto thread = pluss::table::Thread(pluss::make_series({10.0}));
auto value = thread.value();

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
  <a class="next-link" href="#as_timeindex">
    <span class="label">Next</span>
    <span class="title">as_timeindex</span>
  </a>
</div>

### `as_timeindex`

#### Signature

```cpp
ThreadValue::as_timeindex() -> pluss::TimeIndex
```

#### What It Does

Converts the wrapped scalar-like value into a `pluss::TimeIndex`.

#### Parameters

`None`

#### Returns

`pluss::TimeIndex`
    Value produced by this call.

#### Examples

Example 1. Convert the current object into the `timeindex` representation before continuing with another type-specific workflow.

```cpp
auto thread = pluss::table::Thread(pluss::make_timeindex({"2024-01-01T09:10:00Z"}));
auto value = thread.value();

auto out = value.as_timeindex();
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
  <a class="next-link" href="#as_timeline">
    <span class="label">Next</span>
    <span class="title">as_timeline</span>
  </a>
</div>

### `as_timeline`

#### Signature

```cpp
ThreadValue::as_timeline() -> pluss::Timeline
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
auto thread = pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z"}));
auto value = thread.value();

auto out = value.as_timeline();
```

##### Output

```text
["2024-01-01T09:10:00Z"]
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#as_timeindex">
    <span class="label">Previous</span>
    <span class="title">as_timeindex</span>
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
ThreadValue::defined() -> bool
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
auto thread = pluss::table::Thread(pluss::make_series({10.0}));
auto value = thread.value();

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
ThreadValue::host() -> HostAny
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
auto thread = pluss::table::Thread(pluss::make_series({10.0}));
auto value = thread.value();

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
ThreadValue::kind() -> ThreadKind
```

#### What It Does

Returns the underlying thread kind carried by this value wrapper.

#### Parameters

`None`

#### Returns

`ThreadKind`
    Value produced by this call.

#### Examples

Example 1. Inspect the `kind` property of the current object.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0}));
auto value = thread.value();

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
ThreadValue::payload() -> Payload
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
auto thread = pluss::table::Thread(pluss::make_series({10.0}));
auto value = thread.value();

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
  <a class="next-link" href="#pluss::Note">
    <span class="label">Next</span>
    <span class="title">pluss::Note</span>
  </a>
</div>

### `pluss::Note`

#### Signature

```cpp
ThreadValue::pluss::Note() -> operator
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
auto thread = pluss::table::Thread(pluss::make_series({10.0}));
auto value = thread.value();

auto out = value.pluss::Note();
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
  <a class="next-link" href="#pluss::Series">
    <span class="label">Next</span>
    <span class="title">pluss::Series</span>
  </a>
</div>

### `pluss::Series`

#### Signature

```cpp
ThreadValue::pluss::Series() -> operator
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
auto thread = pluss::table::Thread(pluss::make_series({10.0}));
auto value = thread.value();

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
  <a class="next-link" href="#pluss::TimeIndex">
    <span class="label">Next</span>
    <span class="title">pluss::TimeIndex</span>
  </a>
</div>

### `pluss::TimeIndex`

#### Signature

```cpp
ThreadValue::pluss::TimeIndex() -> operator
```

#### What It Does

Uses the implicit conversion operator to materialize the wrapped value as a `pluss::TimeIndex`.

#### Parameters

`None`

#### Returns

`operator`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0}));
auto value = thread.value();

auto out = value.pluss::TimeIndex();
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
  <a class="next-link" href="#pluss::Timeline">
    <span class="label">Next</span>
    <span class="title">pluss::Timeline</span>
  </a>
</div>

### `pluss::Timeline`

#### Signature

```cpp
ThreadValue::pluss::Timeline() -> operator
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
auto thread = pluss::table::Thread(pluss::make_series({10.0}));
auto value = thread.value();

auto out = value.pluss::Timeline();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#pluss::TimeIndex">
    <span class="label">Previous</span>
    <span class="title">pluss::TimeIndex</span>
  </a>
  <a class="next-link" href="#thread">
    <span class="label">Next</span>
    <span class="title">thread</span>
  </a>
</div>

## T

### `thread`

#### Signature

```cpp
ThreadValue::thread() -> const Thread&
```

#### What It Does

Returns the wrapped single-value `Thread` object.

#### Parameters

`None`

#### Returns

`const Thread&`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0}));
auto value = thread.value();

auto out = value.thread();
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

### `to_dbl`

#### Signature

```cpp
ThreadValue::to_dbl() -> double
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
auto thread = pluss::table::Thread(pluss::make_series({10.0}));
auto value = thread.value();

auto out = value.to_dbl();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#thread">
    <span class="label">Previous</span>
    <span class="title">thread</span>
  </a>
  <a class="next-link" href="#to_int">
    <span class="label">Next</span>
    <span class="title">to_int</span>
  </a>
</div>

### `to_int`

#### Signature

```cpp
ThreadValue::to_int() -> std::int64_t
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
auto thread = pluss::table::Thread(pluss::make_series({10.0}));
auto value = thread.value();

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
ThreadValue::to_str() -> std::string
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
auto thread = pluss::table::Thread(pluss::make_series({10.0}));
auto value = thread.value();

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

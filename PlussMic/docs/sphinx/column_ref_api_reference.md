# ColumnRef API Reference

This page documents the public `ColumnRef` helper returned by DataFrame column-reference calls.

## How To Use This Page

- Methods are grouped alphabetically so you can jump quickly from the left rail.
- Each method page section starts with the public signatures, then explains what the method does, how each parameter changes behavior, and what the call returns.
- Overloaded methods include several examples so you can see the supported call shapes instead of reverse-engineering them from signatures alone.
- Each example is followed immediately by verified output captured from the current compiled examples so the result shape is visible before you move to the next overload.

## H

### `head`

#### Signature

```cpp
ColumnRef::head(std::int64_t n) -> Thread
```

#### What It Does

Returns the first `n` values from the referenced column as a thread.

#### Parameters

**n**
    Number of leading values to preview from the referenced feature.

#### Returns

`Thread`
    Value produced by this call.

#### Examples

Example 1. Inspect the first part of the current object without changing the source data.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto ref = pluss::table::ColumnRef("sales", df.features("sales"));

auto out = ref.head(2);
```

##### Output

```text
[100.0, 120.0]
```
<div class="pluss-inline-prev-next">
  <a class="next-link" href="#mean">
    <span class="label">Next</span>
    <span class="title">mean</span>
  </a>
</div>

## M

### `mean`

#### Signature

```cpp
ColumnRef::mean() -> pluss::Series
```

#### What It Does

Computes the mean of the referenced numeric column.

#### Parameters

`None`

#### Returns

`pluss::Series`
    Value produced by this call.

#### Examples

Example 1. Compute `mean` and inspect the resulting summary value or summary object.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto ref = pluss::table::ColumnRef("sales", df.features("sales"));

auto out = ref.mean();
```

##### Output

```text
103.3
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#head">
    <span class="label">Previous</span>
    <span class="title">head</span>
  </a>
  <a class="next-link" href="#name">
    <span class="label">Next</span>
    <span class="title">name</span>
  </a>
</div>

## N

### `name`

#### Signature

```cpp
ColumnRef::name() -> const std::string&
```

#### What It Does

Returns the feature name associated with this column reference.

#### Parameters

`None`

#### Returns

`const std::string&`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto ref = pluss::table::ColumnRef("sales", df.features("sales"));

auto out = ref.name();
```

##### Output

```text
sales
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#mean">
    <span class="label">Previous</span>
    <span class="title">mean</span>
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
ColumnRef::thread() -> const Thread&
```

#### What It Does

Returns the referenced column as a `Thread` object.

#### Parameters

`None`

#### Returns

`const Thread&`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto ref = pluss::table::ColumnRef("sales", df.features("sales"));

auto out = ref.thread();
```

##### Output

```text
[10.0, 20.0, 30.0]
```

#### Practical Behavior

- `thread()` gives you the exact referenced column payload without copying it into a new frame.
- Use this when you want to switch into `Thread` operations such as comparisons, casting, or text cleanup.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#name">
    <span class="label">Previous</span>
    <span class="title">name</span>
  </a>
  <a class="next-link" href="#value">
    <span class="label">Next</span>
    <span class="title">value</span>
  </a>
</div>

## V

### `value`

#### Signature

```cpp
ColumnRef::value() -> ThreadValue
```

#### What It Does

Returns one scalar-like wrapper for the first referenced value, which is useful when you need typed single-cell access.

#### Parameters

`None`

#### Returns

`ThreadValue`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto ref = pluss::table::ColumnRef("sales", df.features("sales"));

auto out = ref.value();
```

##### Output

```text
100.0
```

#### Practical Behavior

- `value()` returns a `ThreadValue` wrapper over the first element of the referenced column.
- Use this when you need typed scalar extraction rather than a whole-column thread.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#thread">
    <span class="label">Previous</span>
    <span class="title">thread</span>
  </a>
  <a class="next-link" href="#values">
    <span class="label">Next</span>
    <span class="title">values</span>
  </a>
</div>

### `values`

#### Signature

```cpp
ColumnRef::values() -> Thread
```

#### What It Does

Materializes the full referenced column as a standalone `Thread` so you can reuse it outside the parent frame.

#### Parameters

`None`

#### Returns

`Thread`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto ref = pluss::table::ColumnRef("sales", df.features("sales"));

auto out = ref.values();
```

##### Output

```text
[10.0, 20.0, 30.0]
```

#### Practical Behavior

- `values()` materializes the referenced feature as a standalone `Thread`.
- Use this when you want to pass the column into another API that expects a thread rather than a column reference.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#value">
    <span class="label">Previous</span>
    <span class="title">value</span>
  </a>
</div>

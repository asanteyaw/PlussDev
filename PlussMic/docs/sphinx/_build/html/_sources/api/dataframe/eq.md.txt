# DataFrame::eq

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::eq(const char* other) -> DataFrame

DataFrame::eq(const DataFrame& other) -> DataFrame

DataFrame::eq(const DataFrame& other, const AlignOptions& options) -> DataFrame

DataFrame::eq(const Thread& other, Axis axis = Axis::Feature) -> DataFrame

DataFrame::eq(const Thread& other, Axis axis, const AlignOptions& options) -> DataFrame

DataFrame::eq(const Thread& other, const AlignOptions& options, Axis axis = Axis::Feature) -> DataFrame

DataFrame::eq(double other) -> DataFrame

DataFrame::eq(std::string_view other) -> DataFrame
```

Compare the current frame for equality against a scalar, a thread, or another frame and return a boolean-style result frame.

`eq(...)` is the explicit named equality surface when you want a reusable mask table instead of operator syntax.

## Parameters

**other**

Right-hand equality target.

Supported overload families include:

- numeric scalar broadcast such as `84.0`,
- text scalar broadcast such as `"Ada"`,
- `const Thread&` when you want one thread compared across a chosen axis,
- `const DataFrame&` when you want cellwise frame-to-frame equality.

**axis**

Axis used by the thread overload.

- `Axis::Feature`: compare one thread across features.
- `Axis::Case`: compare one thread across cases.

Use the examples below as the primary guide for when a thread is being interpreted row-wise versus column-wise.

**options**

Optional alignment rules for overloads that compare against another `DataFrame` or `Thread`.

This is where you choose label-aware matching behavior such as `AlignBy::Both` when the right-hand side has a different row order or a smaller structural footprint.

## Returns

`DataFrame`
    Boolean-style frame marking which participating cells compare equal under the selected overload.

## See Also

- <a href="equals.html"><code>DataFrame::equals</code></a>: whole-frame exact identity check that returns one `bool` instead of a mask frame.
- <a href="ne.html"><code>DataFrame::ne</code></a>: not-equal comparison.
- <a href="../thread/eq.html"><code>Thread::eq</code></a>: equality comparison on one extracted thread.

## Notes

Numeric scalar equality broadcasts one value across compatible numeric features.

Text scalar equality is most useful on note-backed columns. It maps the comparison across the current frame's threads and returns a boolean-style frame.

Frame-to-frame equality compares matched cells. When you need explicit structural matching rules, use the overload that accepts `AlignOptions`.

## Examples

Example 1. Broadcast one numeric scalar across the current frame.

```cpp
using namespace pluss::table;

auto scores = DataFrame({
    {"quiz", pluss::make_series({84.0, 90.0, 84.0})},
    {"final", pluss::make_series({84.0, 88.0, 72.0})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto out = scores.eq(84.0);
```

### Output

```text
===================
     | quiz   final
-------------------
s1   | true   true
s2   | false  false
s3   | true   false
-------------------
[3 cs x 2 feat]
```

Example 2. Broadcast one text scalar across a note-backed feature.

```cpp
using namespace pluss::table;

auto names = DataFrame({
    {"name", pluss::make_note({"Ada", "Bola", "Ada"})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto out = names.eq("Ada");
```

### Output

```text
============
     | name
------------
s1   | true
s2   | false
s3   | true
------------
[3 cs x 1 feat]
```

Example 3. Compare two frames cell by cell when they already share the same structure.

```cpp
using namespace pluss::table;

auto official = DataFrame({
    {"quiz", pluss::make_series({84.0, 90.0, 84.0})},
    {"final", pluss::make_series({84.0, 88.0, 72.0})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto changed = DataFrame({
    {"quiz", pluss::make_series({84.0, 90.0, 83.0})},
    {"final", pluss::make_series({84.0, 88.0, 72.0})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto out = official.eq(changed);
```

### Output

```text
===================
     | quiz   final
-------------------
s1   | true   true
s2   | true   true
s3   | false  true
-------------------
[3 cs x 2 feat]
```

Example 4. Use `AlignOptions` when the right-hand frame has a different row and feature order.

```cpp
using namespace pluss::table;

auto official = DataFrame({
    {"quiz", pluss::make_series({84.0, 90.0, 84.0})},
    {"final", pluss::make_series({84.0, 88.0, 72.0})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto reordered = DataFrame({
    {"final", pluss::make_series({72.0, 84.0})},
    {"quiz", pluss::make_series({84.0, 84.0})},
}, pluss::make_note({"s3", "s1"}));

auto out = official.eq(reordered, AlignOptions{.align = AlignBy::Both});
```

### Output

```text
==================
     | quiz  final
------------------
s1   | true  true
s2   | NA    NA
s3   | true  true
------------------
[3 cs x 2 feat]
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="duplicated.html">
    <span class="label">Previous</span>
    <span class="title">duplicated</span>
  </a>
  <a class="next-link" href="equals.html">
    <span class="label">Next</span>
    <span class="title">equals</span>
  </a>
</div>

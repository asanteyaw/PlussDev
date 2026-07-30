<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 80574f1c2930 -->

# DataFrame::assign

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::assign(pluss::RangeView<DeriveSpec> specs) -> DataFrame

DataFrame::assign(pluss::RangeView<std::pair<std::string, Thread>> assignments) -> DataFrame

DataFrame::assign(std::string_view name, const Thread& value) -> DataFrame

DataFrame::assign(std::string_view name, const std::function<Thread(const DataFrame&)>& expr) -> DataFrame
```

Add or replace one or more features on a DataFrame and return the resulting frame.

`DataFrame::assign(...)` is the main column-creation surface when you want a pipeline-friendly, non-mutating way to engineer features. The two collection overloads accept several features at once — one as already-computed `Thread` values, the other as `DeriveSpec` expressions so PlussMic can compute the features inside the call. The two single-feature overloads are thin convenience shims over those same two paths, for the common case of assigning just one feature: `assign(name, value)` is exactly `assign({{std::string(name), value}})`, and `assign(name, expr)` is exactly `assign({DeriveSpec{.feature = std::string(name), .expr = expr}})`.

The motivation is the same as any non-mutating feature-creation call: instead of
mutating a frame in a sequence of separate statements
(`df["total"] = ...; df["passed"] = ...;`), `assign()` lets a chain of feature
derivations read as a single expression —
`df.assign({...}).filter(...).arrange({...})` — which keeps intermediate,
partially-derived frames out of named variables you would otherwise have to track.

## Parameters

**assignments**

Collection of `(feature_name, thread)` pairs.

- Use this overload when each output feature is already computed.
- If a feature name already exists, the existing feature is replaced.
- If a feature name is new, the feature is appended to the frame.

**specs**

Collection of `DeriveSpec` instructions.

Each spec provides:

- `feature`: the output feature name.
- `expr`: a callback that receives the current frame and returns the derived `Thread`.

Use this overload when you want the frame to compute the derived features inside the `assign(...)` call itself.

**name** (single-feature overloads)

The output feature name. Empty throws the same `"DataFrame::assign: feature name cannot be empty"` check as the collection overloads, since these are thin shims over them.

**value** (single-feature, `Thread` overload)

An already-computed `Thread` to assign under `name` — equivalent to a one-element `assignments` collection.

**expr** (single-feature, callback overload)

A callback receiving the current frame and returning the derived `Thread` — equivalent to a one-element `specs` collection with `feature = name`.

## Returns

`DataFrame`
    A new frame with the requested features added or replaced.

    Existing features keep their current order. New features are appended in the order you supply them unless a name collision causes one to overwrite an existing feature in place.

## See Also

- <a href="transform.html"><code>DataFrame::transform</code></a>: compute one derived thread from a callback when you are working row-wise or feature-wise.
- <a href="set.html"><code>DataFrame::set</code></a>: write one feature directly when you already have the final thread.
- <a href="put.html"><code>DataFrame::put</code></a>: update selected cells rather than constructing whole derived features.

## Notes

The `assignments` overload is best when you already computed the target `Thread` values before the call.

The `DeriveSpec` overload evaluates specs from left to right against the progressively updated frame. That means later specs in the same call can depend on features created by earlier specs.

`assign(...)` is non-mutating. The current frame is not edited in place.

The two single-feature overloads (`assign(name, value)` and `assign(name, expr)`) are pure convenience — each is implemented as a one-element call into the matching collection overload, so they share the same empty-name check, the same replace-in-place-if-the-name-exists behavior, and the same non-mutating semantics. Reach for them when you're assigning exactly one feature and don't want the syntactic overhead of a collection literal.

## Examples

Example 1. Assign a feature from a thread you already computed.

```cpp
using namespace pluss::table;

auto grades = DataFrame({
    {"student", pluss::make_note({"Ada", "Bola", "Chen"})},
    {"assignment", pluss::make_series({9.5, 8.0, 7.5})},
    {"final", pluss::make_series({28.0, 24.0, 21.5})},
}, pluss::make_note({"s1", "s2", "s3"})).title("grades");

auto total_grade = [](const DataFrame& df) {
  return df.features("assignment") + df.features("final");
};

auto out = grades.assign({
    std::pair<std::string, pluss::table::Thread>{"total", total_grade(grades)},
});
std::cout << out;
```

### Output

```text
========================================
     | student  assignment  final  total
----------------------------------------
s1   | Ada      9.5         28     37.5
s2   | Bola     8           24     32
s3   | Chen     7.5         21.5   29
----------------------------------------
[3 cs x 4 feat]
```

Example 2. Compute several derived features inside one assign call.

```cpp
using namespace pluss::table;

auto grades = DataFrame({
    {"student", pluss::make_note({"Ada", "Bola", "Chen"})},
    {"assignment", pluss::make_series({9.5, 8.0, 7.5})},
    {"final", pluss::make_series({28.0, 24.0, 21.5})},
}, pluss::make_note({"s1", "s2", "s3"})).title("grades");

auto total_grade = [](const DataFrame& df) {
  return df.features("assignment") + df.features("final");
};

auto out = grades.assign({
    DataFrame::DeriveSpec{
        .feature = "total",
        .expr = total_grade,
    },
    DataFrame::DeriveSpec{
        .feature = "passed",
        .expr = [](const DataFrame& df) {
          return df.features("total") >= 30.0;
        },
    },
});
std::cout << out;
```

### Output

```text
================================================
     | student  assignment  final  total  passed
------------------------------------------------
s1   | Ada      9.5         28     37.5   true
s2   | Bola     8           24     32     true
s3   | Chen     7.5         21.5   29     false
------------------------------------------------
[3 cs x 5 feat]
```

Example 3. The single-feature overloads assign exactly one feature at a time — one from an
already-computed `Thread`, the other from a callback — without wrapping it in a collection
literal.

```cpp
using namespace pluss::table;

auto grades = DataFrame({
    {"student", pluss::make_note({"Ada", "Bola", "Chen"})},
    {"assignment", pluss::make_series({9.5, 8.0, 7.5})},
    {"final", pluss::make_series({28.0, 24.0, 21.5})},
}, pluss::make_note({"s1", "s2", "s3"})).title("grades");

auto total_thread = Thread(pluss::make_series({37.5, 32.0, 29.0}));
auto out1 = grades.assign("total", total_thread);
std::cout << out1;

auto out2 = grades.assign("total", [](const DataFrame& df) {
  return df.features("assignment") + df.features("final");
});
std::cout << out2;
```

### Output

```text
========================================
     | student  assignment  final  total
----------------------------------------
s1   | Ada      9.5         28     37.5 
s2   | Bola     8           24     32   
s3   | Chen     7.5         21.5   29   
----------------------------------------
[3 cs x 4 feat]========================================
     | student  assignment  final  total
----------------------------------------
s1   | Ada      9.5         28     37.5 
s2   | Bola     8           24     32   
s3   | Chen     7.5         21.5   29   
----------------------------------------
[3 cs x 4 feat]
```

Example 4. Assigning under a name that already exists replaces it in place — same
collision behavior as the collection overloads.

```cpp
using namespace pluss::table;

auto grades = DataFrame({
    {"student", pluss::make_note({"Ada", "Bola", "Chen"})},
    {"assignment", pluss::make_series({9.5, 8.0, 7.5})},
    {"final", pluss::make_series({28.0, 24.0, 21.5})},
}, pluss::make_note({"s1", "s2", "s3"})).title("grades");

auto with_total = grades.assign("total", [](const DataFrame& df) {
  return df.features("assignment") + df.features("final");
});
auto doubled = with_total.assign("total", [](const DataFrame& df) {
  return df.features("total") * 2.0;
});
std::cout << doubled;
```

### Output

```text
========================================
     | student  assignment  final  total
----------------------------------------
s1   | Ada      9.5         28     75   
s2   | Bola     8           24     64   
s3   | Chen     7.5         21.5   58   
----------------------------------------
[3 cs x 4 feat]
```

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="any.html">
    <span class="label">Previous</span>
    <span class="title">any</span>
  </a>
  <a class="next-link" href="astype.html">
    <span class="label">Next</span>
    <span class="title">astype</span>
  </a>
</div>

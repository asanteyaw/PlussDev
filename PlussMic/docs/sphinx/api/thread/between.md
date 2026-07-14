# Thread::between

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::between(const Thread& lo, const Thread& hi, bool include_left, bool include_right) -> Thread

Thread::between(const Thread& lo, const Thread& hi, bool inclusive = true) -> Thread

Thread::between(double a, double b, bool include_left, bool include_right) -> Thread

Thread::between(double a, double b, bool inclusive = false) -> Thread

Thread::between(std::string_view lo, std::string_view hi, bool include_left, bool include_right) -> Thread

Thread::between(std::string_view lo, std::string_view hi, bool inclusive = true) -> Thread
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

**lo**
    Lower/upper boundary used by this overload.

**hi**
    Lower/upper boundary used by this overload.

**include_left**
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

**include_right**
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

**inclusive**
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

**a**
    Lower/upper boundary used by this overload.

**b**
    Lower/upper boundary used by this overload.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `between` to the current thread using explicit `lo, hi, include_left, include_right` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({45.0, 65.0, 82.0}));
auto lo = pluss::table::Thread(pluss::make_series({40.0, 60.0, 80.0}));
auto hi = pluss::table::Thread(pluss::make_series({50.0, 70.0, 81.0}));

auto out = thread.between(lo, hi, true);
```

### Output

```text
[true, true, false]
```
Example 2. Apply `between` to the current thread using explicit `lo, hi, inclusive` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({45.0, 65.0, 82.0}));
auto lo = pluss::table::Thread(pluss::make_series({40.0, 60.0, 80.0}));
auto hi = pluss::table::Thread(pluss::make_series({50.0, 70.0, 81.0}));

auto out = thread.between(lo, hi, true);
```

### Output

```text
[true, true, false]
```
Example 3. Apply `between` to the current thread using explicit `a, b, include_left, include_right` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({45.0, 65.0, 82.0, 101.0}));

auto out = thread.between(50.0, 100.0, true);
```

### Output

```text
[false, true, true, false]
```
Example 4. Apply `between` to the current thread using explicit `a, b, inclusive` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({45.0, 65.0, 82.0, 101.0}));

auto out = thread.between(50.0, 100.0, true);
```

### Output

```text
[false, true, true, false]
```
Example 5. Apply `between` to the current thread using explicit `lo, hi, include_left, include_right` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_timeline({
    "2024-01-05T09:00:00Z",
    "2024-01-18T12:00:00Z",
    "2024-02-02T08:00:00Z",
}));

auto out = thread.between("2024-01-01T00:00:00Z", "2024-01-31T23:59:59Z", true);
```

### Output

```text
[true, true, false]
```
Example 6. Apply `between` to the current thread using explicit `lo, hi, inclusive` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_timeline({
    "2024-01-05T09:00:00Z",
    "2024-01-18T12:00:00Z",
    "2024-02-02T08:00:00Z",
}));

auto out = thread.between("2024-01-01T00:00:00Z", "2024-01-31T23:59:59Z", true);
```

### Output

```text
[true, true, false]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="astype.html">
    <span class="label">Previous</span>
    <span class="title">astype</span>
  </a>
  <a class="next-link" href="bfill.html">
    <span class="label">Next</span>
    <span class="title">bfill</span>
  </a>
</div>

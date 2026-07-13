# Thread::put

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::put(const pluss::Series& bool_mask, const char* value) -> Thread

Thread::put(const pluss::Series& bool_mask, const Thread& value) -> Thread

Thread::put(const pluss::Series& bool_mask, double value) -> Thread

Thread::put(const pluss::Series& bool_mask, pluss::null_t value) -> Thread

Thread::put(const pluss::Series& bool_mask, std::string_view value) -> Thread

Thread::put(const Thread& bool_mask, const char* value) -> Thread

Thread::put(const Thread& bool_mask, const Thread& value) -> Thread

Thread::put(const Thread& bool_mask, double value) -> Thread

Thread::put(const Thread& bool_mask, pluss::null_t value) -> Thread

Thread::put(const Thread& bool_mask, std::string_view value) -> Thread
```

## What It Does

Writes replacement values into positions selected by a boolean mask.

## Parameters

**bool_mask**
    Boolean row mask. `true` keeps or rewrites the corresponding row, while `false` leaves it out or leaves the original value unchanged depending on the method.

**value**
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `put` to the current thread using explicit `bool_mask, value` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.put({}, "demo");
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 2. Apply `put` to the current thread using explicit `bool_mask, value` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto other = pluss::table::Thread(pluss::make_series({15.0, 10.0, 35.0}));

auto out = thread.put({}, other);
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 3. Apply `put` to the current thread using explicit `bool_mask, value` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.put({}, 10.0);
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 4. Apply `put` to the current thread using explicit `bool_mask, value` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.put({}, {});
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 5. Apply `put` to the current thread using explicit `bool_mask, value` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.put({}, "demo");
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 6. Apply `put` to the current thread using explicit `bool_mask, value` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto other = pluss::table::Thread(pluss::make_series({15.0, 10.0, 35.0}));

auto out = thread.put(other, "demo");
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 7. Apply `put` to the current thread using explicit `bool_mask, value` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto other = pluss::table::Thread(pluss::make_series({15.0, 10.0, 35.0}));
auto other = pluss::table::Thread(pluss::make_series({15.0, 10.0, 35.0}));

auto out = thread.put(other, other);
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 8. Apply `put` to the current thread using explicit `bool_mask, value` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto other = pluss::table::Thread(pluss::make_series({15.0, 10.0, 35.0}));

auto out = thread.put(other, 10.0);
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 9. Apply `put` to the current thread using explicit `bool_mask, value` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto other = pluss::table::Thread(pluss::make_series({15.0, 10.0, 35.0}));

auto out = thread.put(other, {});
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 10. Apply `put` to the current thread using explicit `bool_mask, value` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto other = pluss::table::Thread(pluss::make_series({15.0, 10.0, 35.0}));

auto out = thread.put(other, "demo");
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="percentiles.html">
    <span class="label">Previous</span>
    <span class="title">percentiles</span>
  </a>
  <a class="next-link" href="rank.html">
    <span class="label">Next</span>
    <span class="title">rank</span>
  </a>
</div>

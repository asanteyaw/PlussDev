# Thread::select

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::select(pluss::RangeView<Thread> conditions, pluss::RangeView<Thread> choices, const char* default_value) -> Thread

Thread::select(pluss::RangeView<Thread> conditions, pluss::RangeView<Thread> choices, const pluss::TimeIndex& default_value) -> Thread

Thread::select(pluss::RangeView<Thread> conditions, pluss::RangeView<Thread> choices, const pluss::Timeline& default_value) -> Thread

Thread::select(pluss::RangeView<Thread> conditions, pluss::RangeView<Thread> choices, const Thread& default_value) -> Thread

Thread::select(pluss::RangeView<Thread> conditions, pluss::RangeView<Thread> choices, double default_value) -> Thread

Thread::select(pluss::RangeView<Thread> conditions, pluss::RangeView<Thread> choices, pluss::null_t default_value) -> Thread

Thread::select(pluss::RangeView<Thread> conditions, pluss::RangeView<Thread> choices, std::string_view default_value) -> Thread

Thread::select(std::initializer_list<Thread> conditions, std::initializer_list<Thread> choices, const char* default_value) -> Thread

Thread::select(std::initializer_list<Thread> conditions, std::initializer_list<Thread> choices, const pluss::TimeIndex& default_value) -> Thread

Thread::select(std::initializer_list<Thread> conditions, std::initializer_list<Thread> choices, const pluss::Timeline& default_value) -> Thread

Thread::select(std::initializer_list<Thread> conditions, std::initializer_list<Thread> choices, const Thread& default_value) -> Thread

Thread::select(std::initializer_list<Thread> conditions, std::initializer_list<Thread> choices, double default_value) -> Thread

Thread::select(std::initializer_list<Thread> conditions, std::initializer_list<Thread> choices, pluss::null_t default_value) -> Thread

Thread::select(std::initializer_list<Thread> conditions, std::initializer_list<Thread> choices, std::string_view default_value) -> Thread
```

## What It Does

Use this to reduce the current object to the rows or fields that matter for the next step.

## Parameters

**conditions**
    Ordered collection used by a select-style overload. Each condition or choice is matched positionally with the others.

**choices**
    Ordered collection used by a select-style overload. Each condition or choice is matched positionally with the others.

**default_value**
    Fallback value written anywhere the input is missing or unmatched.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Pass several thread values together in one call.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.select({}, {}, "demo");
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 2. Pass several thread values together in one call.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.select({}, {}, {});
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 3. Pass several thread values together in one call.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.select({}, {}, {});
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 4. Pass several thread values together in one call.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto other = pluss::table::Thread(pluss::make_series({15.0, 10.0, 35.0}));

auto out = thread.select({}, {}, other);
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 5. Pass several thread values together in one call.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.select({}, {}, 10.0);
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 6. Pass several thread values together in one call.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.select({}, {}, {});
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 7. Pass several thread values together in one call.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.select({}, {}, "demo");
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 8. Pass several thread values together in one call.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.select({}, {}, "demo");
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 9. Pass several thread values together in one call.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.select({}, {}, {});
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 10. Pass several thread values together in one call.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.select({}, {}, {});
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 11. Pass several thread values together in one call.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto other = pluss::table::Thread(pluss::make_series({15.0, 10.0, 35.0}));

auto out = thread.select({}, {}, other);
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 12. Pass several thread values together in one call.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.select({}, {}, 10.0);
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 13. Pass several thread values together in one call.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.select({}, {}, {});
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 14. Pass several thread values together in one call.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.select({}, {}, "demo");
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="se.html">
    <span class="label">Previous</span>
    <span class="title">se</span>
  </a>
  <a class="next-link" href="shape.html">
    <span class="label">Next</span>
    <span class="title">shape</span>
  </a>
</div>

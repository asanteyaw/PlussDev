# Thread::fillnull

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::fillnull(const char* value) -> Thread

Thread::fillnull(const pluss::TimeIndex& value) -> Thread

Thread::fillnull(const pluss::Timeline& value) -> Thread

Thread::fillnull(double value) -> Thread

Thread::fillnull(std::string_view value) -> Thread
```

## What It Does

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

## Parameters

**value**
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillnull(...)`, or `replace(...)`.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `fillnull` to the current thread using explicit `value` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.fillnull("Unknown");
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 2. Apply `fillnull` to the current thread using explicit `value` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_timeindex({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z"}));

auto out = thread.fillnull(pluss::make_timeindex("2024-01-01T00:00:00Z"));
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 3. Apply `fillnull` to the current thread using explicit `value` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z", "2024-03-05T15:30:00Z"}));

auto out = thread.fillnull(pluss::make_timeline("2024-01-01T00:00:00Z"));
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 4. Apply `fillnull` to the current thread using explicit `value` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.fillnull(0.0);
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 5. Apply `fillnull` to the current thread using explicit `value` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.fillnull("Unknown");
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ffill.html">
    <span class="label">Previous</span>
    <span class="title">ffill</span>
  </a>
  <a class="next-link" href="fillnull_.html">
    <span class="label">Next</span>
    <span class="title">fillnull_</span>
  </a>
</div>

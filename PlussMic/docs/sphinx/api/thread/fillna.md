# Thread::fillna

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::fillna(const char* value) -> Thread

Thread::fillna(const pluss::TimeIndex& value) -> Thread

Thread::fillna(const pluss::Timeline& value) -> Thread

Thread::fillna(double value) -> Thread

Thread::fillna(std::string_view value) -> Thread
```

## What It Does

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

## Parameters

**value**
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `fillna` to the current thread using explicit `value` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.fillna("Unknown");
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 2. Apply `fillna` to the current thread using explicit `value` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_timeindex({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z"}));

auto out = thread.fillna(pluss::make_timeindex("2024-01-01T00:00:00Z"));
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 3. Apply `fillna` to the current thread using explicit `value` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-02-01T11:00:00Z", "2024-03-05T15:30:00Z"}));

auto out = thread.fillna(pluss::make_timeline("2024-01-01T00:00:00Z"));
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 4. Apply `fillna` to the current thread using explicit `value` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.fillna(0.0);
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 5. Apply `fillna` to the current thread using explicit `value` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.fillna("Unknown");
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
  <a class="next-link" href="fillna_.html">
    <span class="label">Next</span>
    <span class="title">fillna_</span>
  </a>
</div>

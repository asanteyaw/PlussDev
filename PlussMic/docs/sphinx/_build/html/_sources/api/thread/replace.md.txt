# Thread::replace

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::replace(const pluss::Note& from, const pluss::Note& to) -> Thread

Thread::replace(const pluss::Note& from, pluss::null_t to) -> Thread

Thread::replace(const pluss::Series& from, const pluss::Series& to) -> Thread

Thread::replace(const pluss::Series& from, pluss::null_t to) -> Thread

Thread::replace(const pluss::TimeIndex& from, const pluss::TimeIndex& to) -> Thread

Thread::replace(const pluss::TimeIndex& from, pluss::null_t to) -> Thread

Thread::replace(const pluss::Timeline& from, const pluss::Timeline& to) -> Thread

Thread::replace(const pluss::Timeline& from, pluss::null_t to) -> Thread

Thread::replace(double from, double to) -> Thread

Thread::replace(double from, pluss::null_t to) -> Thread

Thread::replace(pluss::null_t from, const pluss::Note& to) -> Thread

Thread::replace(pluss::null_t from, const pluss::Series& to) -> Thread

Thread::replace(pluss::null_t from, const pluss::TimeIndex& to) -> Thread

Thread::replace(pluss::null_t from, const pluss::Timeline& to) -> Thread

Thread::replace(pluss::null_t from, double to) -> Thread

Thread::replace(std::string_view from, std::string_view to) -> Thread
```

## What It Does

This is part of the text-processing surface for note-backed or text-oriented data.

## Parameters

**from**
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

**to**
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `replace` to the current thread using explicit `from, to` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.replace("Tema", "Tamale");
```

### Output

```text
["Accra", "Tema", "Kumasi"]
```
Example 2. Apply `replace` to the current thread using explicit `from, to` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.replace("Tema", "Tamale");
```

### Output

```text
["Accra", "Tema", "Kumasi"]
```
Example 3. Apply `replace` to the current thread using explicit `from, to` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.replace("Tema", "Tamale");
```

### Output

```text
["Accra", "Tema", "Kumasi"]
```
Example 4. Apply `replace` to the current thread using explicit `from, to` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.replace("Tema", "Tamale");
```

### Output

```text
["Accra", "Tema", "Kumasi"]
```
Example 5. Apply `replace` to the current thread using explicit `from, to` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.replace("Tema", "Tamale");
```

### Output

```text
["Accra", "Tema", "Kumasi"]
```
Example 6. Apply `replace` to the current thread using explicit `from, to` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.replace("Tema", "Tamale");
```

### Output

```text
["Accra", "Tema", "Kumasi"]
```
Example 7. Apply `replace` to the current thread using explicit `from, to` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.replace("Tema", "Tamale");
```

### Output

```text
["Accra", "Tema", "Kumasi"]
```
Example 8. Apply `replace` to the current thread using explicit `from, to` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.replace("Tema", "Tamale");
```

### Output

```text
["Accra", "Tema", "Kumasi"]
```
Example 9. Apply `replace` to the current thread using explicit `from, to` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.replace("Tema", "Tamale");
```

### Output

```text
["Accra", "Tema", "Kumasi"]
```
Example 10. Apply `replace` to the current thread using explicit `from, to` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.replace("Tema", "Tamale");
```

### Output

```text
["Accra", "Tema", "Kumasi"]
```
Example 11. Apply `replace` to the current thread using explicit `from, to` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.replace("Tema", "Tamale");
```

### Output

```text
["Accra", "Tema", "Kumasi"]
```
Example 12. Apply `replace` to the current thread using explicit `from, to` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.replace("Tema", "Tamale");
```

### Output

```text
["Accra", "Tema", "Kumasi"]
```
Example 13. Apply `replace` to the current thread using explicit `from, to` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.replace("Tema", "Tamale");
```

### Output

```text
["Accra", "Tema", "Kumasi"]
```
Example 14. Apply `replace` to the current thread using explicit `from, to` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.replace("Tema", "Tamale");
```

### Output

```text
["Accra", "Tema", "Kumasi"]
```
Example 15. Apply `replace` to the current thread using explicit `from, to` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.replace("Tema", "Tamale");
```

### Output

```text
["Accra", "Tema", "Kumasi"]
```
Example 16. Apply `replace` to the current thread using explicit `from, to` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.replace("Tema", "Tamale");
```

### Output

```text
["Accra", "Tema", "Kumasi"]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rank.html">
    <span class="label">Previous</span>
    <span class="title">rank</span>
  </a>
  <a class="next-link" href="rstrip.html">
    <span class="label">Next</span>
    <span class="title">rstrip</span>
  </a>
</div>

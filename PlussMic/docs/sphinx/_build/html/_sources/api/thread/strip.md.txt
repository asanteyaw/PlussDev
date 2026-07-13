# Thread::strip

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::strip() -> Thread

Thread::strip(std::string_view chars) -> Thread
```

## What It Does

Trims note-backed text values. Use `strip()` for whitespace or `strip(chars)` for an explicit trim set.

## Parameters

**chars**
    Characters to trim from the edge of each text value.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `strip` to the current thread values and inspect the result.

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.strip();
```

### Output

```text
["Accra", "Tema", "Kumasi"]
```
Example 2. Apply `strip` to the current thread using explicit `chars` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.strip("# ");
```

### Output

```text
["Accra", "Tema", "Kumasi"]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="std.html">
    <span class="label">Previous</span>
    <span class="title">std</span>
  </a>
  <a class="next-link" href="sum.html">
    <span class="label">Next</span>
    <span class="title">sum</span>
  </a>
</div>

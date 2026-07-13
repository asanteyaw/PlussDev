# Thread::lstrip

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::lstrip() -> Thread

Thread::lstrip(std::string_view chars) -> Thread
```

## What It Does

Trims only the left edge of note-backed text values. This is useful for prefixes such as leading `#` in IDs.

## Parameters

**chars**
    Characters to trim from the edge of each text value.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `lstrip` to the current thread values and inspect the result.

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.lstrip();
```

### Output

```text
["Accra", "Tema", "Kumasi"]
```
Example 2. Apply `lstrip` to the current thread using explicit `chars` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"#203930", "##7788", "4455##"}));

auto out = thread.lstrip("#");
```

### Output

```text
["203930", "7788", "4455##"]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lower.html">
    <span class="label">Previous</span>
    <span class="title">lower</span>
  </a>
  <a class="next-link" href="map.html">
    <span class="label">Next</span>
    <span class="title">map</span>
  </a>
</div>

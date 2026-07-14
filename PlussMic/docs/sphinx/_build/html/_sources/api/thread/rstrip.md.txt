# Thread::rstrip

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::rstrip() -> Thread

Thread::rstrip(std::string_view chars) -> Thread
```

## What It Does

Trims only the right edge of note-backed text values. Use it when trailing markers should be removed without touching the left side.

## Parameters

**chars**
    Characters to trim from the edge of each text value.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `rstrip` to the current thread values and inspect the result.

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.rstrip();
```

### Output

```text
["Accra", "Tema", "Kumasi"]
```
Example 2. Apply `rstrip` to the current thread using explicit `chars` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"#203930", "##7788", "4455##"}));

auto out = thread.rstrip("#");
```

### Output

```text
["#203930", "##7788", "4455"]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rpow.html">
    <span class="label">Previous</span>
    <span class="title">rpow</span>
  </a>
  <a class="next-link" href="rsub.html">
    <span class="label">Next</span>
    <span class="title">rsub</span>
  </a>
</div>

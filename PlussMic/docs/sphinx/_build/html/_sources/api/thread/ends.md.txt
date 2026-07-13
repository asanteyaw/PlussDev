# Thread::ends

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::ends(std::string_view suffix) -> Thread
```

## What It Does

Checks whether each note-backed text value ends with the requested suffix.

## Parameters

**suffix**
    Text suffix appended after the existing label or value.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `ends` to the current thread using explicit `suffix` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.ends("demo");
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="encode.html">
    <span class="label">Previous</span>
    <span class="title">encode</span>
  </a>
  <a class="next-link" href="exp.html">
    <span class="label">Next</span>
    <span class="title">exp</span>
  </a>
</div>

# Thread::fullmatch

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::fullmatch(std::string_view pattern) -> Thread
```

## What It Does

Checks whether each note-backed text value fully matches the requested regular expression.

## Parameters

**pattern**
    Pattern string used for matching or extraction.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `fullmatch` to the current thread using explicit `pattern` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.fullmatch("Acc");
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="freq.html">
    <span class="label">Previous</span>
    <span class="title">freq</span>
  </a>
  <a class="next-link" href="has_cases.html">
    <span class="label">Next</span>
    <span class="title">has_cases</span>
  </a>
</div>

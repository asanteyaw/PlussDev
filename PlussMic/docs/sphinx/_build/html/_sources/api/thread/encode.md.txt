# Thread::encode

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::encode(std::string_view encoding = "base64") -> Thread
```

## What It Does

Encodes note-backed text values into numeric codes or related compact representations.

## Parameters

**encoding**
    Text encoding used when reading or writing text-based files.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `encode` to the current thread using explicit `encoding` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.encode("demo");
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="duplicated.html">
    <span class="label">Previous</span>
    <span class="title">duplicated</span>
  </a>
  <a class="next-link" href="ends.html">
    <span class="label">Next</span>
    <span class="title">ends</span>
  </a>
</div>

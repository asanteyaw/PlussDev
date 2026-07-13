# Thread::lower

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::lower() -> Thread
```

## What It Does

Applies an elementwise cleanup or transformation while preserving the surrounding alignment, labels, and payload family whenever possible.

## Parameters

`None`

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `lower` to the current thread values and inspect the result.

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.lower();
```

### Output

```text
["Accra", "Tema", "Kumasi"]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="log.html">
    <span class="label">Previous</span>
    <span class="title">log</span>
  </a>
  <a class="next-link" href="lstrip.html">
    <span class="label">Next</span>
    <span class="title">lstrip</span>
  </a>
</div>

# Thread::pad

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::pad(std::int64_t width, TextAlign align = TextAlign::Left, char fill = ' ') -> Thread
```

## What It Does

Pads note-backed text values to a target width.

## Parameters

**width**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

**align**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

**fill**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `pad` to the current thread using explicit `width, align, fill` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.pad(2, {}, {});
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="numeric_binary.html">
    <span class="label">Previous</span>
    <span class="title">numeric_binary</span>
  </a>
  <a class="next-link" href="partition.html">
    <span class="label">Next</span>
    <span class="title">partition</span>
  </a>
</div>

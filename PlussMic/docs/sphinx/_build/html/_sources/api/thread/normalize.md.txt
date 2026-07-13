# Thread::normalize

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::normalize(UnicodeForm form = UnicodeForm::NFC) -> Thread
```

## What It Does

Normalizes note-backed text values into the requested Unicode normalization form.

## Parameters

**form**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `normalize` to the current thread using explicit `form` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.normalize({});
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ndistinct.html">
    <span class="label">Previous</span>
    <span class="title">ndistinct</span>
  </a>
  <a class="next-link" href="note.html">
    <span class="label">Next</span>
    <span class="title">note</span>
  </a>
</div>

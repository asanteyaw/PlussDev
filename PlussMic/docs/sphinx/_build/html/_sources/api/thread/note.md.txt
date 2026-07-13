# Thread::note

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::note() -> ThreadNoteAccessor
```

## What It Does

Returns the current payload as a note/text object.

## Parameters

`None`

## Returns

`ThreadNoteAccessor`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto out = thread.note();
```

### Output

```text
["Accra", "Tema", "Kumasi"]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="normalize.html">
    <span class="label">Previous</span>
    <span class="title">normalize</span>
  </a>
  <a class="next-link" href="notnull.html">
    <span class="label">Next</span>
    <span class="title">notnull</span>
  </a>
</div>

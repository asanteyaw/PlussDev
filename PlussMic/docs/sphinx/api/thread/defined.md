# Thread::defined

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::defined() -> bool
```

Report whether the thread currently owns a usable backing payload.

This is the object-level integrity check for `Thread` itself.

It tells you whether the thread object exists as a real typed vector container, not whether every entry is non-missing.

## Parameters

`None`

## Returns

`bool`
    `true` when the thread object is defined and can participate in ordinary thread operations.

## Notes

`defined()` is about object validity. Use `isnull()`, `notnull()`, `dropnull()`, or payload-aware validity checks when your question is about missing entries inside the thread.

## Examples

Example 1. Check whether a thread object is defined.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = scores.defined();
```

### Output

```text
true
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="day.html">
    <span class="label">Previous</span>
    <span class="title">day</span>
  </a>
  <a class="next-link" href="describe.html">
    <span class="label">Next</span>
    <span class="title">describe</span>
  </a>
</div>

# Thread::duplicated

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::duplicated(bool keep_first = true, bool keep_last = false) -> Thread
```

Build a boolean mask that marks duplicate entries.

This is the thread-level inspection route behind duplicate-aware filtering and counting workflows.

It tells you which entries are duplicate followers under the keep policy you selected.

## Parameters

**keep_first**

Whether the first observed occurrence in each duplicate group should be treated as the surviving reference entry.

With the default keep-first route, the first appearance of a value stays unmarked and later repeats are marked duplicate.

**keep_last**

Whether the last observed occurrence in each duplicate group should be treated as the surviving reference entry instead.

With this route, earlier repeats are marked duplicate and the final appearance is left unmarked.

## Returns

`Thread`
    Boolean-like thread with one value per entry indicating duplicate membership.

## Notes

The returned mask has the same length as the source thread, so it can be inspected directly or reused in later boolean-style filtering logic.

`duplicated(...)` reports duplicate membership only. It never removes entries by itself.

## Examples

Example 1. Mark duplicates while keeping the first occurrence as the survivor.

```cpp
using namespace pluss::table;

auto out = Thread(pluss::make_note({"Accra", "Tema", "Accra", "Kumasi", "Tema"}))
               .duplicated();
```

### Output

```text
0  false
1  false
2  true
3  false
4  true
dtype: Series
```

Example 2. Switch to a keep-last duplicate policy.

```cpp
using namespace pluss::table;

auto out = Thread(pluss::make_note({"Accra", "Tema", "Accra", "Kumasi", "Tema"}))
               .duplicated(false, true);
```

### Output

```text
0  true
1  true
2  false
3  false
4  false
dtype: Series
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dtype.html">
    <span class="label">Previous</span>
    <span class="title">dtype</span>
  </a>
  <a class="next-link" href="encode.html">
    <span class="label">Next</span>
    <span class="title">encode</span>
  </a>
</div>

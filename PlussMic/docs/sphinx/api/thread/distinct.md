# Thread::distinct

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::distinct(bool dropnull = true) -> Thread
```

Return distinct values in stable first-seen order.

Use this when you want the unique observed values themselves, not just the count.

Unlike a sorting operation, `distinct(...)` preserves the encounter order of the first surviving representatives.

## Parameters

**dropnull**

Whether missing entries should be removed before distinctness is computed.

- `true`: ignore null entries,
- `false`: keep at most one null entry in the output.

This flag matters when missingness itself is meaningful and you want a single null representative preserved among the distinct values.

## Returns

`Thread`
    Thread containing one representative of each distinct value, ordered by first appearance.

## Notes

`distinct(...)` is value-returning, not count-returning. Use `ndistinct(...)` when you only need the number of distinct values.

## Examples

Example 1. Collect distinct note values in first-seen order.

```cpp
using namespace pluss::table;

auto out = Thread(pluss::make_note({"Accra", "Tema", "Accra", "Kumasi"})).distinct(true);
```

### Output

```text
0  Accra
1  Tema
2  Kumasi
dtype: Note
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="display_name.html">
    <span class="label">Previous</span>
    <span class="title">display_name</span>
  </a>
  <a class="next-link" href="dropnull.html">
    <span class="label">Next</span>
    <span class="title">dropnull</span>
  </a>
</div>

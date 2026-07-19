# Thread::repeated

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::repeated() -> Thread
```

Build a boolean mask that marks every entry whose value appears more than once.

`repeated()` differs from `duplicated()`: `duplicated()` marks duplicate occurrences according to a keep policy, while `repeated()` marks the whole repeated group.

## Parameters

`None`

## Returns

`Thread`
    Boolean-like thread with one value per entry indicating whether that value belongs to a repeated group.

## Notes

For values `[A, B, A, C, B]`, `repeated()` returns `[true, true, true, false, true]`.

Use `duplicated(...)` when you want pandas-style duplicate follower semantics such as `[false, false, true, false, true]`.

## Examples

Example 1. Mark every member of repeated value groups.

```cpp
using namespace pluss::table;

auto out = Thread(pluss::make_note({"Accra", "Tema", "Accra", "Kumasi", "Tema"}))
               .repeated();
```

### Output

```text
0  true
1  true
2  true
3  false
4  true
ptype: Series
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rdiv.html">
    <span class="label">Previous</span>
    <span class="title">rdiv</span>
  </a>
  <a class="next-link" href="replace.html">
    <span class="label">Next</span>
    <span class="title">replace</span>
  </a>
</div>

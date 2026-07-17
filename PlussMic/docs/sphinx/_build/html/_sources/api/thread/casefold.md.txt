# Thread::casefold

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::casefold() -> Thread
```

Apply full case folding to every note-backed text value in the thread.

Use this when you want normalization for case-insensitive comparison, matching, or grouping.

## Parameters

`None`

## Returns

`Thread`
    Note-backed thread with folded text values.

## Notes

`casefold()` is stronger than simple lowercase conversion. It is the text-normalization route to prefer before equality checks or membership checks that should ignore case.

## Examples

Example 1. Normalize text values for case-insensitive downstream work.

```cpp
using namespace pluss::table;

auto names = Thread(pluss::make_note({"Ada", "BOLA", "cHeN"}));

auto out = names.casefold();
```

### Output

```text
0  ada
1  bola
2  chen
dtype: Note
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bucketize.html">
    <span class="label">Previous</span>
    <span class="title">bucketize</span>
  </a>
  <a class="next-link" href="cast.html">
    <span class="label">Next</span>
    <span class="title">cast</span>
  </a>
</div>

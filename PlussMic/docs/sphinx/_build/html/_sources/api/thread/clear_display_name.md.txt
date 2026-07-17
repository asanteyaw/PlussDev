# Thread::clear_display_name

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::clear_display_name() -> Thread
```

Remove the display name from the current thread without changing its values.

## Parameters

`None`

## Returns

`Thread`
    Thread with its display name cleared.

## Examples

Example 1. Clear a display name while preserving the payload.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0}));
auto named = scores.with_display_name("scores");

auto out = named.clear_display_name();
```

### Output

```text
0  10
1  20
2  30
3  40
dtype: Series
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="chars.html">
    <span class="label">Previous</span>
    <span class="title">chars</span>
  </a>
  <a class="next-link" href="clip.html">
    <span class="label">Next</span>
    <span class="title">clip</span>
  </a>
</div>

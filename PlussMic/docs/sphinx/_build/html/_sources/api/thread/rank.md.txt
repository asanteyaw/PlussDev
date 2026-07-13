# Thread::rank

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::rank(RankTieRule ties = RankTieRule::Average, RankNullRule nulls = RankNullRule::Keep, bool ascending = true, bool percentage = false) -> Thread
```

## What It Does

Assigns numeric ranks to thread values while controlling ties and null handling.

## Parameters

**ties**
    Tie policy for equal values: `Average`, `Lowest`, `Highest`, `First`, or `Dense`.

**nulls**
    Null ranking policy: `Keep` leaves null ranks missing, `Top` ranks them first, and `Bottom` ranks them last.

**ascending**
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

**percentage**
    When `true`, divide each rank by the number of ranked observations.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `rank` to the current thread using explicit `ties, nulls, ascending, percentage` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.rank({}, {}, true, true);
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="put.html">
    <span class="label">Previous</span>
    <span class="title">put</span>
  </a>
  <a class="next-link" href="replace.html">
    <span class="label">Next</span>
    <span class="title">replace</span>
  </a>
</div>

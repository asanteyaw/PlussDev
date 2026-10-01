<!-- pluss-hand-authored -->

# TimeIndex Factory API Reference

[Back to PlussQit API Reference](../api_reference.md)

This reference teaches the free functions in `pluss::timeindex`. These
functions create and render the one-dimensional temporal axis consumed by
search, slicing, rolling, resampling, and alignment operations. Instance
behavior remains in the
[Timeline Index Accessor API Reference](timeline_index_accessor_api_reference.md).

## Choose By Source

- Use [`make`](api/timeindex_factory/make.md) for timestamp text, exact epoch
  nanoseconds, or an existing one-dimensional `Timeline`.
  `pluss::make_timeindex(...)` is its intentional root-level forwarding
  spelling and is taught on the same page because its contract is identical.
- Use [`date_range`](api/timeindex_factory/date_range.md) for a regular,
  endpoint-inclusive temporal axis.
- Use [`from_buffer`](api/timeindex_factory/from_buffer.md) when an external
  owner supplies signed 64-bit epoch-nanosecond storage and an ownership
  callback.
- Use [`print`](api/timeindex_factory/print.md) for structural diagnostic text;
  use TimeIndex stream insertion when default formatting is sufficient.

Construction checks for `NaT`, rank, and non-decreasing order by default.
`require_sorted=false` is an explicit low-level escape for workflows that must
first inspect or reorder an axis; ordered lookup, rolling, and resampling still
require an increasing axis.

Every displayed result in this family is emitted by
`examples/CodeH/timeindex_factory_reference_demo.cpp`. Its checked-in transcript
is `docs/sphinx/_verified/timeindex_factory_reference_output.txt`.

```{toctree}
:hidden:
:maxdepth: 1

api/timeindex_factory/date_range
api/timeindex_factory/from_buffer
api/timeindex_factory/make
api/timeindex_factory/print
```

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../api_reference.html"><span class="label">Previous</span><span class="title">PlussQit API Reference</span></a>
  <a class="next-link" href="api/timeindex_factory/date_range.html"><span class="label">Next</span><span class="title">date_range</span></a>
</div>

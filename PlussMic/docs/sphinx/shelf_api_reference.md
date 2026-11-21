# Shelf API Reference

This page documents the full public `Shelf` surface with direct typed-column examples.

## Using This Reference

- Use the left sidebar to open one method page at a time.
- Each method page contains its own signatures, parameter details, notes, verified outputs, and working previous/next navigation.
- This landing page stays intentionally short so the content panel is not one giant scroll of every method.

## Method Pages

The sidebar lists methods alphabetically. You can also use the search box to jump directly to a method name.

## Static functions

`Shelf::select` builds a shelf without needing one to start from, so it is called on the type: `Shelf::select({conditions...}, {choices...}, default)`.
It has its own page, [`select`](api/shelf/select.md), in the method list below.

## Accessors

`Shelf` exposes nested accessors, each with its own method pages and sidebar:

- `cast()` -> [`cast`](shelf_cast_accessor_api_reference.md)
- `group()` -> [`group`](shelf_group_accessor_api_reference.md)
- `label()` -> [`label`](shelf_label_accessor_api_reference.md)
- `nt()` -> [`nt`](shelf_note_accessor_api_reference.md)
- `tl()` -> [`tl`](shelf_timeline_accessor_api_reference.md)
- `sort()` -> [`sort`](shelf_sort_accessor_api_reference.md)
- `ts()` -> [`ts`](shelf_ts_accessor_api_reference.md)
- `plot()` -> [`plot`](shelf_plot_api_reference.md)

```{toctree}
:hidden:
:maxdepth: 1

shelf_cast_accessor_api_reference
shelf_group_accessor_api_reference
shelf_label_accessor_api_reference
shelf_note_accessor_api_reference
shelf_sort_accessor_api_reference
shelf_timeline_accessor_api_reference
shelf_ts_accessor_api_reference
shelf_plot_api_reference
api/shelf/abs
api/shelf/add
api/shelf/align
api/shelf/all
api/shelf/any
api/shelf/append
api/shelf/arrange
api/shelf/as_duration
api/shelf/as_note
api/shelf/as_series
api/shelf/as_timeline
api/shelf/astype
api/shelf/between
api/shelf/bfill
api/shelf/bottom
api/shelf/bounded
api/shelf/bucketize
api/shelf/casefold
api/shelf/cast
api/shelf/chars
api/shelf/clip
api/shelf/compare
api/shelf/corr
api/shelf/corr_with
api/shelf/count
api/shelf/covar
api/shelf/csmax
api/shelf/csmin
api/shelf/cummax
api/shelf/cummin
api/shelf/cumprod
api/shelf/cumsum
api/shelf/defined
api/shelf/describe
api/shelf/distinct
api/shelf/div
api/shelf/dropinf
api/shelf/dropinvalid
api/shelf/dropnan
api/shelf/dropneginf
api/shelf/dropnull
api/shelf/dropposinf
api/shelf/dtype
api/shelf/duplicated
api/shelf/encode
api/shelf/ends
api/shelf/eq
api/shelf/exp
api/shelf/extract
api/shelf/ffill
api/shelf/fillinf
api/shelf/fillinvalid
api/shelf/fillnan
api/shelf/fillneginf
api/shelf/fillnull
api/shelf/fillnull_
api/shelf/fillposinf
api/shelf/filter
api/shelf/floordiv
api/shelf/flow
api/shelf/fold
api/shelf/framerize
api/shelf/freq
api/shelf/fullmatch
api/shelf/ge
api/shelf/group
api/shelf/gt
api/shelf/has
api/shelf/head
api/shelf/indicate
api/shelf/info
api/shelf/info_string
api/shelf/interp
api/shelf/invalid
api/shelf/isfinite
api/shelf/isinf
api/shelf/isnan
api/shelf/isneginf
api/shelf/isnull
api/shelf/isposinf
api/shelf/kind
api/shelf/kindelize
api/shelf/kurtosis
api/shelf/label
api/shelf/land
api/shelf/le
api/shelf/length
api/shelf/log
api/shelf/lor
api/shelf/lower
api/shelf/lstrip
api/shelf/lt
api/shelf/lxor
api/shelf/map
api/shelf/matches
api/shelf/matmul
api/shelf/max
api/shelf/mean
api/shelf/median
api/shelf/min
api/shelf/mod
api/shelf/mode
api/shelf/mul
api/shelf/name
api/shelf/named
api/shelf/ncases
api/shelf/ndistinct
api/shelf/ne
api/shelf/nnan
api/shelf/nneginf
api/shelf/nnull
api/shelf/normalize
api/shelf/notinf
api/shelf/notnan
api/shelf/notnull
api/shelf/nposinf
api/shelf/nt
api/shelf/pad
api/shelf/partition
api/shelf/percentiles
api/shelf/plot
api/shelf/pow
api/shelf/prod
api/shelf/ptype
api/shelf/put
api/shelf/radd
api/shelf/rand
api/shelf/rank
api/shelf/rdiv
api/shelf/repeated
api/shelf/replace
api/shelf/rfloordiv
api/shelf/rmod
api/shelf/rmul
api/shelf/ror
api/shelf/round
api/shelf/rpow
api/shelf/rstrip
api/shelf/rsub
api/shelf/rtruediv
api/shelf/rxor
api/shelf/sample
api/shelf/scan
api/shelf/scribbelize
api/shelf/se
api/shelf/select
api/shelf/shape
api/shelf/shift
api/shelf/shuffle
api/shelf/size
api/shelf/skew
api/shelf/slice
api/shelf/sort
api/shelf/split
api/shelf/starts
api/shelf/std
api/shelf/strip
api/shelf/sub
api/shelf/sum
api/shelf/tail
api/shelf/take
api/shelf/titlecase
api/shelf/tl
api/shelf/to_string
api/shelf/top
api/shelf/transform
api/shelf/truediv
api/shelf/ts
api/shelf/unname
api/shelf/upper
api/shelf/value
api/shelf/var
api/shelf/view
api/shelf/weigh
api/shelf/window
```

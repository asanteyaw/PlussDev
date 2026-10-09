Use the table model as the maturity reference, but do not blindly copy every method into Kindle/Bundle. The right rule is:

- same concept → same Pluss-native name;
- same dimensional role → similar behavior;
- different model semantics → Kindle/Bundle-specific API;
- no alias shims just to look table-like.

That keeps the ecosystem coherent without turning `Stream/Catalog/Archive` and `Slot/Registry/Axel` into awkward DataFrame clones.

Recommended mapping

| Table | Kindle | Bundle | Role |
|---|---|---|---|
| `Slate` | `Stream` | `Slot` | 1D-ish payload container |
| `DataFrame` | `Catalog` | `Registry` | named collection / 2D-like object |
| `Atlas` | `Archive` | `Axel` | stack/depth collection |

But the mapping is conceptual, not exact. `Slate` is a strict 1D table feature. `Stream` may be more document/record oriented depending on Kindle semantics. `Slot` is heterogeneous, so it should not pretend to have every numeric/text/timeline method unless delegation is type-safe.

What I would standardize across all models

These should be consistent wherever they make sense:

```cpp
defined()
empty()
shape()
size()
ncases()
nfeatures() / nstreams() / nslots()
head()
tail()
slice()
take()
sample()
label()
dtype() / ptype()
info()
info_string()
to_string()
show()
copy()
```

Missing/null semantics:

```cpp
isnull()
notnull()
invalid()
fillnull()
dropnull()
ffill()
bfill()
```

Collection/selection semantics:

```cpp
features()   // table only
streams()    // kindle
slots()      // bundle

get()
has()
set()
drop()
rename()
reorder()
```

Arithmetic/statistical operations only where payload-safe:

```cpp
sum()
mean()
min()
max()
median()
mode()
std()
var()
count()
ndistinct()
freq()
rank()
```

For `Stream`, these can delegate to underlying `Series/Note/Timeline` similarly to `Slate`.

For `Catalog`, batch/delegate to `Stream`.

For `Archive`, batch/delegate to `Catalog`.

For `Slot`, only expose these if the slot payload supports it. Otherwise throw cleanly with `pluss::Error`.

What should not be copied blindly

Do not force these everywhere:

```cpp
corr()
pivot()
unstack()
group_by()
join()
sql()
plot()
ts()
tl()
note()
case_study()
```

Some belong naturally to table. Kindle/Bundle may need analogues, but not always the same API.

For example:

- `Catalog::join()` may make sense if catalogs are keyed records.
- `Archive::align_depth()` makes sense.
- `Registry::join()` may be weird unless registry slots have compatible keys.
- `Slot::corr()` likely makes no sense unless it is numeric matrix-like.
- `Stream::note()` makes sense if it can hold Note payloads.
- `Stream::tl()` makes sense if it can hold Timeline payloads.
- `Slot::note()` is questionable unless Slot has strong payload family detection.

Kindle-specific methods should exist

Kindle likely needs methods that are not table methods, especially if it models document/collection/archive concepts. Examples:

```cpp
catalog.keys()
catalog.schema()
catalog.catalogue() // though avoid this if “catalog” already owns the noun
catalog.index()     // avoid if “index” is banned globally
catalog.entries()
catalog.lookup()
catalog.merge()
catalog.attach()
catalog.detach()
catalog.flatten()
catalog.pack()
catalog.unpack()
catalog.kindelize()
catalog.tabelize()
```

But naming should follow the same discipline:

- one word preferred;
- two words only when unavoidable;
- no three-word public methods;
- no duplicate aliases.

My implementation strategy would be phased

Phase 1: inventory and policy

- Diff public methods:
  - `Slate` vs `Stream` vs `Slot`
  - `DataFrame` vs `Catalog` vs `Registry`
  - `Atlas` vs `Archive` vs `Axel`
- Classify each table method:
  - direct equivalent
  - semantic equivalent with different name
  - not applicable
  - Kindle/Bundle-specific replacement needed

Phase 2: core common API

Bring all models to consistent basics:

```cpp
defined
empty
shape
size
head
tail
slice
take
label
ptype
dtype where meaningful
info
show
to_string
get
has
set
drop
rename
copy
```

Phase 3: payload-family delegation

For `Stream` first:

```cpp
stream.note()
stream.tl()
stream.ts()
stream.cast()
stream.isnull()
stream.fillnull()
stream.dropnull()
stream.freq()
stream.distinct()
stream.ndistinct()
```

Then `Catalog` delegates/batches to `Stream`.

Then `Archive` delegates/batches to `Catalog`.

Phase 4: alignment and labels

Adopt the same hybrid alignment policy where meaningful:

```cpp
AlignBy::Pos
AlignBy::Label
fill
strict default
```

But in Kindle, labels may be stream keys, document IDs, archive depth labels, etc. Do not call them `cases` unless the model truly has cases.

Phase 5: Bundle separately

Bundle is probably more heterogeneous than Kindle, so it needs stricter payload guards. `Slot` should not get bloated with unsafe methods. It should expose generic methods plus payload-specific surfaces only when the payload is compatible.

Bottom line

Yes, use the same method names where the semantics match. No, do not copy the table API wholesale.

The table model should provide the doctrine and naming spine. Kindle/Bundle should adopt the same maturity standards—delegation hierarchy, selected-data-first surfaces, label/alignment consistency, null semantics, vectorized kernels—but their public methods should reflect their own data model.

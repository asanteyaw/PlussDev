Below is your report copied and pasted verbatim. There are two tracks, you can pick which track and more easily acheivable or better to pursue first. Just note one thing and strictly adhere to it: 
1. All methods, surfaces and accessor methods that has a 1D feasible implementation SHOULD BE on Thread, then DataFrame only cordinates/batches/delegates smartly to the Thread counterpart in a vectorized manner for peak performance. I will not stand another violation because then i'll have to waste more resources to fix lol!
2. All methods should LEAVE feature selection to .features(), only the methods that has feature selection
as intrinsic to it can violate this policy(i think you mentioned a few like .drop(), etc). Otherwise the ones that can do without having selection as an arg, should NOT have feature selection as part of their parameters/arguments.



Surface diff report, based on public headers first.

**Track 1**
`DataFrame` and `Thread` are not aligned yet. The biggest drift is not the accessor names themselves, it is the accessor payload.

Public accessor entrypoints:
- Shared on both: `label()`, `group()`, `cast()`, `ts()`, `tl()`, `nt()` in [DataFrame.h](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/DataFrame.h:757) and [Thread.h](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/Thread.h:761).
- `DataFrame`-only: `plot()`, `plot3d()`, `sql()`, `prep_sql()` in [DataFrame.h](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/DataFrame.h:860).
- `feataxis()` is `DataFrame`-only by design, and I would not count that as a thread-first violation because `Thread` has no feature axis.

Accessor families that are mostly fine:
- `cast()` is aligned: [DataFrameCastAccessor](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/DataFrame.h:1711) vs [ThreadCastAccessor](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/Thread.h:1121).
- `nt()` is largely aligned: [DataFrameNoteAccessor](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/DataFrame.h:1816) vs [ThreadNoteAccessor](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/Thread.h:1184).
- `tl()` is largely aligned: [DataFrameTimelineAccessor](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/DataFrame.h:1872) vs [ThreadTimelineAccessor](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/Thread.h:1239).

Accessor families with real drift:
- `ts()` is the largest mismatch. `DataFrameTsAccessor` in [DataFrame.h](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/DataFrame.h:1922) exposes public surfaces that `TimeSeriesAccessor` in [Thread.h](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/Thread.h:951) does not.
- Missing on thread-side `ts()`: `window(duration, closure)`, `rolling(specs)`, `window(specs)`, `sort()`, `calendar()`, `calendar(time_feature)`, and the richer resample-builder surface.
- `DataFrameTsResampler` in [DataFrame.h](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/DataFrame.h:2072) has `mean/sum/min/max/first/last/count/asfreq/ffill/bfill/nearest`. Thread-side only returns `TimeSeriesResample { labels, values }` in [Thread.h](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/Thread.h:916), so the thread surface stops too early.
- `label()` is also materially richer on `DataFrame`. Compare [DataFrameLabelAccessor](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/DataFrame.h:1606) vs [ThreadLabelAccessor](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/Thread.h:1059).
- Missing on thread-side `label()`: `distinct`, `freq`, `duplicated`, `repeated`, `cast`, `map`, `take`, numeric slice, `monotonic`, plus the hierarchical label management pieces like `names`, `part`, `depth`, `codes`, `levels`, `rename`, `order`, `drop`.
- `group()` is close but still asymmetric. `DataFrameGrouped` in [DataFrame.h](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/DataFrame.h:1740) has spec-based `transform(...)`; `ThreadGrouped` in [Thread.h](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/Thread.h:1148) only has `transform(GroupOp)`.

My read:
- `nt()` and `tl()` are not the problem.
- `ts()` and `label()` are the main thread-first gaps.
- `plot/sql` are policy decisions: if single-thread plotting and SQL are wanted, thread has no first-class entry surface yet.

**Track 2**
Clear public selector-policy violations in `DataFrame` where selection is still embedded in the method instead of going through `.features(...)`:

Methods with an existing thread primitive behind them:
- `astype(features, dtype)` in [DataFrame.h](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/DataFrame.h:796). This should be `df.features(...).astype(dtype)`. Thread already has `astype(dtype)` in [Thread.h](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/Thread.h:766).
- `bucketize(feature_name/feature_names, ...)` in [DataFrame.h](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/DataFrame.h:817). This should be `df.features(...).bucketize(...)`. Thread already has `bucketize(...)` in [Thread.h](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/Thread.h:427).
- `shift(features, periods)` in [DataFrame.h](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/DataFrame.h:925). This should be `df.features(...).shift(periods)`. Thread already has `shift(...)` in [Thread.h](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/Thread.h:414).
- `map(fn, features={})` in [DataFrame.h](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/DataFrame.h:937). This should be `df.features(...).map(fn)`. Thread already has `map(...)` in [Thread.h](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/Thread.h:484).
- `transform(feature, fn)` overloads in [DataFrame.h](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/DataFrame.h:953). These should be selector-chain forms. Thread already has `transform(...)` in [Thread.h](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/Thread.h:485).
- `put(feature, mask, value)` overload family in [DataFrame.h](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/DataFrame.h:956). This should become `df.set(name) = df.features(name).put(mask, value)` style. Thread already has `put(...)` in [Thread.h](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/Thread.h:531).
- `any(features={})` and `all(features={})` in [DataFrame.h](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/DataFrame.h:1186). These should be `df.features(...).any()` and `df.features(...).all()`. Thread already has `any/all` in [Thread.h](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/Thread.h:439).
- `ndistinct(features={})` in [DataFrame.h](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/DataFrame.h:1196). This is the exact kind of violation you called out. Thread already has `ndistinct()` in [Thread.h](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/Thread.h:473).
- `duplicated(features={})` in [DataFrame.h](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/DataFrame.h:1200). Thread already has `duplicated()` in [Thread.h](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/Thread.h:474).
- `repeated(features={})` in [DataFrame.h](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/DataFrame.h:1203). Thread already has `repeated()` in [Thread.h](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/Thread.h:475).
- `drop_duplicates(features={})` in [DataFrame.h](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/DataFrame.h:1204). Same selector smell; it should key off a selected subframe rather than take names directly.
- `corr(features, ...)` overloads in [DataFrame.h](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/DataFrame.h:1209). Even though `corr` is frame-level, the selection should still happen outside via `.features(...)`.

Convenience methods that also violate the direction:
- `count_by(feature, ...)` in [DataFrame.h](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/DataFrame.h:1041).
- `mean_by(group_feature, value_feature, ...)` in [DataFrame.h](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/DataFrame.h:1045).

I would not count these as selector violations, because the feature names are intrinsic to table orchestration rather than column selection:
- `drop(...)`, `rename(...)`, `reorder(...)`, `place(...)`
- `arrange(...)`, `top(...)`, `bottom(...)`
- `pivot(...)`, `pivot_table(...)`
- `unify(...)`, `join(...)`
- `group().by(...)`

Private compatibility debt still present, but not public:
- `match_features`, `series_features`, `note_features`, `timeline_features`, `set_features`, old internal `group(...)`, and old internal `cast(...)` helpers are all in the private section around [DataFrame.h](/Users/yawasante/software/cxx/PlussDev/PlussMic/src/table/DataFrame.h:1494).

Patch order I’d use:
1. Fix `ts()` parity first.
2. Fix `label()` parity second.
3. Remove public selector violations that already have direct thread analogues: `ndistinct`, `duplicated`, `repeated`, `any`, `all`, `shift(features)`, `astype(features)`, `bucketize(feature...)`, `map(features)`, `transform(feature...)`, `put(feature...)`.
4. Kill convenience selectors like `count_by` and `mean_by` after the core chain is strong enough.

If you want, I can turn this report into a patch ledger next: exact public signatures to remove, exact thread surfaces to add, and the migration order that minimizes breakage.
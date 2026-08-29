# `lymph::models::Builder` Notes

## Registry Notes

`Builder` is not a generic map. It is a candidate-model workspace. That is why
names matter so much on this surface: they become the stable identity used by
reports and comparison results.

## Benchmark Memory Notes

The central semantic point is non-destructive benchmark memory:

- the registry keeps the originally stored candidates
- `benchmark()` stores comparison outcomes separately
- `best()` and `worst()` return fitted remembered artifacts
- `get(name)` still lets you inspect the original registered entry

## Inspection Notes

Use the inspection surfaces by cost:

- `has()` for presence
- `names()` for full identity
- `list()` for quick preview
- `report()` for tabular inspection

## Related Pages

- [Introduction](Introduction.md)
- [benchmark](benchmark.md)
- [report](report.md)

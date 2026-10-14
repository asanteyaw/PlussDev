# Core Utilities Manual

[Back to PlussTils](../index.md) · [Core Utilities API Reference](../api/core.md)

Core utilities are small, independent helpers. Each one answers a question that
comes up in ordinary programs, and none of them asks you to adopt a data model.
This manual groups the utilities by the decision you are making, and each
chapter covers one of them with complete programs.

## Choosing a utility

- **I need to print a value, a report, or a log line.** Start with
  [Writing output](tils-output.md), then pick the options in
  [Print options](tils-print-options.md).
- **I need a setting from the environment.** Read [Reading the environment](tils-env.md).
- **I need the same operation applied to many values.** Use
  [Mapping with transmit](tils-aux-transmit.md), or
  [Mapping along an axis with translate](tils-aux-translate.md) when the values
  form a table.
- **I need to convert between a host number and a Series.** Read
  [Scalar casts](tils-aux-scalar-casts.md).
- **I need a path under a project root, or a remote address.** Read
  [Paths and roots](tils-aux-paths.md).
- **I need to wrap a value and chain steps on it, or fix a binding.** Read
  [Values in pipelines: suppose and let](tils-suppose-let.md).
- **I need repeatable random numbers.** Read [Deterministic seeds](core_seeds.md).
- **I need to read a dataset in batches.** Read [Loaders](loaders.md), which
  covers the data loader and its samplers.

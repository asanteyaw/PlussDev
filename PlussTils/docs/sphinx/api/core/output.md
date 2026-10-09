# Output: OutputEngine and its options

[Back to Core Utilities](../core.md) · [Core Utilities API Reference](../../api_reference.md)

The output engine writes values, formatted lines, and level-tagged log lines to a
stream. You configure an engine with an `OutputOptions` value and with
`PrintOptions` defaults, and each call may add its own `PrintOptions`.

- [`OutputEngine`](../core-output-engine.md): construction, the sink, and every
  member that writes, configures, or logs.
- [`OutputLevel`](../core-output-level.md): the ranked levels that filter log lines.
- [`OutputOptions`](../core-output-options.md): the bulk configuration of an engine.

For a guide with worked programs, see
[Writing output](../../manual/tils-output.md) and
[Print options](../../manual/tils-print-options.md).

```{toctree}
:maxdepth: 1
:hidden:

../core-output-engine
../core-output-engine/to
../core-output-engine/sep
../core-output-engine/prefix
../core-output-engine/enable_levels
../core-output-engine/set_options
../core-output-engine/options
../core-output-engine/print_defaults
../core-output-engine/write
../core-output-engine/writeln
../core-output-engine/writef
../core-output-engine/writelnf
../core-output-engine/endln
../core-output-engine/flush
../core-output-engine/log
../core-output-engine/logf
../core-output-engine/trace
../core-output-engine/debug
../core-output-engine/info
../core-output-engine/warn
../core-output-engine/error
../core-output-engine/tracef
../core-output-engine/debugf
../core-output-engine/infof
../core-output-engine/warnf
../core-output-engine/errorf
../core-output-level
../core-output-options
```

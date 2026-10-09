# `pluss::ai::ice::models::TFTOptions`

Static/encoder/decoder widths and lengths, hidden size, LSTM layers,
attention heads, dropout, forecast quantiles, and categorical
variable slots for
[`TemporalFusionTransformer`](TemporalFusionTransformer.md)'s
constructor -- with `from_data`/`from_dataset` factories that size
every width/length field automatically from a
[`data::DataSet`](../data/DataSet.md)/[`TimeSeriesDataSet`](../data/TimeSeriesDataSet.md),
the same pattern
[`AutoregressiveForecasterOptions`](AutoregressiveForecasterOptions.md)
follows.

## Fields

### `static_size`

`std::int64_t static_size = 0`
: Number of static (per-series, time-invariant) covariates. `0`
  disables the static covariate encoder entirely -- no static context
  is injected into variable selection, enrichment, or the LSTM's
  initial state.

### `encoder_size`

`std::int64_t encoder_size = 0`
: Number of time-varying features supplied at every past (lookback)
  timestep.

### `decoder_size`

`std::int64_t decoder_size = 0`
: Number of time-varying features known ahead of time for the
  forecast horizon (calendar features, planned events, etc.) -- must
  be at least `1`.

### `encoder_length`

`std::int64_t encoder_length = 0`
: Lookback window length, in timesteps.

### `decoder_length`

`std::int64_t decoder_length = 0`
: Forecast horizon length, in timesteps.

### `hidden_size`

`std::int64_t hidden_size = 32`
: Hidden width shared throughout variable selection, the gated
  residual networks, and the LSTM encoder/decoder.

### `lstm_layers`

`std::int64_t lstm_layers = 1`
: Number of stacked LSTM layers. One shared static-derived context is
  broadcast across every layer, rather than a distinct per-layer
  projection -- a deliberate scope cut from the original architecture.

### `attention_heads`

`std::int64_t attention_heads = 4`
: Number of heads in the causal self-attention layer over the full
  (encoder+decoder) sequence.

### `dropout`

`double dropout = 0.1`
: Dropout strength throughout the gated residual networks and
  attention. Set to `0.0` for small/synthetic data or when debugging a
  training run.

### `quantiles`

`std::vector<double> quantiles = {0.1, 0.5, 0.9}`
: Forecast quantiles -- order here is the order
  [`TFTForecast::quantiles`](TFTForecast.md)' last axis comes back in.

### `static_categoricals`

`std::vector<CategoricalVariable> static_categoricals = {}`
: Marks which 0-based slots within `static_inputs`' own `static_size`
  width hold an integer category code rather than a continuous value,
  and each one's cardinality -- same
  [`CategoricalVariable`](../data/CategoricalVariable.md) type
  `data::TimeSeriesDataSet` uses for its own bookkeeping. A slot
  listed here is routed through a real per-category embedding lookup
  rather than being treated as an ordinal/continuous number. Any slot
  not listed is treated as continuous.

### `encoder_categoricals`

`std::vector<CategoricalVariable> encoder_categoricals = {}`
: The same marking, for slots within `encoder_inputs`.

### `decoder_categoricals`

`std::vector<CategoricalVariable> decoder_categoricals = {}`
: The same marking, for slots within `decoder_inputs`.

### `cfg`

`Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU)`
: Dtype and platform the model's own parameters are constructed on.

### `seed`

`std::uint64_t seed = 0`
: Seeds the model's own parameter initialization -- independent of
  [`TFTTrainOptions::seed`](TFTTrainOptions.md), which seeds
  shuffling/the train-validation split instead.

## Guidance

Build this by hand only when `static_size`/`encoder_size`/
`decoder_size`/`encoder_length`/`decoder_length` are already known
exactly; otherwise prefer `from_data(dataset)` (any
[`data::DataSet`](../data/DataSet.md)) or
`from_dataset(time_series_dataset)` (a
[`data::TimeSeriesDataSet`](../data/TimeSeriesDataSet.md), forwarding
to `from_data(dataset.data())`), which read those three width fields
plus `static_categoricals`/`encoder_categoricals`/`decoder_categoricals`
straight off the dataset's own materialized batch. Either factory
leaves `encoder_length`/`decoder_length`/`hidden_size`/`lstm_layers`/
`attention_heads`/`dropout`/`quantiles`/`cfg`/`seed` at their
defaults -- set those explicitly afterward, same as building
`TFTOptions` by hand.

## See Also

- [`TemporalFusionTransformer`](TemporalFusionTransformer.md) -- the
  constructor this configures.
- [`data::DataSet`](../data/DataSet.md) /
  [`data::TimeSeriesDataSet`](../data/TimeSeriesDataSet.md) -- what
  `from_data`/`from_dataset` read.
- [`CategoricalVariable`](../data/CategoricalVariable.md) -- the
  `*_categoricals` element type.
- [`AutoregressiveForecasterOptions`](AutoregressiveForecasterOptions.md)
  -- the same shape and `from_data`/`from_dataset` pattern, for
  `AutoregressiveForecaster`.

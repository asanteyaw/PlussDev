# `pluss::ai::ice::models::AutoregressiveForecasterOptions`

Static/encoder/decoder widths and lengths, hidden size, RNN layer
count, dropout, and categorical variable slots for
[`AutoregressiveForecaster`](AutoregressiveForecaster.md)'s
constructor -- with `from_data`/`from_dataset` factories that size
every width/length field automatically from a
[`data::DataSet`](../data/DataSet.md)/[`TimeSeriesDataSet`](../data/TimeSeriesDataSet.md),
the same way [`TFTOptions`](TFTOptions.md) does for
[`TemporalFusionTransformer`](TemporalFusionTransformer.md).

## Fields

### `static_size`

`std::int64_t static_size = 0`
: Number of static (per-series, time-invariant) covariates. `0`
  disables the static encoder entirely -- no static context is
  injected into the encoder/decoder RNN's initial state, and
  `AutoregressiveBatch::static_inputs` is expected to stay undefined.

### `encoder_size`

`std::int64_t encoder_size = 0`
: Number of time-varying features supplied at every lookback
  (encoder) timestep.

### `decoder_size`

`std::int64_t decoder_size = 0`
: Number of time-varying features known ahead of time for the
  forecast horizon (decoder) -- calendar features, planned events,
  and the like.

### `encoder_length`

`std::int64_t encoder_length = 0`
: Lookback window length, in timesteps.

### `decoder_length`

`std::int64_t decoder_length = 0`
: Forecast horizon length, in timesteps -- also the trailing
  dimension of [`GaussianForecast::mean`/`scale`](GaussianForecast.md).

### `hidden_size`

`std::int64_t hidden_size = 32`
: The RNN's hidden width, shared by the encoder and decoder.

### `rnn_layers`

`std::int64_t rnn_layers = 2`
: Number of stacked RNN layers in the encoder/decoder.

### `dropout`

`double dropout = 0.1`
: Dropout strength inside the RNN stack. Set to `0.0` for small/
  synthetic data or when debugging a training run, where dropout's
  own randomness would otherwise mask whether a change actually
  helped.

### `static_categoricals`

`std::vector<CategoricalVariable> static_categoricals = {}`
: Marks which 0-based slots within `static_inputs`' own `static_size`
  width hold an integer category code rather than a continuous value,
  and each one's cardinality -- same
  [`CategoricalVariable`](../data/CategoricalVariable.md) type
  `data::TimeSeriesDataSet` uses for its own bookkeeping. Any slot not
  listed here is treated as continuous.

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
  [`AutoregressiveTrainOptions::seed`](AutoregressiveTrainOptions.md),
  which seeds shuffling/the train-validation split instead.

## Guidance

Build this by hand only when `static_size`/`encoder_size`/
`decoder_size`/`encoder_length`/`decoder_length` are already known
exactly; otherwise prefer `from_data(dataset)` (any
[`data::DataSet`](../data/DataSet.md)) or
`from_dataset(time_series_dataset)` (a
[`data::TimeSeriesDataSet`](../data/TimeSeriesDataSet.md), forwarding
to `from_data(dataset.data())`), which read those five fields plus
`static_categoricals`/`encoder_categoricals`/`decoder_categoricals`
straight off the dataset's own materialized batch -- the same pattern
[`TFTOptions::from_data`/`from_dataset`](TFTOptions.md) follows.
Either factory leaves `hidden_size`/`rnn_layers`/`dropout`/`cfg`/`seed`
at their defaults; set those explicitly afterward the same as building
`AutoregressiveForecasterOptions` by hand.

## See Also

- [`AutoregressiveForecaster`](AutoregressiveForecaster.md) -- the
  constructor this configures.
- [`data::DataSet`](../data/DataSet.md) /
  [`data::TimeSeriesDataSet`](../data/TimeSeriesDataSet.md) -- what
  `from_data`/`from_dataset` read.
- [`CategoricalVariable`](../data/CategoricalVariable.md) -- the
  `*_categoricals` element type.
- [`TFTOptions`](TFTOptions.md) -- the same shape and `from_data`/
  `from_dataset` pattern, for `TemporalFusionTransformer`.

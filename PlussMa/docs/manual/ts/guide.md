<!-- generated from guide.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Time series guide

This guide covers the time-series framework of PlussMa, `pluss::subsp::stats::ts`, from describing a series to forecasting it. Each chapter is a short series of pages; each page explains one topic in prose and then works through complete programs whose printed output is the real output of the program. Most of the programs generate their own data from a model with known parameters, so you can see how closely a method recovers the truth, and several repeat an experiment many times to show how reliable a method is.

The chapters build on each other in this order, and each page ends with a pointer to the next:

- **Describing a series** (chapter 1) is where every analysis starts: transforms, autocorrelation, and the tests for noise, unit roots, stationarity and volatility clustering.
- **ARIMA models** (chapter 2) fit, forecast and check models for the mean of a single series.
- **Volatility models** (chapter 3) model its variance, and **vector models** (chapter 4) model several series together, with their long-run relations.
- **State-space models** (chapter 5) describe a series by a hidden state, with missing data and simulation.
- **Direct forecasting** (chapter 6) forecasts each horizon separately and compares models out of sample.
- **Plots** (chapter 7) show the correlation and residual pictures used throughout.

The API reference documents every function and type these chapters use.

Chapters, in reading order:

- [Time Series 1: Describing a Series](guide/ts_01_describing_a_series.md)
- [Time Series 2: ARIMA Models](guide/ts_02_arima_models.md)
- [Time Series 3: Volatility Models](guide/ts_03_volatility_models.md)
- [Time Series 4: Vector Autoregressions and Cointegration](guide/ts_04_vector_models.md)
- [Time Series 5: State-Space Models](guide/ts_05_state_space_models.md)
- [Time Series 6: Direct Forecasting and Model Comparison](guide/ts_06_direct_forecasting.md)
- [Time Series 7: Plots](guide/ts_07_plots.md)

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

guide/ts_01_describing_a_series
guide/ts_02_arima_models
guide/ts_03_volatility_models
guide/ts_04_vector_models
guide/ts_05_state_space_models
guide/ts_06_direct_forecasting
guide/ts_07_plots
```
<!-- apiref:toctree:end -->

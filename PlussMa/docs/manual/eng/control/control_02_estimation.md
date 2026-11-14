<!-- generated from control_02_estimation.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Control 2: Estimation

Feedback needs the state, and often only some outputs can be measured. This chapter reconstructs the states of a system from noisy outputs: the steady-state estimator, which is the dual of the regulator, the Kalman filter that adapts its gain to the data and handles missing measurements, and the smoother that uses the whole record.

- [The steady-state estimator](control_02_estimation/steady_state_estimator.md)
- [The Kalman filter](control_02_estimation/kalman_filter.md)
- [Smoothing](control_02_estimation/smoothing.md)

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

control_02_estimation/steady_state_estimator
control_02_estimation/kalman_filter
control_02_estimation/smoothing
```
<!-- apiref:toctree:end -->

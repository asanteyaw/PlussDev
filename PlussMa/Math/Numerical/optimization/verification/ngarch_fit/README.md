# NGARCH family: fmincon vs Optima

Gaussian returns likelihood for NGARCH (5 parameters), component NGARCH (8) and the RNN-driven time-varying
NGARCH (12: one tanh RNN unit with 3 inputs, an absolute-value output layer, plus alpha, phi, lambda, g), ported
from `with_returns.m`: scaled parameters (`10^ceil(log10|p|)`), the same bounds, and 5 shared random starts.

    matlab -batch "addpath('<this dir>'); matlab_fit('<returns.csv>','/tmp/ngfit',5)"   # also writes starts_*.csv
    verify_example.sh optima_fit.cpp optima_fit.exe
    optima_fit.exe MODEL returns.csv starts_MODEL.csv internal-protocol|sqp|working-set|merit [autodiff|dual]

`dual` uses `optim::Fwd<N>` (plain-double forward-mode scalars) with exact gradients; `autodiff` records the recurrence on the
Trace tape (about 900x slower for 6,000 time steps). MATLAB uses its default finite-difference gradients.

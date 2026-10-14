# Chemistry verification against Cantera

Run `compare_cantera.py` with a Python environment that has Cantera (see the docstring). It checks:
NASA-7 coefficients, Arrhenius / three-body / falloff parameters, net production rates, equilibrium
constants, closed adiabatic constant-volume ignition (Cantera `IdealGasReactor`), and the TP / HP / UV
equilibrium states against Cantera's Gibbs minimiser.

Last run 2026-10-13 with Cantera 3.2.0: thermo coefficients exact; 0 of 29 reactions differ; net
production rates 1.7e-14 and Kc 2.1e-14 relative; ignition delays within 0.025 percent; final reactor
states and equilibrium temperatures identical to 1e-6 K; equilibrium compositions to 2e-14 relative.

`h2o2_ref.py`, `eq_ref.py`, `gen_ref.py`, `gen_eq.py` are the independent NumPy implementation (CGS/cal
units, SciPy Radau and fsolve) that generated the C++ test reference headers; `mech.json` is the mechanism
dumped from the C++ fixture.

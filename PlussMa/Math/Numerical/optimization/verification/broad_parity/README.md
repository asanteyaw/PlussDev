# Broad parity suite

68 problems (Hock–Schittkowski style constrained problems, unconstrained classics, and structured families with
bounds, equalities, inequalities, NaN and infinite bounds) defined once in `problems.py`.

    python3 gen.py /tmp/g/broad                       # writes suite_matlab.m and suite_ours.cpp
    matlab -batch "cd('/tmp/g/broad'); suite_matlab('/tmp/g/broad/matlab_out.txt')"
    PlussTils/tools/verify_example.sh /tmp/g/broad/suite_ours.cpp /tmp/g/broad/ours.exe && /tmp/g/broad/ours.exe > ours_out.txt
    python3 score.py matlab_out.txt ours_out.txt -v

`score.py` reports a strict score (against the best reference result) and a matched score (against the
corresponding reference algorithm), per Optima algorithm. Compile the C++ at more than one optimization level:
an earlier QP solver diverged only at `-O3`.

State on 2026-10-14: strict 196/204 (96.1%) and matched 200/204 (98.0%) over interior-point, sqp and active-set.
Open items: interior-point on HS61 (stalls at -142.32, true -143.65), active-set on ROS10 (budget), interior-point on
HS16 and sqp on HS47 (different local minimum from the reference's best run), and the merit method (no counterpart)
on HS16/33/37/47/61/71.

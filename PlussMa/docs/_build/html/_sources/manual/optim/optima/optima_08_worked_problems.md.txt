<!-- generated from optima_08_worked_problems.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Optima 8: Worked Problems

Chapters 1 to 7 each taught one part of the solver. This chapter uses all of them on five complete problems, each the kind you would meet outside a guide: a portfolio, a physical design with a known answer, a design that cannot be built, a maximum-likelihood fit, and a plant with every kind of constraint at once. After it you have a routine for any new problem: formulate, solve, then decide whether to believe the answer before you use it.

Every program is complete and its output is the output of that program.

## The routine

Solving is the short part. The routine that the programs follow, and that you should follow, is:

| Step | What you do | Where it was taught |
|---|---|---|
| 1. Formulate | Name the unknowns, write the objective, put every requirement in the constraint family that fits it. | chapter 1 |
| 2. Start | Choose a starting point where the objective and its gradient are of ordinary size. | chapter 3, and Program 4 below |
| 3. Solve | Call `optima`, with default options unless a chapter gave you a reason. | chapters 3 to 6 |
| 4. Check the flag | `exit_flag` is `1` only when the solver believes it converged. Anything else is a message to read. | chapter 2 |
| 5. Check the violation | `max_constraint_violation` must be at or below your tolerance. | chapter 2 |
| 6. Read the multipliers | A multiplier of zero means a limit does not bind; a positive one is the price of that limit. | chapter 2 |
| 7. Verify independently | Check the answer by something the solver did not use: a closed form, a re-solve, a second start, an identity the optimum must satisfy. | below |

Step 7 is the one that is skipped, and Program 4 shows why it matters.

## Programs

- [A minimum-variance portfolio](optima_08_worked_problems/a_minimum_variance_portfolio.md)
- [A design with a closed-form answer](optima_08_worked_problems/a_design_with_a_closed_form_answer.md)
- [A design that cannot be built](optima_08_worked_problems/a_design_that_cannot_be_built.md)
- [A maximum-likelihood fit, and what a flag cannot tell you](optima_08_worked_problems/a_maximum_likelihood_fit_and_what_a_flag_cannot.md)
- [A plant with every kind of constraint](optima_08_worked_problems/a_plant_with_every_kind_of_constraint.md)

## What goes wrong in practice

| Symptom | Likely cause | What to do |
|---|---|---|
| Flag `-6` with a nonzero violation | the requirements conflict (Program 3) | read the message, find the limit that cannot be met, relax it |
| Flag `1` but the answer fails your own check | a start with a huge gradient (Program 4) or a wrong model | restart from an ordinary point; verify by an identity |
| Different starts give different answers | more than one local optimum | compare objective values; keep the best (chapter 3) |
| A multiplier you expected to be positive is zero | that limit does not bind at the optimum (Program 5) | the limit is not what is shaping the answer |
| The solver rejects the problem before iterating | a malformed callback (chapter 1, chapter 7) | read the message; audit hand-written derivatives (chapter 7) |

## What to remember

- Follow the routine: formulate, start sensibly, solve, check the flag, the violation and the multipliers, then verify independently.
- A multiplier is a price. Re-solve with a limit nudged to confirm it (Program 1), or compare it with a closed form (Program 2).
- A problem with a known answer is the best test of a formulation.
- An infeasible problem returns its best attempt and a flag, not an error. Read the flag and the violation, never only the parameters (Program 3).
- The optimality measure is relative to the size of the gradient at the start. Start at an ordinary point, try more than one start when it matters, and check the answer by something the solver did not use (Program 4).
- Positive multipliers name the limits that shape the answer; zero multipliers name the ones that do not (Program 5).

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

optima_08_worked_problems/a_minimum_variance_portfolio
optima_08_worked_problems/a_design_with_a_closed_form_answer
optima_08_worked_problems/a_design_that_cannot_be_built
optima_08_worked_problems/a_maximum_likelihood_fit_and_what_a_flag_cannot
optima_08_worked_problems/a_plant_with_every_kind_of_constraint
```
<!-- apiref:toctree:end -->

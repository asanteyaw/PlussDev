# Contributing to PlussII

Thank you for helping improve the PlussII C++ technical computing ecosystem. Contributions to code, documentation, examples, and issue reports are welcome.

## Before you start

- Search [existing issues](https://github.com/asanteyaw/PlussDev/issues) before opening a new one.
- For a bug, include a minimal reproduction, expected and actual behavior, operating system, compiler, and CMake version.
- For a substantial feature or public API change, open an issue first so the design can be discussed before implementation.

## Make a focused change

1. Fork the repository and create a branch from `main`.
2. Keep the change focused. Add or update documentation when user-visible behavior changes.
3. Configure and build with CMake 3.28 or newer and a C++20 compiler. On macOS and Linux, start with:

   ```sh
   cmake --preset core
   cmake --build --preset core
   ```

   On Windows, follow the ClangCL and vcpkg source-build commands on the
   [download page](https://plussible.org/download), selecting Windows and
   “Build from source.”

4. Run the tests relevant to the code you changed. If you add a test target, build and run it explicitly; include the exact commands in your pull request.
5. Open a pull request against `main`. Describe the problem, your solution, platforms checked, and any limitations. Address review feedback in the pull request.

The [installation guide](docs/sphinx/installation.md) covers optional dependencies and source builds. The source is available under the [BSD 3-Clause License](LICENSE).

## One codebase

`main` on GitHub is the single source of truth. Everyone, and every tool or session that edits the code, works from it, so the version one person sees is the version everyone sees.

- **Start from the latest.** Run `git pull --rebase origin main` before you begin, and again before you push.
- **Commit and push when a piece is done.** At the end of a working session nothing should be left uncommitted in your folder. A change that exists only on one disk is not in the next transient or Steady build, and it cannot be reviewed or merged with anyone else's.
- **Keep work small and short-lived.** One focused piece per commit; use a branch for anything that will take more than a day, and merge it back into `main` instead of letting it drift.
- **Do not commit generated output.** Build folders (`build*`, `build_cpp_env`), Sphinx output (`_build`, `.doctrees`), compiled binaries and LaTeX by-products stay out of git. Regenerate them; do not edit them by hand.
- **Names follow one rule.** Free functions live in the lowercase namespaces (`pluss::series`, `pluss::timeline`, `pluss::duration`, `pluss::note`, `pluss::loader`); the capitalised types, such as `Series`, hold methods. Public method and function names are one or two words in `snake_case`.
- **Docs keep the standards.** Run `python3 tools/docs_lint_types.py --check` (it must report 0 hits) and, for the PlussMa reference, `python3 PlussMa/docs/apiref.py lint` before you push documentation.
- **A change to a shared name changes its callers and docs in the same commit.** Rename a function, move it, or remove it only with every call site, example and documentation page updated together, and add the change to `docs/releases/transient.md` under "Names that changed".
- **If your folder and GitHub have diverged, say so before resetting.** Save your work on a branch first (`git switch -c backup-<date>` and commit), then bring your folder to `origin/main`.

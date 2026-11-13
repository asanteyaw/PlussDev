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

## Maintainer release checks

Use a `validation/<topic>` branch while preparing a release. Each push to that
branch automatically builds and checks the Windows distribution, including the
built-in renderer and Atlas view checks. A newer push cancels the older run for
the same branch. This catches Windows-specific compile and link failures before
the final source commit.

After Windows passes, merge or squash the finished change into `main`. The
distribution verification workflow then runs on Windows, Ubuntu, and macOS.
Tag a version only after that full check passes. The tag starts the release
workflow, which builds and publishes the platform ZIPs only when all archive
jobs succeed.

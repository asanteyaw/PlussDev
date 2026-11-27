# The pluss command

```{raw} html
<meta name="pluss-doc-kind" content="guide">
```

[Back to PlussTils](../index.md) · [PlussTils Core Manual](core.md)

`pluss` is a small command in the `bin` folder of the PlussII download. It builds and runs a single C++ file against that download with one command, starts a new project, and checks that your machine can build against PlussII. It needs only CMake and a C++20 compiler, which the download needs anyway.

## How it works

The command finds everything from where it sits, so it works wherever the download was unpacked. Put the `bin` folder on your path, or call it by its full path.

| Command | What it does |
|---|---|
| `pluss run file.cpp [more.cpp ...] [-- arguments]` | builds the files against this PlussII and runs the program |
| `pluss new name` | creates a project folder with a `CMakeLists.txt`, a `main.cpp`, a CMake preset that points at this download, a `.gitignore` and a short README |
| `pluss doctor` | builds and runs a test program against this download, and says what is missing if it cannot |
| `pluss kernel install` | registers the notebook kernel for Jupyter and VS Code (see "Notebooks (preview)") |
| `pluss kernel check [--fix] [notebook ...]` | checks the kernel and VS Code, and the notebooks named |
| `pluss version` | prints the version and channel |

`run` accepts these options before the program's own arguments.

| Option | Meaning |
|---|---|
| `--release` or `--debug` | the build type; the default is `RelWithDebInfo`, which keeps the stack traces that errors print readable |
| `--verbose` | show the build output instead of only showing it when the build fails |
| `--clean` | discard the cached build and start again |

What `run` does for you:

- **One build folder per download.** Every program you run is a target of one project kept in your cache folder, and they all share a precompiled header of `<pluss/pluss.h>`. The first program pays for reading PlussII's headers; the ones after it do not.
- **No work when nothing changed.** A program that has not changed since it was last built runs at once, without calling the build tool. A program that has changed is compiled again; the decision is made from the content of the files, not only their times, so a file you save and run again within the same second is never mistaken for an unchanged one.
- **The program is yours.** It runs in your current folder, receives the arguments after the file (or after `--`), and its exit code is `pluss`'s exit code.

On Windows `run` and `doctor` use the Visual Studio 2022 generator with the ClangCL toolset, which is what the download is built with. Set `PLUSS_CMAKE_ARGS` to add arguments to the CMake configure step on any system.

## Worked tour

The transcripts below are from a Mac. Lines that begin with `$` are what you type.

## Example 1: Check the setup

```text
$ pluss doctor
pluss 0.0.6 (transient)

  [ok]   PlussII download: /path/to/plussii
  [ok]   CMake package files found
  [ok]   cmake version 3.31.5
  [ok]   a C++ compiler was found and CMake can use this download
  [ok]   a test program compiled and linked against this download
  [ok]   the test program ran

PlussII 0.0.6 (transient, Release)
compiler:  AppleClang 21.0.0.21000401
system:    macOS arm64
platforms: cpu
sum:       6

Everything needed is in place.
```

Each line is a check: the download is where `pluss` expects, CMake is new enough, a compiler was found, a program compiled and linked against this download, and it ran. The last lines are that program's own output, which is the version, compiler, system and compute platforms. If a step fails, the line says what to fix and shows the tool's own message.

## Example 2: Run a file

```cpp
#include <pluss/pluss.h>
#include <pluss/tils/kit/datastore.h>

#include <iostream>

int main(int argc, char** argv) {
  auto sales = pluss::loader::datastore("sales").frame();
  std::cout << sales.head(3) << '\n';

  std::cout << "arguments:";
  for (int i = 1; i < argc; ++i) std::cout << ' ' << argv[i];
  std::cout << '\n';
}
```

```text
$ pluss run hello.cpp
===============================================
     | city    month    units  revenue  returns
-----------------------------------------------
0    | Accra   2024-01  116    27.84    3      
1    | Tema    2024-01  90     19.8     2      
2    | Kumasi  2024-01  144    33.84    4      
-----------------------------------------------
[3 cs x 5 feat]
arguments:

$ pluss run hello.cpp -- --flag x
arguments: --flag x
```

The program printed the first rows of an example table and the arguments it received. Run it again unchanged and it starts immediately.

## Example 3: Start a project

```text
$ pluss new myproject
created /home/you/myproject

  cd myproject
  cmake --preset default
  cmake --build --preset default
  ./build/myproject
```

The preset names the download that made the project, so `cmake --preset default` finds PlussII with no further arguments. Move the project to another download by changing `CMAKE_PREFIX_PATH` in `CMakePresets.json`.

## Pitfalls

- **`pluss` must stay in `bin`.** It looks for `include/pluss/pluss.h` beside its own folder. Copying only the executable elsewhere stops it from finding PlussII.
- **Several files form one program.** `pluss run a.cpp b.cpp` builds both into one program; the first word that is not a `.cpp` file starts the program's arguments.
- **Local headers.** A header beside your source is tracked for changes. A header elsewhere is tracked by the build tool; use `--clean` if you ever suspect a stale build.

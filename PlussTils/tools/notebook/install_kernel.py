"""Install the PlussII kernel for Jupyter (prototype).

    pluss kernel install [--name name] [--display-name "shown name"]

Several kernels can be installed side by side, each under its own name (from different downloads, say), each with its
own shared library; `pluss kernel list` shows them and `pluss kernel remove name` removes one.

or, by hand, python3 install_kernel.py [--prefix /path/to/plussii-download] [--clang-repl /path/to/clang-repl]

The download's folder, clang-repl and (on macOS) an SDK that clang-repl can read are looked for
when they are not given. The shared library is built into ~/.pluss the first time.
"""
import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))

# The kernel installed when no --name is given, and the language every PlussII kernel declares. Editors ask a kernel for
# completions under the language it declares, and VS Code names its C++ cells "cpp", so the two must match.
KERNEL_NAME = "pluss"
DEFAULT_DISPLAY_NAME = "PlussII C++"
KERNEL_LANGUAGE = "cpp"
KERNEL_SCRIPT = "pluss_kernel.py"
# What Jupyter accepts as a kernel's name (it compares names without regard to case, so they are kept in lower case).
NAME_PATTERN = re.compile(r"^[a-z0-9][a-z0-9._-]*$")


def kernel_library(name):
    """Where the kernel called `name` keeps its shared library: one per kernel, so installing one never replaces another's."""
    suffix = ".dylib" if sys.platform == "darwin" else ".so"
    return os.path.join(os.path.expanduser("~"), ".pluss", "kernels", name, "libpluss_all" + suffix)


def display_name_for(name):
    return DEFAULT_DISPLAY_NAME if name == KERNEL_NAME else "%s (%s)" % (DEFAULT_DISPLAY_NAME, name)


def pluss_kernels():
    """(name, spec) of every installed kernel that is a PlussII kernel (its program is pluss_kernel.py), by name."""
    from jupyter_client.kernelspec import KernelSpecManager
    manager = KernelSpecManager()
    found = []
    for name in sorted(manager.find_kernel_specs()):
        try:
            spec = manager.get_kernel_spec(name)
        except Exception:  # a broken kernel of another program is not ours to report
            continue
        if len(spec.argv) > 1 and os.path.basename(spec.argv[1]) == KERNEL_SCRIPT:
            found.append((name, spec))
    return found


def download_of(spec):
    """The PlussII download a kernel was installed from (the folder that holds its include folder)."""
    include = (spec.env or {}).get("PLUSS_INCLUDE", "")
    return os.path.dirname(include.rstrip("/\\")) if include else ""


def find_clang_repl():
    """clang-repl from the PATH, or from the usual LLVM install folders."""
    found = shutil.which("clang-repl")
    if found:
        return found
    places = ["/opt/homebrew/opt/llvm/bin", "/usr/local/opt/llvm/bin", "/usr/lib/llvm/bin"]
    places += ["/usr/lib/llvm-%d/bin" % v for v in range(25, 16, -1)]
    for folder in places:
        candidate = os.path.join(folder, "clang-repl")
        if os.path.exists(candidate):
            return candidate
    return None


def sysroot_works(clang_repl, sysroot):
    """Whether clang-repl can read the C++ library of this SDK (a newer SDK than the LLVM knows fails here)."""
    cmd = [clang_repl, "-Xcc", "-std=c++20", "-Xcc", "-isysroot", "-Xcc", sysroot, "-Xcc", "-nostdinc++",
           "-Xcc", "-isystem", "-Xcc", sysroot + "/usr/include/c++/v1"]
    try:
        done = subprocess.run(cmd, input=b"#include <cmath>\n#include <iostream>\nfloat probe = INFINITY;\n", capture_output=True, timeout=120)
    except (OSError, subprocess.TimeoutExpired):
        return False
    return b"error" not in done.stderr + done.stdout


ENGINE_NAME = "pluss-engine"
# What a Linux machine needs to build the engine, named in the message when it cannot be built there.
ENGINE_PACKAGES = "macOS: brew install llvm; Ubuntu: sudo apt install libclang-cpp18-dev llvm-18-dev"


def build_engine(clang_repl, output):
    """Builds engine.cpp into `output` against the LLVM that provides clang_repl: the engine that keeps earlier cells
    when a cell throws. (True, "") when it was built, (False, why) when it could not be (the kernel then uses clang-repl)."""
    prefix = os.path.dirname(os.path.dirname(os.path.realpath(clang_repl)))
    if not os.path.exists(os.path.join(prefix, "include", "clang", "Interpreter", "Interpreter.h")):
        return False, "clang's development headers are not in %s (%s)" % (prefix, ENGINE_PACKAGES)
    compiler = os.path.join(prefix, "bin", "clang++")
    if not os.path.exists(compiler):
        compiler = shutil.which("clang++") or "c++"
    try:
        resource = subprocess.run([compiler, "-print-resource-dir"], capture_output=True, text=True, timeout=60).stdout.strip()
        library_dir = os.path.join(prefix, "lib")
        command = [compiler, "-std=c++17", "-frtti", "-O1", os.path.join(HERE, "engine.cpp"), "-I", os.path.join(prefix, "include"),
                   "-L", library_dir, "-lclang-cpp", "-lLLVM", "-Wl,-rpath," + library_dir,
                   '-DPLUSS_RESOURCE_DIR="%s"' % resource, "-o", output]
        done = subprocess.run(command, capture_output=True, text=True, timeout=600)
    except (OSError, subprocess.TimeoutExpired) as error:
        return False, str(error)
    if done.returncode != 0 or not os.path.exists(output):
        last = [line for line in done.stderr.splitlines() if "error" in line][:1]
        return False, (last[0] if last else "the build failed") + " (%s)" % ENGINE_PACKAGES
    return True, ""


def find_sysroot(clang_repl):
    """On macOS, the newest SDK of the Command Line Tools that this clang-repl can read."""
    if sys.platform != "darwin" or not clang_repl:
        return ""
    folder = "/Library/Developer/CommandLineTools/SDKs"
    if not os.path.isdir(folder):
        return ""
    names = sorted((n for n in os.listdir(folder) if n.startswith("MacOSX") and n.endswith(".sdk") and n != "MacOSX.sdk"), reverse=True)
    for name in names + ["MacOSX.sdk"]:
        path = os.path.join(folder, name)
        if os.path.exists(path) and sysroot_works(clang_repl, path):
            return path
    return ""


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--prefix", default=os.path.abspath(os.path.join(HERE, "..", "..", "..")), help="the extracted PlussII download (default: the one this script came with)")
    parser.add_argument("--clang-repl", default=find_clang_repl(), help="path to clang-repl")
    parser.add_argument("--sysroot", default=None, help="SDK to compile against (macOS)")
    parser.add_argument("--extra-flags", default="-DACCELERATE_NEW_LAPACK" if sys.platform == "darwin" else "")
    parser.add_argument("--user", action="store_true", help="install for the current user (default)")
    parser.add_argument("--yes", action="store_true", help="also add the VS Code setting that turns on completions, without asking")
    parser.add_argument("--name", default=KERNEL_NAME, help="the kernel's name, to keep several side by side (default: %s)" % KERNEL_NAME)
    parser.add_argument("--display-name", default=None, help="what Jupyter and VS Code show for it (default: %s, with the name when it is not the default)" % DEFAULT_DISPLAY_NAME)
    args = parser.parse_args()
    args.name = args.name.lower()
    if not NAME_PATTERN.match(args.name):
        sys.exit("'%s' cannot be a kernel's name: use letters, digits, '.', '_' and '-', starting with a letter or a digit." % args.name)
    display_name = args.display_name or display_name_for(args.name)
    if args.sysroot is None:
        args.sysroot = find_sysroot(args.clang_repl)
    if not args.clang_repl:
        sys.exit("clang-repl was not found. Install LLVM (macOS: brew install llvm; Ubuntu: sudo apt install clang) or pass --clang-repl.")
    if not os.path.isdir(os.path.join(args.prefix, "include", "pluss")):
        sys.exit("%s does not look like a PlussII download (no include/pluss); pass --prefix." % args.prefix)
    try:
        from jupyter_client.kernelspec import KernelSpecManager
        import ipykernel  # noqa: F401
    except ImportError:
        sys.exit("The notebook kernel needs Jupyter's Python packages: run  %s -m pip install ipykernel jupyter_client  and try again." % sys.executable)

    work = tempfile.mkdtemp(prefix="pluss_kernel_")
    spec_dir = os.path.join(work, args.name)
    os.makedirs(spec_dir)
    library = kernel_library(args.name)
    os.makedirs(os.path.dirname(library), exist_ok=True)
    subprocess.run([sys.executable, os.path.join(HERE, "make_library.py"), args.prefix, library], check=True)
    engine = os.path.join(os.path.dirname(library), ENGINE_NAME)
    built, why = build_engine(args.clang_repl, engine)
    if built:
        print("built the notebook engine (a cell that throws keeps the variables of earlier cells)")
    else:
        print("could not build the notebook engine, so clang-repl runs the cells and a cell that throws restarts it: " + why)
        engine = ""
    spec = {
        "argv": [sys.executable, os.path.join(HERE, "pluss_kernel.py"), "-f", "{connection_file}"],
        "display_name": display_name,
        "language": KERNEL_LANGUAGE,
        "env": {
            "PLUSS_CLANG_REPL": args.clang_repl,
            "PLUSS_ENGINE": engine,
            "PLUSS_SYSROOT": args.sysroot,
            "PLUSS_INCLUDE": os.path.join(args.prefix, "include"),
            "PLUSS_LIBRARY": library,
            "PLUSS_EXTRA_FLAGS": args.extra_flags,
        },
    }
    with open(os.path.join(spec_dir, "kernel.json"), "w") as handle:
        json.dump(spec, handle, indent=2)
    path = KernelSpecManager().install_kernel_spec(spec_dir, kernel_name=args.name, user=True)
    print("installed the kernel '%s' (%s) at %s" % (args.name, display_name, path))
    print("Open Jupyter or VS Code and pick the kernel '%s'.\n" % display_name)
    # Earlier installers kept one library for every kernel in ~/.pluss; once no kernel uses it, it is only taking room.
    in_use = {(spec.env or {}).get("PLUSS_LIBRARY") for _, spec in pluss_kernels()}
    for suffix in (".dylib", ".so"):
        old_library = os.path.join(os.path.expanduser("~"), ".pluss", "libpluss_all" + suffix)
        if os.path.exists(old_library) and old_library not in in_use:
            os.remove(old_library)
    # The same checks as `pluss kernel check`. VS Code needs one setting to ask the kernel for completions; it is added
    # only when --yes was given or the answer to the question is yes.
    import kernel_check
    fix = args.yes
    missing = kernel_check.editors_missing_setting()
    if missing and not fix and sys.stdin.isatty():
        names = ", ".join(editor for editor, _ in missing)
        answer = input("%s needs the setting %s to offer completions for C++ cells.\nAdd it to the user settings now? [y/N] " % (names, kernel_check.SETTING))
        fix = answer.strip().lower() in ("y", "yes")
    kernel_check.run_checks(args.prefix, fix=fix, name=args.name)


if __name__ == "__main__":
    main()

"""Install the PlussII kernel for Jupyter (prototype).

    pluss kernel install

or, by hand, python3 install_kernel.py [--prefix /path/to/plussii-download] [--clang-repl /path/to/clang-repl]

The download's folder, clang-repl and (on macOS) an SDK that clang-repl can read are looked for
when they are not given. The shared library is built into ~/.pluss the first time.
"""
import argparse
import json
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))

# The kernel's name, and the language it declares. Editors ask a kernel for completions under the language it declares,
# and VS Code names its C++ cells "cpp", so the two must match; kernel_check.py reads these same two values.
KERNEL_NAME = "pluss"
KERNEL_LANGUAGE = "cpp"


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
    args = parser.parse_args()
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
    spec_dir = os.path.join(work, "pluss")
    os.makedirs(spec_dir)
    suffix = ".dylib" if sys.platform == "darwin" else ".so"
    library = os.path.join(os.path.expanduser("~"), ".pluss", "libpluss_all" + suffix)
    os.makedirs(os.path.dirname(library), exist_ok=True)
    subprocess.run([sys.executable, os.path.join(HERE, "make_library.py"), args.prefix, library], check=True)
    spec = {
        "argv": [sys.executable, os.path.join(HERE, "pluss_kernel.py"), "-f", "{connection_file}"],
        "display_name": "PlussII C++",
        "language": KERNEL_LANGUAGE,
        "env": {
            "PLUSS_CLANG_REPL": args.clang_repl,
            "PLUSS_SYSROOT": args.sysroot,
            "PLUSS_INCLUDE": os.path.join(args.prefix, "include"),
            "PLUSS_LIBRARY": library,
            "PLUSS_EXTRA_FLAGS": args.extra_flags,
        },
    }
    with open(os.path.join(spec_dir, "kernel.json"), "w") as handle:
        json.dump(spec, handle, indent=2)
    path = KernelSpecManager().install_kernel_spec(spec_dir, kernel_name=KERNEL_NAME, user=True)
    print("installed the PlussII C++ kernel at", path)
    print("Open Jupyter or VS Code and pick the kernel 'PlussII C++'.\n")
    # The same checks as `pluss kernel check`. VS Code needs one setting to ask the kernel for completions; it is added
    # only when --yes was given or the answer to the question is yes.
    import kernel_check
    fix = args.yes
    missing = kernel_check.editors_missing_setting()
    if missing and not fix and sys.stdin.isatty():
        names = ", ".join(editor for editor, _ in missing)
        answer = input("%s needs the setting %s to offer completions for C++ cells.\nAdd it to the user settings now? [y/N] " % (names, kernel_check.SETTING))
        fix = answer.strip().lower() in ("y", "yes")
    kernel_check.run_checks(args.prefix, fix=fix)


if __name__ == "__main__":
    main()

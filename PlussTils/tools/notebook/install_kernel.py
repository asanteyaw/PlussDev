"""Install the PlussII kernel for Jupyter (prototype).

    python3 install_kernel.py --prefix /path/to/plussii-download --clang-repl /path/to/clang-repl

On macOS pass --sysroot with an SDK the clang-repl can read (it must not be newer than that
clang knows). The shared library is built next to the kernel the first time.
"""
import argparse
import json
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--prefix", required=True, help="the extracted PlussII download")
    parser.add_argument("--clang-repl", default=shutil.which("clang-repl"), help="path to clang-repl")
    parser.add_argument("--sysroot", default="", help="SDK to compile against (macOS)")
    parser.add_argument("--extra-flags", default="-DACCELERATE_NEW_LAPACK" if sys.platform == "darwin" else "")
    parser.add_argument("--user", action="store_true", help="install for the current user (default)")
    args = parser.parse_args()
    if not args.clang_repl:
        sys.exit("clang-repl was not found; pass --clang-repl")

    from jupyter_client.kernelspec import KernelSpecManager

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
        "language": "c++",
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
    path = KernelSpecManager().install_kernel_spec(spec_dir, kernel_name="pluss", user=True)
    print("installed the PlussII C++ kernel at", path)


if __name__ == "__main__":
    main()

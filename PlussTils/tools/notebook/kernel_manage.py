"""List and remove the PlussII notebook kernels.

    pluss kernel list
    pluss kernel remove name

Several PlussII kernels can be installed side by side, each under its own name (see `pluss kernel install --name`).
`list` shows each one with what editors call it and the download it runs; `remove` unregisters one and deletes the shared
library it was given, and touches nothing else (not the download, not the notebooks).
"""
import argparse
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import install_kernel as installer  # noqa: E402


def need_jupyter():
    try:
        import jupyter_client  # noqa: F401
    except ImportError:
        sys.exit("Jupyter's Python packages are not installed for %s: run  %s -m pip install ipykernel jupyter_client"
                 % (sys.executable, sys.executable))


def list_kernels():
    kernels = installer.pluss_kernels()
    if not kernels:
        print("No PlussII kernel is installed. Run  pluss kernel install  (add --name to choose its name).")
        return 0
    rows = [("name", "shown as", "download")]
    rows += [(name, spec.display_name, installer.download_of(spec) or "(unknown)") for name, spec in kernels]
    widths = [max(len(row[i]) for row in rows) for i in range(2)]
    for row in rows:
        print("  %-*s   %-*s   %s" % (widths[0], row[0], widths[1], row[1], row[2]))
    return 0


def remove_kernel(name):
    name = name.lower()
    kernels = dict(installer.pluss_kernels())
    if name not in kernels:
        known = ", ".join(sorted(kernels)) or "none"
        print("No PlussII kernel is called '%s' (installed: %s)." % (name, known))
        return 1
    from jupyter_client.kernelspec import KernelSpecManager
    library = (kernels[name].env or {}).get("PLUSS_LIBRARY", "")
    KernelSpecManager().remove_kernel_spec(name)
    print("removed the kernel '%s' (%s)" % (name, kernels[name].display_name))
    # Only the folder this installer made for the kernel's library goes; a library anywhere else was not ours to make.
    own_folder = os.path.dirname(installer.kernel_library(name))
    if library and os.path.dirname(library) == own_folder and os.path.isdir(own_folder):
        shutil.rmtree(own_folder)
        print("deleted its shared library (%s)" % own_folder)
    return 0


def main():
    parser = argparse.ArgumentParser(description="List and remove the PlussII notebook kernels.")
    parser.add_argument("--prefix", default=None, help=argparse.SUPPRESS)  # passed by the pluss command; not needed here
    parser.add_argument("action", choices=["list", "remove"])
    parser.add_argument("name", nargs="?", help="the kernel to remove")
    args = parser.parse_args()
    need_jupyter()
    if args.action == "list":
        return list_kernels()
    if not args.name:
        parser.error("remove needs the kernel's name (see pluss kernel list)")
    return remove_kernel(args.name)


if __name__ == "__main__":
    sys.exit(main())

"""Link the PlussII static libraries into one shared library that clang-repl can load.

clang-repl compiles each cell in memory and resolves PlussII's functions from a shared library, so
the static libraries of a PlussII download are joined into one. Usage:

    python3 make_library.py /path/to/plussii-download /path/to/libpluss_all.dylib
"""
import glob
import os
import platform
import subprocess
import sys


def main(prefix, output):
    libs = sorted(glob.glob(os.path.join(prefix, "lib", "*.a"))) + sorted(glob.glob(os.path.join(prefix, "lib", "pluss-native", "*.a")))
    shared = sorted(glob.glob(os.path.join(prefix, "lib", "*.dylib"))) + sorted(glob.glob(os.path.join(prefix, "lib", "*.so")))
    if not libs:
        sys.exit("no static libraries under " + os.path.join(prefix, "lib"))
    lib_dir = os.path.join(prefix, "lib")
    if platform.system() == "Darwin":
        frameworks = ["Accelerate", "Cocoa", "UniformTypeIdentifiers", "WebKit", "QuartzCore", "CoreText", "CoreFoundation", "CoreGraphics", "CoreVideo", "Metal"]
        cmd = ["/usr/bin/clang++", "-dynamiclib", "-arch", platform.machine(), "-Wl,-all_load", "-Wl,-undefined,dynamic_lookup"]
        cmd += libs + shared
        for name in frameworks:
            cmd += ["-framework", name]
        cmd += ["-lcurl", "-lsqlite3", "-ldl", "-lm", "-lpthread", "-Wl,-rpath," + lib_dir, "-install_name", "@rpath/" + os.path.basename(output), "-o", output]
    else:
        cmd = ["c++", "-shared", "-fPIC", "-Wl,--whole-archive"] + libs + ["-Wl,--no-whole-archive"] + shared
        cmd += ["-lcurl", "-lsqlite3", "-ldl", "-lm", "-lpthread", "-Wl,-rpath," + lib_dir, "-o", output]
    subprocess.run(cmd, check=True)
    print("wrote", output)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    main(*sys.argv[1:])

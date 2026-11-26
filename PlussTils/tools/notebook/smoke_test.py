"""Run the PlussII notebook kernel end to end and check what a user would see.

    python3 smoke_test.py /path/to/plussii-download [--clang-repl path]

It registers the kernel in a temporary Jupyter folder, starts it, runs cells and checks the output:
state kept between cells, a name defined again, a table and a plot shown inline, help from the bundled
documentation, a crash and an interrupt that restart the engine, and a compile error that does not.
Exit status 0 means every check passed.
"""
import argparse
import os
import queue
import subprocess
import sys
import tempfile
import threading

HERE = os.path.dirname(os.path.abspath(__file__))
failures = []


def check(name, ok, detail=""):
    print(("ok   " if ok else "FAIL ") + name + ("" if ok else "  -- " + str(detail)[:300]), flush=True)
    if not ok:
        failures.append(name)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("prefix")
    parser.add_argument("--clang-repl", default=None)
    parser.add_argument("--use-installed", action="store_true", help="test the kernel that is already registered (from pluss kernel install)")
    args = parser.parse_args()
    if not args.use_installed:
        os.environ["JUPYTER_DATA_DIR"] = tempfile.mkdtemp(prefix="pluss_jupyter_")
    cmd = [sys.executable, os.path.join(HERE, "install_kernel.py"), "--prefix", args.prefix]
    if args.clang_repl:
        cmd += ["--clang-repl", args.clang_repl]
    if not args.use_installed:
        installed = subprocess.run(cmd, capture_output=True, text=True)
        print(installed.stdout, installed.stderr)
        check("kernel installs", installed.returncode == 0, installed.stderr)
        if installed.returncode != 0:
            return 1

    # The kernel writes what it sends to the engine and gets back to this file; it is printed when a check fails.
    log = os.path.join(tempfile.mkdtemp(prefix="pluss_notebook_log_"), "engine.log")
    os.environ["PLUSS_NOTEBOOK_LOG"] = log

    from jupyter_client.manager import start_new_kernel

    km, kc = start_new_kernel(kernel_name="pluss", startup_timeout=600)

    def show_log():
        if os.path.exists(log):
            # What the kernel and the compile engine said to each other, so a failure on one machine can be read.
            lines = open(log, errors="replace").read().splitlines()
            print("---- engine log (last %d of %d lines) ----" % (min(len(lines), 150), len(lines)))
            print("\n".join(line[:400] for line in lines[-150:]), flush=True)

    def run(code, timeout=300):
        mid = kc.execute(code)
        seen = {"text": "", "err": "", "mime": [], "error": None}
        while True:
            try:
                m = kc.get_iopub_msg(timeout=timeout)
            except queue.Empty:
                # The cell never finished: say which one and what the engine was doing, then stop.
                check("a cell finishes within %d s" % timeout, False, code)
                show_log()
                km.shutdown_kernel(now=True)
                sys.exit(1)
            if m["parent_header"].get("msg_id") != mid:
                continue
            t, c = m["msg_type"], m["content"]
            if t == "stream":
                seen["err" if c["name"] == "stderr" else "text"] += c["text"]
            elif t == "display_data":
                seen["mime"] += list(c["data"])
            elif t == "error":
                seen["error"] = c["evalue"]
                seen["traceback"] = "\n".join(c.get("traceback", []))
            elif t == "status" and c["execution_state"] == "idle":
                return seen

    try:
        r = run("double square(double v) { return v * v; }")
        check("a function is defined", r["error"] is None, r["err"])
        r = run("std::cout << square(3) << std::endl;")
        check("a later cell uses it", r["text"].strip() == "9", r)
        run("double square(double v) { return v * v * 10; }")
        r = run("std::cout << square(2) << std::endl;")
        check("a function defined again replaces it", r["text"].strip() == "40", r)
        run("struct Point { double x, y; };")
        run("struct Point { double x, y, z; };")
        r = run("Point q{1, 2, 3}; std::cout << q.z << std::endl;")
        check("a type defined again replaces it", r["text"].strip() == "3", r)
        run("auto v = 1;")
        run("auto v = 2;")
        r = run("std::cout << v << std::endl;")
        check("a variable defined again is rebound", r["text"].strip() == "2", r)
        r = run("int broken = ;")
        check("a compile error is reported", r["error"] is not None, r)
        check("it names the user's line and shows no internal call",
              "int broken = ;" in r.get("traceback", "") and "pluss::nb::show" not in r.get("traceback", ""), r)
        r = run("an_undeclared_name")
        check("an undeclared name on the last line is shown as the user wrote it",
              r["error"] is not None and "an_undeclared_name" in r.get("traceback", "") and "pluss::nb::show" not in r.get("traceback", ""), r)
        r = run("std::cout << v << std::endl;")
        check("the engine survives a compile error", r["text"].strip() == "2", r)
        run("#include <pluss/tils/kit/datastore.h>")  # a notebook starts with <pluss/pluss.h> only, as a main.cpp would
        r = run('auto df = pluss::loader::datastore("sales").frame(); df')
        check("a table is shown inline", "text/html" in r["mime"], r)
        r = run("df.show();")
        check(".show() on a table goes inline", "text/html" in r["mime"], r)
        r = run('auto chart = df.plot().line({"units", "revenue"}); pluss::nb::show(chart);')
        check("a plot is shown inline", "image/svg+xml" in r["mime"], r)
        run("#include <pluss/tils/doc.h>")
        r = run('pluss::tils::doc("DataFrame");')
        check("doc() answers offline from the download", len(r["text"].strip()) > 50 and "not found" not in r["text"].lower(), r)
        r = run("int* n = nullptr; *n = 1;")
        check("a crash restarts the engine and says so", r["error"] is not None, r)
        r = run('std::cout << "alive" << std::endl;')
        check("the next cell runs after a crash", r["text"].strip() == "alive", r)
        threading.Timer(5, km.interrupt_kernel).start()
        r = run("while (true) {}")
        check("a running cell can be interrupted", r["error"] is not None, r)
        r = run('std::cout << "alive2" << std::endl;')
        check("the next cell runs after an interrupt", r["text"].strip() == "alive2", r)
        def completions(text):
            """What the kernel offers at the end of `text` (the first request waits for the prelude's precompiled header)."""
            mid = kc.complete(text, len(text))
            while True:
                m = kc.get_shell_msg(timeout=600)
                if m["parent_header"].get("msg_id") == mid:
                    return m["content"]["matches"]

        r = run("#include <numeric>\n#include <vector>")
        r = run("std::vector<int> nums{1, 2, 3, 4}; auto total = std::accumulate(nums.begin(), nums.end(), 0); "
                "auto square_of = [](int x) { return x * x; }; std::cout << total << ' ' << square_of(5) << std::endl;")
        check("plain C++ runs, not only PlussII code", r["text"].strip() == "10 25", r)
        run("auto sv = pluss::make_series({1.0, 2.0, 3.0});")
        names = completions("std::cou")
        check("names complete", "cout" in names, names)
        check("a variable from an earlier cell completes", "sv" in completions("s"), "sv not offered")
        check("a dot offers the members of the variable", "sum" in completions("sv."), "sum not offered after sv.")
    finally:
        km.shutdown_kernel(now=True)
    print("%d check(s) failed" % len(failures))
    if failures:
        show_log()
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())

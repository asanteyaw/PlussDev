"""A Jupyter kernel for PlussII C++ (prototype).

Each cell is C++ that is compiled and run by clang-repl, which keeps variables, functions and
includes alive from one cell to the next. The kernel starts clang-repl once, loads the PlussII
libraries into it, and sends it every cell followed by an end marker; what the cell prints comes
back as the cell's output. Tables and plots are returned as rich output:

    df                      // the last expression of a cell is shown
    pluss::nb::show(df);    // or show anything explicitly

The settings come from the environment, which `install_kernel.py` writes into the kernel's
kernel.json:

    PLUSS_CLANG_REPL   path to clang-repl
    PLUSS_SYSROOT      the SDK to compile against (macOS)
    PLUSS_INCLUDE      the PlussII include folder
    PLUSS_LIBRARY      one shared library holding PlussII (see make_library.py)
    PLUSS_EXTRA_FLAGS  anything else clang-repl should be told, e.g. -DACCELERATE_NEW_LAPACK
"""
import os
import queue
import re
import shlex
import subprocess
import tempfile
import threading

from ipykernel.kernelbase import Kernel

END = "\x1e<<pluss-end>>"
MIME = "\x1e<<pluss-mime "

# What the kernel adds in front of the first cell. `pluss::nb::show` writes a rich value to the
# output as a marker line the kernel turns into a Jupyter display message.
PRELUDE = r'''
#include <pluss/pluss.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
namespace pluss::nb {
inline void emit(const std::string& mime, const std::string& body) {
  std::cout << "\x1e<<pluss-mime " << mime << " " << body.size() << ">>\n" << body << std::flush;
}
inline void show(const std::string& text) { std::cout << text << std::endl; }
inline void show(const char* text) { std::cout << text << std::endl; }
inline void show(const pluss::table::DataFrame& frame) {
  emit("text/html", pluss::table::display::render_html(frame, {}));
}
template <class Chart>
auto show(Chart& chart) -> decltype(chart.save_svg(std::string()), void()) {
  const std::string path = std::string(std::getenv("TMPDIR") ? std::getenv("TMPDIR") : "/tmp") + "/pluss_nb_chart.svg";
  chart.save_svg(path);
  std::ifstream in(path, std::ios::binary);
  emit("image/svg+xml", std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()));
}
template <class T>
auto show(const T& value) -> decltype(std::cout << value, void()) { std::cout << value << std::endl; }
}  // namespace pluss::nb
'''


class PlussKernel(Kernel):
    implementation = "pluss"
    implementation_version = "0.0.1"
    language = "c++"
    language_version = "20"
    language_info = {
        "name": "c++",
        "mimetype": "text/x-c++src",
        "file_extension": ".cpp",
        "codemirror_mode": "text/x-c++src",
    }
    banner = "PlussII C++ kernel (clang-repl)"

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self._out = queue.Queue()
        self._err = queue.Queue()
        self._proc = None
        self._start_engine()

    # ---- the engine -----------------------------------------------------------------------------

    def _command(self):
        repl = os.environ.get("PLUSS_CLANG_REPL", "clang-repl")
        cmd = [repl, "-Xcc", "-std=c++20"]
        sysroot = os.environ.get("PLUSS_SYSROOT")
        if sysroot:
            cmd += ["-Xcc", "-isysroot", "-Xcc", sysroot, "-Xcc", "-nostdinc++", "-Xcc", "-isystem", "-Xcc", sysroot + "/usr/include/c++/v1"]
        include = os.environ.get("PLUSS_INCLUDE")
        if include:
            cmd += ["-Xcc", "-I" + include]
        for flag in shlex.split(os.environ.get("PLUSS_EXTRA_FLAGS", "")):
            cmd += ["-Xcc", flag]
        return cmd

    def _start_engine(self):
        self._proc = subprocess.Popen(
            self._command(), stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, bufsize=0
        )
        for stream, target in ((self._proc.stdout, self._out), (self._proc.stderr, self._err)):
            threading.Thread(target=self._pump, args=(stream, target), daemon=True).start()
        self._cells = tempfile.mkdtemp(prefix="pluss_cells_")
        self._count = 0
        # %lib is a command of its own; everything else goes in as a file.
        self._send("%lib " + os.environ["PLUSS_LIBRARY"] + "\n")
        self._run_text(PRELUDE)

    @staticmethod
    def _pump(stream, target):
        while True:
            chunk = stream.read1(65536) if hasattr(stream, "read1") else stream.read(65536)
            if not chunk:
                target.put(None)
                return
            target.put(chunk)

    def _send(self, line):
        self._proc.stdin.write(line.encode())
        self._proc.stdin.flush()

    def _run_text(self, text):
        """Run a cell in the engine and collect what it printed until both end markers arrive.

        clang-repl reads one line at a time, so a cell is written to a file and brought in with a
        single #include line: that keeps its lines together, and what it declares stays defined
        for the cells after it."""
        self._count += 1
        path = os.path.join(self._cells, "cell_%d.cpp" % self._count)
        with open(path, "w") as handle:
            handle.write(text.rstrip("\n") + "\n")
        marker = 'std::cout << "\\x1e<<pluss-end>>" << std::endl; std::cerr << "\\x1e<<pluss-end>>" << std::endl;\n'
        self._send('#include "%s"\n' % path + marker)
        self._last_path = path
        collected = []
        for source in (self._out, self._err):
            buffer = b""
            while True:
                chunk = source.get()
                if chunk is None:
                    raise RuntimeError("the clang-repl engine stopped")
                buffer += chunk
                if END.encode() in buffer:
                    buffer = buffer.split(END.encode())[0]
                    break
            collected.append(buffer.decode(errors="replace"))
        return collected[0], collected[1].replace(self._last_path, "cell")

    # ---- cells ----------------------------------------------------------------------------------

    @staticmethod
    def _show_last_expression(code):
        """A cell whose last line is a bare expression shows it, as a notebook does."""
        lines = code.rstrip().split("\n")
        last = lines[-1].strip()
        if not last or last[-1] in ";{}" or last.startswith(("#", "%", "//")):
            return code
        if code.count("{") != code.count("}") or code.count("(") != code.count(")"):
            return code
        lines[-1] = "pluss::nb::show(" + last + ");"
        return "\n".join(lines)

    def do_execute(self, code, silent, store_history=True, user_expressions=None, allow_stdin=False):
        out, err = self._run_text(self._show_last_expression(code))
        failed = "error:" in err
        first_error = next((line for line in err.split("\n") if "error:" in line), err.strip().split("\n")[0] if err.strip() else "")
        if not silent:
            self._publish(out)
            if err.strip():
                self.send_response(self.iopub_socket, "stream", {"name": "stderr", "text": err})
        if failed:
            return {"status": "error", "ename": "CompileError", "evalue": first_error, "traceback": [err]}
        return {"status": "ok", "execution_count": self.execution_count, "payload": [], "user_expressions": {}}

    def _publish(self, out):
        """Plain text becomes a stream; each marker becomes a display message."""
        pattern = re.compile(re.escape(MIME) + r"(\S+) (\d+)>>\n")
        at = 0
        while True:
            found = pattern.search(out, at)
            if not found:
                break
            text = out[at:found.start()]
            if text:
                self.send_response(self.iopub_socket, "stream", {"name": "stdout", "text": text})
            size = int(found.group(2))
            start = found.end()
            # The size is in bytes; the text came through as characters, so measure by encoding.
            body, used = "", 0
            while start + used < len(out) and len(body.encode()) < size:
                used += 1
                body = out[start:start + used]
            self.send_response(self.iopub_socket, "display_data", {"data": {found.group(1): body}, "metadata": {}})
            at = start + used
        rest = out[at:]
        if rest:
            self.send_response(self.iopub_socket, "stream", {"name": "stdout", "text": rest})

    def do_shutdown(self, restart):
        if self._proc and self._proc.poll() is None:
            self._proc.terminate()
        return {"status": "ok", "restart": restart}


if __name__ == "__main__":
    from ipykernel.kernelapp import IPKernelApp

    IPKernelApp.launch_instance(kernel_class=PlussKernel)

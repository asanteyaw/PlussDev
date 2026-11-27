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
import ctypes
import os
import platform
import queue
import re
import shlex
import shutil
import subprocess
import sys
import time
import tempfile
import threading

from ipykernel.kernelbase import Kernel

END = "\x1e<<pluss-end>>"
MIME = "\x1e<<pluss-mime "

# What the kernel adds in front of the first cell. `pluss::nb::show` writes a rich value to the
# output as a marker line the kernel turns into a Jupyter display message.
# The library is built with these on (a CMake project gets them from the pluss target); without them the compiler sees
# Timeline and Duration without their methods, and PlussTils headers such as bi/studio.h fail to compile.
SURFACE_DEFINES = ["-DCODEH_ENABLE_TIMELINE_SURFACE", "-DCODEH_ENABLE_INDEX_ACCESSOR_SURFACE", "-DCODEH_ENABLE_DURATION_SURFACE"]

PRELUDE = r'''
#include <pluss/pluss.h>
#include <exception>
#include <iostream>
#include <type_traits>
namespace pluss::nb {
// An exception that escapes a cell ends the engine; before it does, its message (a PlussII error's what() is its
// traceback) is written behind a marker, so the kernel can show what went wrong instead of only that the engine stopped.
inline const bool uncaught_reported = [] {
  std::set_terminate([] {
    if (const auto error = std::current_exception()) {
      std::cerr << "\x1e<<pluss-uncaught>>";
      try {
        std::rethrow_exception(error);
      } catch (const std::exception& e) {
        std::cerr << e.what();
      } catch (...) {
        std::cerr << "an exception that is not a std::exception";
      }
      std::cerr << "\x1e<<pluss-uncaught-end>>" << std::flush;
    }
    std::abort();
  });
  return true;
}();
inline void emit(const std::string& mime, const std::string& body) {
  std::cout << "\x1e<<pluss-mime " << mime << " " << body.size() << ">>\n" << body << std::flush;
}
// What a cell's last value looks like, chosen once for every type, best first: a figure shows nothing (a figure appears
// only when its show() is called, as in a program), a table type is an HTML table (see
// pluss::table::display::NotebookTable: a frame and every frame a method returns, head() and describe() included), and
// anything else that prints is shown as the terminal shows it.
struct as_text {};
struct as_table : as_text {};
struct as_figure : as_table {};
template <class T>
auto show_(T& figure, as_figure) -> decltype(figure.save_svg(std::string()), void()) {
  (void)figure;
}
template <class T>
  requires pluss::table::display::NotebookTable<std::remove_cvref_t<T>>
void show_(T& table, as_table) {
  emit("text/html", pluss::table::display::render_notebook_html(table));
}
template <class T>
auto show_(T& value, as_text) -> decltype(std::cout << value, void()) {
  std::cout << value << std::endl;
}
template <class T>
void show(T&& value) {
  show_(value, as_figure{});
}
}  // namespace pluss::nb
'''


class EngineStopped(RuntimeError):
    """The clang-repl process ended, for example because a cell crashed it; `report` is what it said before."""

    def __init__(self, report=""):
        super().__init__("the engine stopped")
        self.report = report


class EngineBroken(EngineStopped):
    """The engine is running but can no longer link what it compiles; `report` is what it said before that."""

    def __init__(self, report):
        super().__init__(report)


def rename_identifier(code, base, new):
    """Replace the identifier `base` by `new` in C++ code, leaving strings, characters and comments alone."""
    out, i, n = [], 0, len(code)
    while i < n:
        c = code[i]
        if code.startswith("//", i):
            j = code.find("\n", i)
            j = n if j < 0 else j
            out.append(code[i:j])
            i = j
        elif code.startswith("/*", i):
            j = code.find("*/", i + 2)
            j = n if j < 0 else j + 2
            out.append(code[i:j])
            i = j
        elif c in "\"'":
            j = i + 1
            while j < n and code[j] != c:
                j += 2 if code[j] == "\\" else 1
            out.append(code[i:j + 1])
            i = j + 1
        elif c.isalpha() or c == "_":
            j = i
            while j < n and (code[j].isalnum() or code[j] == "_"):
                j += 1
            word = code[i:j]
            out.append(new if word == base else word)
            i = j
        else:
            out.append(c)
            i += 1
    return "".join(out)


TYPE_DEFINITION = re.compile(r"\b(?:struct|class|union|enum(?:\s+class|\s+struct)?)\s+(\w+)\s*(?:final\s*)?(?::[^;{]*)?\{")
VERSION = re.compile(r"(\w+?)__v\d+\b")
# A cell run again defines its names again; that is renamed and retried. A name that clashes with a library's (a variable
# called `nan` beside the C library's nan()) is "a different kind of symbol": that is the user's error to see, not a rerun.
REDEFINED = re.compile(r"redefinition of '(\w+)'(?! as different kind of symbol)")
# What the engine says when its compiled code can no longer be linked (after a failed cell left half-made definitions): it
# will not finish the cell, so waiting for it would wait forever.
ENGINE_BROKEN = ("JIT session error", "Failed to materialize symbols")

# One line of the compiler's report that starts a diagnostic: "cell:1:17: error: use of undeclared identifier 'b'".
DIAGNOSTIC = re.compile(r"^(?P<file>\S[^:]*):(?P<line>\d+):(?P<column>\d+): (?P<kind>fatal error|error|warning|note): (?P<message>.*)$")
# What the kernel wraps a cell's last expression in so that it is shown; a reader never sees it.
SHOW_CALL = "pluss::nb::show("


def format_compile_error(report, code, sent, undef_lines, number):
    """The compiler's report as a traceback about the user's own code, in the style of a pluss::Error:

        Traceback (most recent call last):
          Cell [3], line 2
            auto y = f(x);
                       ^
        CompileError: use of undeclared identifier 'x'

    The compiler talks about the text the kernel sent, which can differ from what the user wrote (the last
    expression is wrapped in a call that shows it, a header may be undefined first), so each place is mapped back to
    the user's line and column. Returns (traceback text, message of the first error)."""
    user_lines = code.rstrip("\n").split("\n")
    sent_lines = sent.rstrip("\n").split("\n")
    wrapped = bool(sent_lines) and SHOW_CALL in sent_lines[-1] and SHOW_CALL not in user_lines[-1]

    def locate(line, column):
        """The user's line number, line text and 0-based column for a place in the text that was sent."""
        number_in_cell = line - undef_lines
        if not 1 <= number_in_cell <= len(user_lines):
            return None
        text = user_lines[number_in_cell - 1]
        offset = column - 1
        inside_wrapper = False
        if wrapped and number_in_cell == len(sent_lines):
            call = sent_lines[-1].find(SHOW_CALL)
            payload = call + len(SHOW_CALL)
            start = PlussKernel._last_statement_start(text)
            first = start + (len(text[start:]) - len(text[start:].lstrip()))
            if offset < call:
                pass
            elif offset < payload:
                offset = first  # inside the display wrapper: point at the expression it shows
                inside_wrapper = True
            else:
                offset = first + (offset - payload)
        return number_in_cell, text, max(0, min(offset, len(text))), inside_wrapper

    blocks, current = [], None
    for raw in report.split("\n"):
        found = DIAGNOSTIC.match(raw)
        if found:
            current = {"head": found, "rest": []}
            blocks.append(current)
        elif current is not None:
            current["rest"].append(raw)
    sections, first_message = [], ""
    for block in blocks:
        head = block["head"]
        kind, message = head.group("kind"), VERSION.sub(r"\1", head.group("message"))
        if kind != "note" and not first_message and kind != "warning":
            first_message = message
        place = locate(int(head.group("line")), int(head.group("column"))) if head.group("file") == "cell" else None
        if place is None:
            # Inside a header or somewhere the user did not write: the compiler's own words are all there is.
            lines = ["%s:%s:%s: %s: %s" % (head.group("file"), head.group("line"), head.group("column"), kind, message)]
            lines += [l for l in block["rest"] if l.strip()]
            sections.append("\n".join(lines))
            continue
        where, text, column, inside_wrapper = place
        if inside_wrapper:
            message = "this value cannot be displayed (%s)" % message
        label = {"error": "CompileError", "fatal error": "CompileError", "warning": "Warning", "note": "Note"}[kind]
        sections.append("\n".join([
            "  Cell [%d], line %d" % (number, where),
            "    " + text,
            "    " + " " * column + "^",
            "%s: %s" % (label, message),
        ]))
    if not sections:
        return report.strip(), first_message
    return "Traceback (most recent call last):\n" + "\n\n".join(sections), first_message


# One line of the compiler's code-completion report: "COMPLETION: sum : [#Series#]sum(<#int dim#>)".
COMPLETION = re.compile(r"^COMPLETION: (?P<name>\S+?)(?: : (?P<signature>.*))?$")
IDENTIFIER = re.compile(r"^[A-Za-z_]\w*$")
# How many completions one request returns. The editor filters the first reply as the user keeps typing and does not ask
# again, so a reply has to carry every member of a type (a Series has several hundred); only a name space such as std::
# is cut.
MAX_COMPLETIONS = 5000


def describe_completion(signature):
    """A completion's signature as a reader would write it: "sum(int dim) -> Series" (clang marks the result type
    [#...#] and each parameter <#...#>)."""
    if not signature:
        return ""
    result = re.search(r"\[#(.*?)#\]", signature)
    text = re.sub(r"\[#.*?#\]", "", signature)
    text = re.sub(r"<#(.*?)#>", r"\1", text)
    text = re.sub(r"\{#(.*?)#\}", r"\1", text)
    return text + (" -> " + result.group(1) if result else "")


class ClangCompleter:
    """Member and name completion from the compiler itself, which knows what `s1.` offers because it has parsed the
    type of `s1`. It runs clang in code-completion mode over the prelude, the cells that ran, and the current cell up
    to the cursor, so a name an earlier cell declared is known. The prelude is compiled once into a precompiled header,
    which keeps a request to a fraction of a second."""

    def __init__(self, clang, flags, prelude):
        self.clang = clang
        # Top-level statements (a loop outside a function), as the engine accepts them; the precompiled prelude is built
        # with the same option, since a precompiled header must match the language options of what includes it.
        self.flags = list(flags) + ["-Xclang", "-fincremental-extensions"]
        self.prelude = prelude
        self.folder = tempfile.mkdtemp(prefix="pluss_complete_")
        self.header = os.path.join(self.folder, "prelude.h")
        self._lock = threading.Lock()
        self._prepared = False
        with open(self.header, "w") as handle:
            handle.write(prelude)

    def prepare(self):
        """Compiles the prelude into a precompiled header (clang finds prelude.h.pch beside the header it includes).
        Without it a request still works, only slower."""
        with self._lock:
            if self._prepared:
                return
            self._prepared = True
            try:
                subprocess.run([self.clang, *self.flags, "-w", "-x", "c++-header", self.header, "-o", self.header + ".pch"],
                               capture_output=True, timeout=600)
            except (OSError, subprocess.TimeoutExpired):
                pass

    def check(self, earlier, text, timeout=120):
        """The compile errors of a cell's text after the text of the cells that ran, as the engine would report them
        (the cell's file is named "cell"), or "" when it compiles. Nothing runs. A cell that does not compile is never
        sent to the engine, which a failed cell can leave unable to link anything more."""
        self.prepare()
        ran = os.path.join(self.folder, "ran.cpp")
        cell = os.path.join(self.folder, "checked_cell.cpp")
        unit = os.path.join(self.folder, "check.cpp")
        with open(ran, "w") as handle:
            handle.write("".join(piece.rstrip("\n") + "\n" for piece in earlier))
        with open(cell, "w") as handle:
            handle.write(text.rstrip("\n") + "\n")
        with open(unit, "w") as handle:
            handle.write('#include "%s"\n#include "%s"\n' % (ran, cell))
        command = [self.clang, *self.flags, "-w", "-fsyntax-only", "-ferror-limit=20", "-include", self.header, unit]
        try:
            done = subprocess.run(command, capture_output=True, text=True, timeout=timeout)
        except (OSError, subprocess.TimeoutExpired):
            return ""  # the check could not be made: the engine is asked, as before
        if "error:" not in done.stderr:
            return ""
        return done.stderr.replace(cell, "cell").replace(unit, "cell")

    def complete(self, history, code, cursor_pos, timeout=60):
        """(start, matches, details) for the cursor in `code`, or None when the compiler could not be asked."""
        start = cursor_pos
        while start > 0 and (code[start - 1].isalnum() or code[start - 1] == "_"):
            start -= 1
        self.prepare()
        earlier = "".join(cell.rstrip("\n") + "\n" for cell in history)
        # The cell being typed is a function body: at file scope `s1.` is not an expression, so the compiler would
        # offer declarations. What the earlier cells declared stays at file scope, as it is in the engine.
        opening = "void pluss_cell_being_typed() {\n"
        text = earlier + opening + code + "\n}\n"
        before = earlier.count("\n") + opening.count("\n") + code[:cursor_pos].count("\n") + 1
        column = len(code[:cursor_pos].split("\n")[-1].encode("utf-8")) + 1
        source = os.path.join(self.folder, "cell.cpp")
        with open(source, "w") as handle:
            handle.write(text)
        command = [self.clang, *self.flags, "-w", "-fsyntax-only", "-include", self.header, "-Xclang",
                   "-code-completion-at=%s:%d:%d" % (source, before, column), source]
        try:
            done = subprocess.run(command, capture_output=True, text=True, timeout=timeout)
        except (OSError, subprocess.TimeoutExpired):
            return None
        found = {}
        for line in done.stdout.split("\n"):
            match = COMPLETION.match(line)
            if match is None:
                continue
            name = match.group("name")
            if name == "Pattern" or not IDENTIFIER.match(name):
                continue
            found.setdefault(name, describe_completion(match.group("signature")))
        prefix = code[start:cursor_pos]
        if not prefix.startswith("_"):
            # A library's own helpers (_conj, _fw_primal) are noise in a list; they are offered once an underscore is typed.
            found = {name: detail for name, detail in found.items() if not name.startswith("_")}
        names = sorted(found)[:MAX_COMPLETIONS]
        return start, names, {name: found[name] for name in names}


class PlussKernel(Kernel):
    implementation = "pluss"
    implementation_version = "0.0.1"
    # The language as editors name it ("cpp" is VS Code's id for C++, which it gives new cells and asks for completions
    # under); the same value install_kernel.py writes, so the kernel and its registration never disagree.
    language = "cpp"
    language_version = "20"
    language_info = {
        "name": "cpp",
        "mimetype": "text/x-c++src",
        "file_extension": ".cpp",
        "codemirror_mode": "text/x-c++src",
    }
    banner = "PlussII C++ kernel (clang-repl)"

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        # The engine inherits these: tables and plots go back to the notebook instead of opening a window,
        # and doc() finds the documentation that came with the download.
        os.environ["PLUSS_NOTEBOOK"] = "1"
        include = os.environ.get("PLUSS_INCLUDE", "")
        if include and not os.environ.get("PLUSS_PREFIX"):
            os.environ["PLUSS_PREFIX"] = os.path.dirname(include.rstrip("/"))
        self._words = None
        self._doc_lib = None    # the documentation lookup in the shared library (False when it cannot be loaded)
        self._last_completion = None
        self._history = []      # the cells that ran, for the compiler to read when it completes a name
        self._completer = self._make_completer()
        self._out = queue.Queue()
        self._err = queue.Queue()
        self._proc = None
        # The engine takes several seconds to read the PlussII headers. Start it in the background so
        # the notebook is usable at once: by the time the first cell is written it is usually ready.
        self._ready = threading.Event()
        self._start_failure = None
        threading.Thread(target=self._start_in_background, daemon=True).start()

    def _start_in_background(self):
        try:
            self._start_engine()
        except Exception as error:  # reported by the first cell that needs the engine
            self._start_failure = error
        finally:
            self._ready.set()

    # ---- the engine -----------------------------------------------------------------------------

    def _command(self):
        # The PlussII engine when the install could build it (a cell that throws does not end it), clang-repl otherwise.
        repl = os.environ.get("PLUSS_ENGINE") or os.environ.get("PLUSS_CLANG_REPL", "clang-repl")
        cmd = [repl, "-Xcc", "-std=c++20"]
        sysroot = os.environ.get("PLUSS_SYSROOT")
        if sysroot:
            cmd += ["-Xcc", "-isysroot", "-Xcc", sysroot, "-Xcc", "-nostdinc++", "-Xcc", "-isystem", "-Xcc", sysroot + "/usr/include/c++/v1"]
        include = os.environ.get("PLUSS_INCLUDE")
        if include:
            cmd += ["-Xcc", "-I" + include]
        if sys.platform.startswith("linux") and platform.machine() in ("aarch64", "arm64"):
            # Atomics become calls to libgcc helpers (__aarch64_ldadd4_*) that the engine's JIT cannot link.
            cmd += ["-Xcc", "-mno-outline-atomics"]
        for flag in SURFACE_DEFINES + shlex.split(os.environ.get("PLUSS_EXTRA_FLAGS", "")):
            cmd += ["-Xcc", flag]
        return cmd

    @staticmethod
    def _make_completer():
        """The compiler-backed completer, or None when there is no clang to ask (completion then offers the words typed
        so far). The compiler is clang-repl's own folder's clang++, or the one on the PATH."""
        explicit = os.environ.get("PLUSS_CLANG")
        repl = os.environ.get("PLUSS_CLANG_REPL", "")
        candidates = [explicit] if explicit else []
        if repl:
            candidates.append(os.path.join(os.path.dirname(repl), "clang++"))
        candidates.append(shutil.which("clang++"))
        clang = next((c for c in candidates if c and os.path.exists(c)), None)
        if clang is None:
            return None
        flags = ["-std=c++20"]
        sysroot = os.environ.get("PLUSS_SYSROOT")
        if sysroot:
            flags += ["-isysroot", sysroot, "-nostdinc++", "-isystem", sysroot + "/usr/include/c++/v1"]
        include = os.environ.get("PLUSS_INCLUDE")
        if include:
            flags += ["-I", include]
        flags += SURFACE_DEFINES + shlex.split(os.environ.get("PLUSS_EXTRA_FLAGS", ""))
        return ClangCompleter(clang, flags, PRELUDE)

    def _start_engine(self):
        # A new engine has new queues, so output of one that stopped can never reach the next.
        self._out = queue.Queue()
        self._err = queue.Queue()
        self._versions = {}  # name -> how many times a cell has redefined it
        self._types = set()  # types defined so far
        self._macros = {}    # name -> the name its latest definition has
        self._ran = []       # the text of each cell the engine ran, as sent: what a new cell is checked against
        self._proc = subprocess.Popen(
            self._command(), stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, bufsize=0
        )
        for stream, target, name in ((self._proc.stdout, self._out, "stdout"), (self._proc.stderr, self._err, "stderr")):
            threading.Thread(target=self._pump, args=(stream, target, name), daemon=True).start()
        self._cells = tempfile.mkdtemp(prefix="pluss_cells_")
        self._count = 0
        # %lib is a command of its own; everything else goes in as a file.
        self._send("%lib " + os.environ["PLUSS_LIBRARY"] + "\n")
        self._run_text(PRELUDE)
        if self._completer is not None:
            # Compile the prelude into a precompiled header in the background, so the first completion is not slow.
            threading.Thread(target=self._completer.prepare, daemon=True).start()

    @staticmethod
    def _log(kind, text):
        """With PLUSS_NOTEBOOK_LOG set to a file, everything the kernel sends to the engine and gets back is written
        there (the smoke test prints it when a check fails), so a platform that misbehaves can be read, not guessed."""
        path = os.environ.get("PLUSS_NOTEBOOK_LOG")
        if not path:
            return
        with open(path, "a", errors="replace") as handle:
            handle.write("[%.3f] %s %r\n" % (time.time(), kind, text if isinstance(text, str) else text.decode(errors="replace")))

    @staticmethod
    def _pump(stream, target, name="engine"):
        while True:
            chunk = stream.read1(65536) if hasattr(stream, "read1") else stream.read(65536)
            if not chunk:
                PlussKernel._log(name + " ended", "")
                target.put(None)
                return
            PlussKernel._log(name, chunk)
            target.put(chunk)

    def _send(self, line):
        self._log("sent", line)
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
        # Both outputs are read together, so a broken engine is seen as soon as it says so instead of after a marker
        # that will never come.
        sources = (self._out, self._err)
        buffers = [b"", b""]
        done = [False, False]
        while not all(done):
            for i, source in enumerate(sources):
                if done[i]:
                    continue
                try:
                    chunk = source.get(timeout=0.05)
                except queue.Empty:
                    continue
                if chunk is None:
                    raise EngineStopped(buffers[1].decode(errors="replace").replace(self._last_path, "cell"))
                buffers[i] += chunk
                if END.encode() in buffers[i]:
                    buffers[i] = buffers[i].split(END.encode())[0]
                    done[i] = True
            said = buffers[1].decode(errors="replace")
            if any(sign in said for sign in ENGINE_BROKEN):
                before = min(said.find(sign) for sign in ENGINE_BROKEN if sign in said)
                raise EngineBroken(said[:before].replace(self._last_path, "cell"))
        return buffers[0].decode(errors="replace"), buffers[1].decode(errors="replace").replace(self._last_path, "cell")

    # ---- cells ----------------------------------------------------------------------------------

    @staticmethod
    def _last_statement_start(line):
        """Where the last statement of a line begins: just after its last semicolon outside strings, brackets and braces."""
        depth, i, start, n = 0, 0, 0, len(line)
        while i < n:
            c = line[i]
            if line.startswith("//", i):
                break
            if c in "\"'":
                j = i + 1
                while j < n and line[j] != c:
                    j += 2 if line[j] == "\\" else 1
                i = j
            elif c in "([{":
                depth += 1
            elif c in ")]}":
                depth -= 1
            elif c == ";" and depth == 0:
                start = i + 1
            i += 1
        return start

    @staticmethod
    def _split_last_expression(code):
        """(everything before it, the expression) when the last statement of the cell is a bare expression, such as `df`
        on its own line or the `df` of `auto df = load(); df`; None when the cell ends with a declaration or a statement."""
        lines = code.rstrip().split("\n")
        line = lines[-1]
        if line.strip().startswith(("#", "%", "//")):
            return None
        start = PlussKernel._last_statement_start(line)
        head, last = line[:start], line[start:].strip()
        if not last or last[-1] in ";{}":
            return None
        if code.count("{") != code.count("}") or code.count("(") != code.count(")"):
            return None
        return "\n".join(lines[:-1] + [head]) if head else "\n".join(lines[:-1]), last

    @staticmethod
    def _show_last_expression(code):
        """A cell whose last statement is a bare expression shows it, as a notebook does: `df` on its own line, or
        `auto df = load(); df` on one line."""
        split = PlussKernel._split_last_expression(code)
        if split is None:
            return code
        before, last = split
        lines = code.rstrip().split("\n")
        head = lines[-1][:PlussKernel._last_statement_start(lines[-1])]
        lines[-1] = head + " pluss::nb::show(" + last + ");" if head else "pluss::nb::show(" + last + ");"
        return "\n".join(lines)

    @staticmethod
    def _declarations_of(code):
        """The cell as the completer reads it: without a final bare expression, which only shows a value, declares
        nothing, and without a semicolon would run into the next cell's first line."""
        split = PlussKernel._split_last_expression(code)
        return code if split is None else split[0]

    @staticmethod
    def _user_traceback(text):
        """An error's traceback without the frames of the engine itself (its own program, clang's interpreter and the
        compiled cells it cannot name), which say nothing about the user's code."""
        lines = text.split("\n")
        kept, skip_source = [], False
        for line in lines:
            frame = re.match(r'\s*File "([^"]*)"', line)
            if frame:
                skip_source = frame.group(1) in ("pluss-engine", "<unknown>") or frame.group(1).startswith("libclang-cpp")
                if skip_source:
                    continue
            elif skip_source and line.startswith("    "):
                continue
            else:
                skip_source = False
            if "frames without a line were built without debug information" in line:
                continue
            kept.append(line)
        return "\n".join(kept)

    def _fail(self, silent, ename, evalue, text):
        """Report a failed cell the way a notebook expects: an error message on the output channel (so the cell is
        marked failed and Run All stops) and an error reply. The text is the traceback shown under the cell."""
        if not silent:
            self.send_response(self.iopub_socket, "error", {"ename": ename, "evalue": evalue, "traceback": [text]})
        return {"status": "error", "ename": ename, "evalue": evalue, "traceback": [text]}

    def _guarded(self, code, force_macros=None):
        """The cell as it is sent: a cell with #include lines runs with the renaming macros off, so a header never
        sees them, and the macros of every redefined name are in force again after it."""
        macros = self._macros if force_macros is None else force_macros
        text = code
        if "#include" in code and macros:
            text = "".join("#undef %s\n" % base for base in macros) + text
        if macros:
            text = text.rstrip("\n") + "\n" + "".join("#undef %s\n#define %s %s\n" % (b, b, m) for b, m in macros.items())
        return text

    def _tidy(self, text):
        """What the user reads: the renaming suffix removed and the include chain of the compiler's report dropped."""
        lines = [l for l in text.split("\n") if not l.startswith("In file included from") and not l.startswith("<<< inputs >>>")
                 and "included multiple times" not in l]
        return VERSION.sub(r"\1", "\n".join(lines))

    def _run_cell(self, code):
        """Run a cell. A name the cell declares again is bound to a new definition, as running a cell again
        rebinds a name in a notebook: the old definition keeps its place for the code compiled against it."""
        text = self._show_last_expression(code)
        self._cell_info = (code, text, 0)  # what an error report is about, until the run below says more
        macros = dict(self._macros)
        # The engine cannot recover from a type defined twice, so a type that already exists is renamed up front.
        for base in TYPE_DEFINITION.findall(text):
            if base in self._types:
                version = self._versions.get(base, 1) + 1
                self._versions[base] = version
                new = "%s__v%d" % (base, version)
                macros[base] = new
                text = rename_identifier(text, base, new)
        source = text
        out, err = "", ""
        for _ in range(4):
            guarded = self._guarded(source, macros)
            # The cell is compiled against the cells that ran before the engine sees it: a cell with an error is reported
            # without touching the engine (whose state a failed cell can break). A name defined again (a cell run
            # again) is renamed and checked again; only code that compiles goes to the engine.
            problems = self._completer.check(self._ran, guarded) if self._completer is not None else ""
            names = {VERSION.sub(r"\1", name) for name in REDEFINED.findall(problems)}
            others = [line for line in problems.split("\n") if "error:" in line and not REDEFINED.search(line)]
            if others:
                out, err = "", problems
                break
            if not names:
                out, err = self._run_text(guarded)
                names = {VERSION.sub(r"\1", name) for name in REDEFINED.findall(err)}
                if not names:
                    if "error:" not in err:
                        self._ran.append(guarded)
                    break
            for base in names:
                version = self._versions.get(base, 1) + 1
                self._versions[base] = version
                new = "%s__v%d" % (base, version)
                macros[base] = new
                source = rename_identifier(source, base, new)
        else:
            return out, err
        self._cell_info = (code, source, len(macros) if "#include" in code and macros else 0)
        if "error:" not in err:
            self._macros = macros
            self._types.update(VERSION.sub(r"\1", name) for name in TYPE_DEFINITION.findall(text))
        return out, err

    def do_execute(self, code, silent, store_history=True, user_expressions=None, allow_stdin=False):
        self._ready.wait()
        if self._start_failure is not None:
            message = "the C++ engine did not start: %s" % self._start_failure
            return {"status": "error", "ename": "EngineError", "evalue": message, "traceback": [message]}
        self._remember_words(code)
        try:
            out, err = self._run_cell(code)
        except KeyboardInterrupt:
            # The engine cannot be interrupted part-way, so it is restarted: the cell stops, earlier cells are gone.
            self._restart_engine()
            message = "Interrupted. The C++ engine was restarted, so the variables and functions from earlier cells are gone: run them again."
            return self._fail(silent, "KeyboardInterrupt", message, message)
        except EngineBroken as broken:
            # The cell's error left the engine unable to link anything more: show the error, and start it afresh.
            self._restart_engine()
            note = ("The C++ engine could not go on after this error and has been restarted, so the variables and functions "
                    "from earlier cells are gone: fix the cell and run the earlier cells again.")
            report = "\n".join(line for line in self._tidy(broken.report).split("\n")
                               if line.strip() != "error: Parsing failed." and not re.match(r"^\d+ errors? generated", line.strip()))
            shown = ""
            if "error:" in report:
                code_sent, source_sent, undef = getattr(self, "_cell_info", (code, code, 0))
                report, shown = format_compile_error(report, code_sent, source_sent, undef, self.execution_count)
            text = (report.rstrip() + "\n\n" if shown else "") + note
            return self._fail(silent, "CompileError" if shown else "EngineRestarted", shown or note, text)
        except EngineStopped as stopped:
            self._restart_engine()
            # An error the cell threw and did not catch: show it as the error it is.
            thrown = re.search(r"\x1e<<pluss-uncaught>>(.*?)\x1e<<pluss-uncaught-end>>", stopped.report, re.S)
            if thrown:
                text = (self._user_traceback(thrown.group(1)).rstrip() + "\n\nThe error was not caught in the cell, which ends the C++ engine: it has been "
                        "restarted, so the variables and functions from earlier cells are gone: run them again.")
                first = thrown.group(1).strip().split("\n")[-1] if thrown.group(1).strip() else "an error was thrown"
                return self._fail(silent, "UncaughtError", first, text)
            message = ("The C++ engine stopped while running this cell (a crash in the code, such as using a null pointer, ends it). "
                       "It has been restarted, so the variables and functions from earlier cells are gone: run them again.")
            return self._fail(silent, "EngineRestarted", message, message)
        except BrokenPipeError:
            self._restart_engine()
            message = ("The C++ engine stopped while running this cell (a crash in the code, such as using a null pointer, ends it). "
                       "It has been restarted, so the variables and functions from earlier cells are gone: run them again.")
            return self._fail(silent, "EngineRestarted", message, message)
        out, err = self._tidy(out), self._tidy(err)
        thrown = re.search(r"\x1e<<pluss-uncaught>>(.*?)\x1e<<pluss-uncaught-end>>\n?", err, re.S)
        if thrown:
            # The engine reported an error the cell threw and carried on: everything earlier cells made is still there.
            err = err[:thrown.start()] + err[thrown.end():]
            if not silent:
                self._publish(out)
            self._history.append(self._declarations_of(code))
            message = self._user_traceback(thrown.group(1))
            first = message.strip().split("\n")[-1] if message.strip() else "an error was thrown"
            return self._fail(silent, "UncaughtError", first, message.rstrip())
        failed = "error:" in err
        first_error = next((line for line in err.split("\n") if "error:" in line), err.strip().split("\n")[0] if err.strip() else "")
        if failed:
            # The report is about the text that was sent; say it about the user's own code instead.
            report = "\n".join(l for l in err.split("\n") if l.strip() != "error: Parsing failed." and not re.match(r"^\d+ errors? generated", l.strip()))
            code_sent, source_sent, undef = getattr(self, "_cell_info", (code, code, 0))
            err, shown = format_compile_error(report, code_sent, source_sent, undef, self.execution_count)
            first_error = shown or first_error
        if not silent:
            self._publish(out)
            if err.strip() and not failed:
                self.send_response(self.iopub_socket, "stream", {"name": "stderr", "text": err})
        if failed:
            return self._fail(silent, "CompileError", first_error, err)
        self._history.append(self._declarations_of(code))
        return {"status": "ok", "execution_count": self.execution_count, "payload": [], "user_expressions": {}}

    def _restart_engine(self):
        try:
            if self._proc and self._proc.poll() is None:
                self._proc.kill()
        finally:
            self._ready.clear()
            self._history = []  # the earlier cells are gone with the engine
            try:
                self._start_engine()
                self._start_failure = None
            except Exception as error:
                self._start_failure = error
            finally:
                self._ready.set()

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

    # ---- completion and help ----------------------------------------------------------------------

    KEYWORDS = ("auto", "bool", "break", "case", "catch", "char", "class", "const", "constexpr", "continue", "double", "else",
                "enum", "false", "float", "for", "if", "include", "int", "long", "namespace", "nullptr", "return", "short",
                "size_t", "static", "std", "string", "struct", "switch", "template", "this", "throw", "true", "try",
                "typename", "using", "vector", "void", "while", "pluss")

    def _remember_words(self, code):
        if self._words is None:
            self._words = set(self.KEYWORDS)
        self._words.update(re.findall(r"[A-Za-z_]\w{2,}", code))
        threading.Thread(target=self._scan_headers, daemon=True).start()

    def _scan_headers(self):
        """The names in the PlussII headers, read once, so completion also offers what the library provides."""
        if getattr(self, "_scanned", False):
            return
        self._scanned = True
        root = os.path.join(os.environ.get("PLUSS_INCLUDE", ""), "pluss")
        found = set()
        for folder, _, files in os.walk(root):
            for name in files:
                if name.endswith((".h", ".hpp")):
                    try:
                        with open(os.path.join(folder, name), errors="ignore") as handle:
                            found.update(re.findall(r"\b[a-z][a-z0-9_]{3,}(?=\()", handle.read()))
                    except OSError:
                        pass
        if self._words is None:
            self._words = set(self.KEYWORDS)
        self._words.update(found)

    def do_complete(self, code, cursor_pos):
        """Names for the cursor. The compiler answers when it can (what `s1.` offers, what follows `pluss::`); otherwise
        the words typed so far in this notebook are offered."""
        # With PLUSS_NOTEBOOK_LOG set this shows whether the editor asks the kernel for completions at all.
        self._log("complete", code[:cursor_pos])
        if self._completer is not None:
            try:
                answer = self._completer.complete(self._history, code, cursor_pos)
            except Exception:
                answer = None
            if answer is not None:
                start, names, details = answer
                context = code[:start]
                # In a member list the compiler also offers the class's own name (Series, shown as "Series::"): it says
                # which class the members belong to, which the hover needs, and is no use to type, so it is not offered.
                member_class = None
                if context.rstrip().endswith((".", "->")):
                    member_class = next((name for name in names if details.get(name) == name + "::"), None)
                    names = [name for name in names if name != member_class]
                self._last_completion = {"context": context, "names": names, "details": details, "member_class": member_class}
                kinds = [{"text": name, "type": "function" if "(" in details[name] else "property",
                          "signature": details[name]} for name in names]
                return {"status": "ok", "matches": names, "cursor_start": start, "cursor_end": cursor_pos,
                        "metadata": {"_jupyter_types_experimental": kinds}}
        start = cursor_pos
        while start > 0 and (code[start - 1].isalnum() or code[start - 1] == "_"):
            start -= 1
        prefix = code[start:cursor_pos]
        words = self._words or set(self.KEYWORDS)
        matches = sorted(w for w in words if w.startswith(prefix) and w != prefix) if prefix else []
        return {"status": "ok", "matches": matches[:200], "cursor_start": start, "cursor_end": cursor_pos, "metadata": {}}

    def _doc_library(self):
        """The shared library's documentation lookup, loaded once; None when it cannot be (an older library)."""
        if self._doc_lib is None:
            try:
                lib = ctypes.CDLL(os.environ["PLUSS_LIBRARY"])
                lib.pluss_tils_doc_brief.restype = ctypes.c_void_p
                lib.pluss_tils_doc_brief.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
                lib.pluss_tils_free.argtypes = [ctypes.c_void_p]
                self._doc_lib = lib
            except (OSError, AttributeError, KeyError):
                self._doc_lib = False
        return self._doc_lib or None

    def _brief(self, name):
        """A few lines about `name` from the documentation (signature, what it does, address), or ''."""
        lib = self._doc_library()
        if lib is None or not name:
            return ""
        raw = lib.pluss_tils_doc_brief(name.encode("utf-8"), os.environ.get("PLUSS_PREFIX", "").encode("utf-8"))
        if not raw:
            return ""
        try:
            return ctypes.string_at(raw).decode("utf-8", "replace")
        finally:
            lib.pluss_tils_free(raw)

    @staticmethod
    def _type_name(type_text):
        """"const pluss::Series &" -> "Series": the name a documentation page is called by."""
        text = re.sub(r"<.*>", "", type_text).replace("const", "").replace("&", "").replace("*", "").strip()
        return text.split("::")[-1].strip()

    def _inspect_target(self, code, cursor_pos):
        """(name to look up, line to show with it, show that line even when a page is found) for the cursor.

        An editor asking about a suggestion in the list sends text that ends with the suggestion's name (VS Code sends
        the first characters of the cell and the name, which is only right on a one-line cell), so the name is matched
        against the list this kernel just offered, which also says what it belongs to."""
        before = code[:cursor_pos]
        last = getattr(self, "_last_completion", None)
        if last:
            fits = [name for name in last["names"] if before.endswith(name)]
            if fits:
                name = max(fits, key=len)
                detail = last["details"].get(name, "")
                if last["member_class"]:
                    return last["member_class"] + "::" + name, last["member_class"] + "::" + detail, False
                qualifier = re.search(r"((?:[A-Za-z_]\w*::)+)$", last["context"])
                if qualifier:
                    return qualifier.group(1) + name, qualifier.group(1) + detail, False
                if " -> " in detail and "(" not in detail:  # a variable: its type, then what the type is
                    kind = detail.split(" -> ", 1)[1]
                    return self._type_name(kind), "%s: %s" % (name, kind), True
                return name, detail, False
        end = cursor_pos
        while end < len(code) and (code[end].isalnum() or code[end] in "_:"):
            end += 1
        start = cursor_pos
        while start > 0 and (code[start - 1].isalnum() or code[start - 1] in "_:"):
            start -= 1
        return code[start:end].strip(":"), "", False

    def do_inspect(self, code, cursor_pos, detail_level=0, omit_sections=()):
        """The hover for a suggestion or for the name under the cursor, from the documentation that came with the
        download. It does not run anything in the notebook's session."""
        name, line, always = self._inspect_target(code, cursor_pos)
        # Pages are called by the name a reader uses (Series, not pluss::Series): the last part is tried after the whole.
        text = self._brief(name) or (self._brief(name.split("::")[-1]) if "::" in name else "")
        if always and line:
            text = line + ("\n\n" + text if text else "")
        elif not text:
            text = line
        if not text:
            return {"status": "ok", "found": False, "data": {}, "metadata": {}}
        return {"status": "ok", "found": True, "data": {"text/plain": text}, "metadata": {}}

    def do_shutdown(self, restart):
        if self._proc and self._proc.poll() is None:
            self._proc.terminate()
        return {"status": "ok", "restart": restart}


if __name__ == "__main__":
    from ipykernel.kernelapp import IPKernelApp

    IPKernelApp.launch_instance(kernel_class=PlussKernel)

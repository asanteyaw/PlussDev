"""Check the PlussII notebook kernel and the editor that talks to it, and say what to change.

    pluss kernel check [--fix] [notebook.ipynb ...]

It looks at the installed kernel (does it belong to the download you are running, does it declare the language the
editors ask for completions under, do clang-repl and the shared library work) and at VS Code (is the setting that turns
on completions for this language there). Notebooks named on the command line are scanned for language tags that an
older kernel left in their cells, which stop the cells from running.

Without --fix nothing is changed. With --fix the VS Code setting is added and the notebooks are cleaned, each after
saving the original beside it. The same checks run at the end of `pluss kernel install`.
"""
import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import install_kernel as installer  # noqa: E402

KERNEL_NAME = installer.KERNEL_NAME
KERNEL_LANGUAGE = installer.KERNEL_LANGUAGE
SETTING = "jupyter.completionTriggerCharacters"
TRIGGER_CHARACTERS = [".", ":", ">"]
EDITORS = ("Code", "Code - Insiders", "VSCodium")
# Language tags an older kernel (which declared "c++") left in notebook cells.
STALE_LANGUAGES = ("c++", "c++11", "c++14", "c++17", "c++20")


# ---- VS Code settings (a JSON file that allows comments and trailing commas) --------------------------

def strip_jsonc(text):
    """The text of a settings file without its comments and trailing commas, so that json can read it."""
    out, i, n = [], 0, len(text)
    while i < n:
        c = text[i]
        if c == '"':
            j = i + 1
            while j < n and text[j] != '"':
                j += 2 if text[j] == "\\" else 1
            out.append(text[i:j + 1])
            i = j + 1
        elif text.startswith("//", i):
            j = text.find("\n", i)
            i = n if j < 0 else j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            i = n if j < 0 else j + 2
        else:
            out.append(c)
            i += 1
    return re.sub(r",(\s*[}\]])", r"\1", "".join(out))


def read_settings(text):
    """The settings as a dict, or None when the text is not something this reader can parse."""
    try:
        data = json.loads(strip_jsonc(text)) if text.strip() else {}
    except ValueError:
        return None
    return data if isinstance(data, dict) else None


def has_trigger_entry(settings):
    entry = settings.get(SETTING)
    return isinstance(entry, dict) and bool(entry.get(KERNEL_LANGUAGE))


def add_trigger_entry(text):
    """The settings text with the setting added after its opening brace (comments and layout stay as they are), or None
    when the setting is already there under another shape, or the text has no object to add it to."""
    settings = read_settings(text)
    if settings is None or SETTING in settings:
        return None
    opening = re.match(r"(?:\s|//[^\n]*\n|/\*.*?\*/)*\{", text, re.S) if text.strip() else None
    line = '  "%s": { "%s": %s }' % (SETTING, KERNEL_LANGUAGE, json.dumps(TRIGGER_CHARACTERS))
    if opening is None:
        return "{\n%s\n}\n" % line if not text.strip() else None
    comma = "," if settings else ""
    return text[:opening.end()] + "\n" + line + comma + text[opening.end():]


def settings_files():
    """The user settings files of the editors that are installed (the folder of the editor exists)."""
    home = os.path.expanduser("~")
    if sys.platform == "darwin":
        base = os.path.join(home, "Library", "Application Support")
    elif sys.platform.startswith("win"):
        base = os.environ.get("APPDATA", os.path.join(home, "AppData", "Roaming"))
    else:
        base = os.environ.get("XDG_CONFIG_HOME", os.path.join(home, ".config"))
    found = []
    for editor in EDITORS:
        folder = os.path.join(base, editor, "User")
        if os.path.isdir(folder):
            found.append((editor, os.path.join(folder, "settings.json")))
    return found


def save_beside(path, suffix):
    """Copies the file to <file><suffix> (a numbered name when that exists) and returns the copy's path."""
    target = path + suffix
    number = 1
    while os.path.exists(target):
        number += 1
        target = "%s%s.%d" % (path, suffix, number)
    shutil.copy2(path, target)
    return target


def apply_setting(path):
    """Adds the setting to the file; (True, message) when done, (False, why) when it was not."""
    text = open(path, encoding="utf-8").read() if os.path.exists(path) else ""
    changed = add_trigger_entry(text)
    if changed is None:
        return False, "the file has a %s entry already, or is not a settings object; add \"%s\": %s to it by hand" % (SETTING, KERNEL_LANGUAGE, json.dumps(TRIGGER_CHARACTERS))
    kept = save_beside(path, ".pluss-backup") if text else None
    with open(path, "w", encoding="utf-8") as handle:
        handle.write(changed)
    if read_settings(open(path, encoding="utf-8").read()) is None:
        if kept:
            shutil.copy2(kept, path)
        return False, "the edited file could not be read back, so it was restored"
    return True, "added the setting%s" % (" (the original is kept as %s)" % os.path.basename(kept) if kept else "")


# ---- notebooks ------------------------------------------------------------------------------------------

def stale_cells(notebook):
    """How many cells carry a language tag an older kernel left, and whether the notebook's own kernel language is old."""
    cells = sum(1 for cell in notebook.get("cells", [])
                if (cell.get("metadata", {}).get("vscode", {}) or {}).get("languageId") in STALE_LANGUAGES)
    kernelspec = notebook.get("metadata", {}).get("kernelspec", {})
    old_spec = kernelspec.get("name") == KERNEL_NAME and kernelspec.get("language") in STALE_LANGUAGES
    return cells, old_spec


def clean_notebook(notebook):
    """The notebook without the stale cell tags and with the kernel language this kernel declares."""
    for cell in notebook.get("cells", []):
        meta = cell.get("metadata", {})
        tag = meta.get("vscode")
        if isinstance(tag, dict) and tag.get("languageId") in STALE_LANGUAGES:
            del tag["languageId"]
            if not tag:
                del meta["vscode"]
    metadata = notebook.get("metadata", {})
    if metadata.get("kernelspec", {}).get("name") == KERNEL_NAME:
        metadata["kernelspec"]["language"] = KERNEL_LANGUAGE
        if isinstance(metadata.get("language_info"), dict):
            metadata["language_info"]["name"] = KERNEL_LANGUAGE
    return notebook


def write_notebook(path, notebook):
    with open(path, "w", encoding="utf-8") as handle:
        json.dump(notebook, handle, indent=1, ensure_ascii=False)
        handle.write("\n")


# ---- the report -----------------------------------------------------------------------------------------

class Report:
    def __init__(self):
        self.problems = 0

    def ok(self, line):
        print("  [ok]   " + line)

    def note(self, line):
        print("  [..]   " + line)

    def bad(self, line, fix):
        print("  [FAIL] " + line)
        print("         " + fix)
        self.problems += 1


def check_kernel(report, prefix):
    try:
        from jupyter_client.kernelspec import KernelSpecManager, NoSuchKernel
    except ImportError:
        report.bad("Jupyter's Python packages are not installed for %s" % sys.executable,
                   "run  %s -m pip install ipykernel jupyter_client  and try again" % sys.executable)
        return None
    try:
        spec = KernelSpecManager().get_kernel_spec(KERNEL_NAME)
    except NoSuchKernel:
        report.bad("the PlussII C++ kernel is not installed", "run  pluss kernel install")
        return None
    report.ok("the PlussII C++ kernel is installed (%s)" % spec.resource_dir)
    env = spec.env or {}
    include = os.path.realpath(env.get("PLUSS_INCLUDE", ""))
    script = os.path.realpath(spec.argv[1]) if len(spec.argv) > 1 else ""
    mine = os.path.realpath(os.path.join(prefix, "include"))
    if include == mine and script.startswith(os.path.realpath(prefix) + os.sep):
        report.ok("it belongs to the download you are running (%s)" % prefix)
    else:
        report.bad("it belongs to another download: %s" % (os.path.dirname(include) or "unknown"),
                   "run  pluss kernel install  from the download you want to use (%s)" % prefix)
    if spec.language == KERNEL_LANGUAGE:
        report.ok("it declares the language '%s', the name editors ask for completions under" % KERNEL_LANGUAGE)
    else:
        report.bad("it declares the language '%s', so VS Code never asks it for completions" % spec.language,
                   "run  pluss kernel install  again (this download's installer declares '%s')" % KERNEL_LANGUAGE)
    clang = env.get("PLUSS_CLANG_REPL", "")
    if clang and os.path.exists(clang):
        sysroot = env.get("PLUSS_SYSROOT", "")
        if sysroot and not installer.sysroot_works(clang, sysroot):
            report.bad("clang-repl cannot read the SDK %s" % sysroot,
                       "run  pluss kernel install --sysroot <an older SDK folder>  (the newest SDK can be newer than this LLVM)")
        else:
            report.ok("clang-repl works%s" % (" with the SDK %s" % sysroot if sysroot else ""))
    else:
        report.bad("clang-repl was not found at %s" % (clang or "(not recorded)"),
                   "install LLVM (macOS: brew install llvm; Ubuntu: sudo apt install clang) and run  pluss kernel install")
    library = env.get("PLUSS_LIBRARY", "")
    if library and os.path.exists(library):
        report.ok("the shared library is built (%s)" % library)
    else:
        report.bad("the shared library is missing (%s)" % (library or "not recorded"), "run  pluss kernel install  to build it")
    return spec


def editors_missing_setting():
    """The editors (name, settings file) that are installed and do not yet have the setting."""
    missing = []
    for editor, path in settings_files():
        settings = read_settings(open(path, encoding="utf-8").read()) if os.path.exists(path) else {}
        if settings is not None and not has_trigger_entry(settings):
            missing.append((editor, path))
    return missing


def check_editors(report, fix):
    editors = settings_files()
    if not editors:
        report.note("VS Code was not found; for Jupyter or JupyterLab no editor setting is needed")
        return
    for editor, path in editors:
        text = open(path, encoding="utf-8").read() if os.path.exists(path) else ""
        settings = read_settings(text)
        if settings is None:
            report.bad("%s: %s could not be read" % (editor, path), "add \"%s\": { \"%s\": %s } to it by hand" % (SETTING, KERNEL_LANGUAGE, json.dumps(TRIGGER_CHARACTERS)))
        elif has_trigger_entry(settings):
            report.ok("%s asks the kernel for completions (%s has the '%s' entry)" % (editor, SETTING, KERNEL_LANGUAGE))
        elif fix:
            done, message = apply_setting(path)
            if done:
                report.ok("%s: %s; reload the window to use it" % (editor, message))
            else:
                report.bad("%s: %s" % (editor, message), "then reload the window")
        else:
            report.bad("%s does not ask the kernel for completions: %s has no '%s' entry" % (editor, SETTING, KERNEL_LANGUAGE),
                       "run  pluss kernel check --fix  to add it, then reload the window")


def check_notebooks(report, paths, fix):
    for path in paths:
        try:
            notebook = json.load(open(path, encoding="utf-8"))
        except (OSError, ValueError) as error:
            report.bad("%s could not be read (%s)" % (path, error), "check the path")
            continue
        cells, old_spec = stale_cells(notebook)
        if not cells and not old_spec:
            report.ok("%s has no language tags from an older kernel" % path)
        elif fix:
            kept = save_beside(path, ".pluss-backup")
            write_notebook(path, clean_notebook(notebook))
            report.ok("%s: removed %d stale cell tag(s) (the original is kept as %s)" % (path, cells, os.path.basename(kept)))
        else:
            report.bad("%s has %d cell(s) tagged with the language of an older kernel, and VS Code will not run them" % (path, cells),
                       "run  pluss kernel check --fix %s" % path)


def run_checks(prefix, fix=False, notebooks=()):
    """Prints the report and returns the number of problems."""
    report = Report()
    print("PlussII notebook kernel\n")
    check_kernel(report, prefix)
    check_editors(report, fix)
    check_notebooks(report, notebooks, fix)
    print("\n%s" % ("Everything looks right." if report.problems == 0 else "%d thing(s) to fix." % report.problems))
    return report.problems


def main():
    parser = argparse.ArgumentParser(description="Check the PlussII notebook kernel and the editor that talks to it.")
    parser.add_argument("--prefix", default=os.path.abspath(os.path.join(HERE, "..", "..", "..")), help="the extracted PlussII download")
    parser.add_argument("--fix", action="store_true", help="add the VS Code setting and clean the notebooks named (the originals are kept)")
    parser.add_argument("notebooks", nargs="*", help="notebooks to scan for tags an older kernel left in their cells")
    args = parser.parse_args()
    return 1 if run_checks(args.prefix, args.fix, args.notebooks) else 0


if __name__ == "__main__":
    sys.exit(main())

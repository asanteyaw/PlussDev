"""Checks for kernel_check.py that need neither Jupyter nor an editor: reading a settings file that has comments, adding the
setting without disturbing it, and cleaning the language tags an older kernel left in a notebook.

    python3 test_kernel_check.py
"""
import json
import os
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import kernel_check as kc

failures = []


def check(name, ok, detail=""):
    print("%-4s %s" % ("ok" if ok else "FAIL", name))
    if not ok:
        failures.append(name)
        print("     ", detail)


def main():
    # A settings file as editors write it: comments, trailing commas, a // inside a string.
    text = '{\n  // my theme\n  "workbench.colorTheme": "Dark", /* keep */\n  "url": "http://x//y",\n  "editor.fontSize": 14,\n}\n'
    settings = kc.read_settings(text)
    check("a settings file with comments and a trailing comma is read", settings is not None and settings["editor.fontSize"] == 14 and settings["url"] == "http://x//y", settings)

    added = kc.add_trigger_entry(text)
    again = kc.read_settings(added)
    check("the setting is added to a settings file", again is not None and kc.has_trigger_entry(again), added)
    check("the comments and the other settings stay", "// my theme" in added and "/* keep */" in added and again["editor.fontSize"] == 14, added)
    check("an entry that is there is not added twice", kc.add_trigger_entry(added) is None)

    empty = kc.add_trigger_entry("{}")
    check("an empty settings object gets the setting without a stray comma", kc.read_settings(empty) is not None and kc.has_trigger_entry(kc.read_settings(empty)), empty)
    check("a missing file gets a settings file", kc.has_trigger_entry(kc.read_settings(kc.add_trigger_entry(""))))
    check("text that is not JSON is left alone", kc.add_trigger_entry("not json") is None)
    other = '{ "%s": { "python": ["."] } }' % kc.SETTING
    check("another language's entry is not overwritten", kc.add_trigger_entry(other) is None and not kc.has_trigger_entry(kc.read_settings(other)))

    with tempfile.TemporaryDirectory() as folder:
        path = os.path.join(folder, "settings.json")
        open(path, "w").write(text)
        done, message = kc.apply_setting(path)
        check("the file is edited and the original kept", done and os.path.exists(path + ".pluss-backup") and open(path + ".pluss-backup").read() == text, message)

        old = {"cells": [{"cell_type": "code", "metadata": {"vscode": {"languageId": "c++"}}, "source": ["1"], "outputs": [], "execution_count": None},
                         {"cell_type": "code", "metadata": {}, "source": ["2"], "outputs": [], "execution_count": None}],
               "metadata": {"kernelspec": {"name": kc.KERNEL_NAME, "language": "c++", "display_name": "PlussII C++"}, "language_info": {"name": "c++"}},
               "nbformat": 4, "nbformat_minor": 5}
        cells, old_spec = kc.stale_cells(old)
        check("a notebook with an older kernel's tags is found", cells == 1 and old_spec, (cells, old_spec))
        clean = kc.clean_notebook(json.loads(json.dumps(old)))
        check("the tags are removed and the language updated",
              kc.stale_cells(clean) == (0, False) and clean["metadata"]["kernelspec"]["language"] == kc.KERNEL_LANGUAGE and "vscode" not in clean["cells"][0]["metadata"], clean)
        other_kernel = {"cells": [], "metadata": {"kernelspec": {"name": "python3", "language": "c++"}}}
        check("another kernel's notebook is not touched", kc.stale_cells(other_kernel) == (0, False) and kc.clean_notebook(other_kernel)["metadata"]["kernelspec"]["language"] == "c++")
        nb = os.path.join(folder, "n.ipynb")
        kc.write_notebook(nb, old)
        report = kc.Report()
        kc.check_notebooks(report, [nb], True)
        again = json.load(open(nb))
        check("--fix cleans the file and keeps the original", report.problems == 0 and kc.stale_cells(again) == (0, False) and os.path.exists(nb + ".pluss-backup"))
        report = kc.Report()
        kc.check_notebooks(report, [nb], False)
        check("a clean notebook passes", report.problems == 0)
        report = kc.Report()
        kc.write_notebook(nb, old)
        kc.check_notebooks(report, [nb], False)
        check("without --fix a stale notebook is reported and left as it was", report.problems == 1 and kc.stale_cells(json.load(open(nb))) == (1, True))
    print("%d check(s) failed" % len(failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())

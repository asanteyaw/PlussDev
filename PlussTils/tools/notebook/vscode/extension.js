// PlussII Notebook: underline the errors of a C++ cell while it is typed, without running it.
//
// For a notebook whose kernel is a PlussII kernel, each C++ cell is sent, with the cells above it, to cell_check.py, the
// checker that comes with that kernel (it uses the kernel's own compiler setup), and what it reports is shown as the
// editor's problems. One checker runs per kernel and stays up, so only the first check waits for the prelude to compile.
const vscode = require("vscode");
const childProcess = require("child_process");
const fs = require("fs");
const os = require("os");
const path = require("path");

const KERNEL_PROGRAM = "pluss_kernel.py";
const CHECKER = "cell_check.py";

const checkers = new Map();   // kernel.json path -> Checker
const timers = new Map();     // cell uri -> pending timer
let problems;

// The folders Jupyter keeps kernels in, for this user, the way Jupyter itself looks for them.
function kernelFolders() {
  const folders = [];
  if (process.env.JUPYTER_DATA_DIR) folders.push(path.join(process.env.JUPYTER_DATA_DIR, "kernels"));
  if (process.platform === "darwin") folders.push(path.join(os.homedir(), "Library", "Jupyter", "kernels"));
  else if (process.platform === "win32") folders.push(path.join(process.env.APPDATA || os.homedir(), "jupyter", "kernels"));
  else folders.push(path.join(process.env.XDG_DATA_HOME || path.join(os.homedir(), ".local", "share"), "jupyter", "kernels"));
  return folders;
}

// The notebook's kernel as Jupyter records it in the file.
function kernelName(notebook) {
  const metadata = notebook.metadata || {};
  const inner = metadata.metadata || (metadata.custom && metadata.custom.metadata) || metadata;
  return inner && inner.kernelspec ? inner.kernelspec.name : undefined;
}

// The kernel.json of a PlussII kernel by name, or undefined when the notebook's kernel is not one.
function plussKernelSpec(name) {
  if (!name) return undefined;
  for (const folder of kernelFolders()) {
    const file = path.join(folder, name, "kernel.json");
    try {
      const spec = JSON.parse(fs.readFileSync(file, "utf8"));
      if (Array.isArray(spec.argv) && spec.argv.length > 1 && path.basename(spec.argv[1]) === KERNEL_PROGRAM) {
        return { file, spec };
      }
    } catch (_) {
      // not here, or not readable: look in the next folder
    }
  }
  return undefined;
}

class Checker {
  constructor(spec) {
    const python = spec.argv[0];
    const program = path.join(path.dirname(spec.argv[1]), CHECKER);
    this.pending = new Map();
    this.nextId = 1;
    this.buffer = "";
    this.process = childProcess.spawn(python, [program], { env: Object.assign({}, process.env, spec.env || {}) });
    this.process.stdout.on("data", (chunk) => this.receive(chunk));
    this.process.on("exit", () => {
      for (const [, resolve] of this.pending) resolve([]);
      this.pending.clear();
      this.dead = true;
    });
  }

  receive(chunk) {
    this.buffer += chunk.toString();
    let end;
    while ((end = this.buffer.indexOf("\n")) >= 0) {
      const line = this.buffer.slice(0, end);
      this.buffer = this.buffer.slice(end + 1);
      let answer;
      try { answer = JSON.parse(line); } catch (_) { continue; }
      if (answer.id === null || answer.id === undefined) continue;  // the ready line, or a request it could not read
      const resolve = this.pending.get(answer.id);
      if (resolve) {
        this.pending.delete(answer.id);
        resolve(answer.diagnostics || []);
      }
    }
  }

  check(earlier, cell) {
    if (this.dead) return Promise.resolve([]);
    const id = this.nextId++;
    return new Promise((resolve) => {
      this.pending.set(id, resolve);
      this.process.stdin.write(JSON.stringify({ id, earlier, cell }) + "\n");
    });
  }

  dispose() {
    try { this.process.kill(); } catch (_) { /* already gone */ }
  }
}

function checkerFor(notebook) {
  const found = plussKernelSpec(kernelName(notebook));
  if (!found) return undefined;
  let checker = checkers.get(found.file);
  if (!checker || checker.dead) {
    checker = new Checker(found.spec);
    checkers.set(found.file, checker);
  }
  return checker;
}

function isCppCell(cell) {
  return cell.kind === vscode.NotebookCellKind.Code && cell.document.languageId === "cpp";
}

// The range a diagnostic underlines: the word at its position, or the rest of the line when there is none.
function rangeOf(document, line, column) {
  const row = Math.min(Math.max(line - 1, 0), Math.max(document.lineCount - 1, 0));
  const start = new vscode.Position(row, Math.max(column - 1, 0));
  const word = document.getWordRangeAtPosition(start);
  return word || new vscode.Range(start, document.lineAt(row).range.end);
}

async function checkCell(notebook, cell) {
  if (!isCppCell(cell)) return;
  const checker = checkerFor(notebook);
  if (!checker) return;
  const earlier = notebook.getCells().slice(0, cell.index).filter(isCppCell).map((c) => c.document.getText());
  const version = cell.document.version;
  const found = await checker.check(earlier, cell.document.getText());
  if (cell.document.version !== version) return;  // typed on since: a newer check is coming
  problems.set(cell.document.uri, found.map((d) => {
    const diagnostic = new vscode.Diagnostic(rangeOf(cell.document, d.line, d.column), d.message,
      d.severity === "warning" ? vscode.DiagnosticSeverity.Warning : vscode.DiagnosticSeverity.Error);
    diagnostic.source = "PlussII";
    return diagnostic;
  }));
}

function schedule(notebook, cell) {
  const key = cell.document.uri.toString();
  clearTimeout(timers.get(key));
  const delay = vscode.workspace.getConfiguration("plussNotebook").get("checkDelayMs", 500);
  timers.set(key, setTimeout(() => { timers.delete(key); checkCell(notebook, cell); }, delay));
}

// A cell's errors can depend on the cells above it, so a change is followed by a check of it and of the cells below.
function scheduleFrom(notebook, index) {
  for (const cell of notebook.getCells().slice(index)) {
    if (isCppCell(cell)) schedule(notebook, cell);
  }
}

function activate(context) {
  problems = vscode.languages.createDiagnosticCollection("PlussII");
  context.subscriptions.push(problems);
  context.subscriptions.push(vscode.workspace.onDidChangeTextDocument((event) => {
    for (const notebook of vscode.workspace.notebookDocuments) {
      const cell = notebook.getCells().find((c) => c.document === event.document);
      if (cell) scheduleFrom(notebook, cell.index);
    }
  }));
  context.subscriptions.push(vscode.workspace.onDidOpenNotebookDocument((notebook) => scheduleFrom(notebook, 0)));
  context.subscriptions.push(vscode.workspace.onDidChangeNotebookDocument((event) => {
    // A kernel chosen for the notebook (its metadata changed) or cells added or removed: check again.
    if (event.metadata || event.contentChanges.length > 0) scheduleFrom(event.notebook, 0);
  }));
  context.subscriptions.push(vscode.workspace.onDidCloseNotebookDocument((notebook) => {
    for (const cell of notebook.getCells()) problems.delete(cell.document.uri);
  }));
  for (const notebook of vscode.workspace.notebookDocuments) scheduleFrom(notebook, 0);
  context.subscriptions.push({ dispose: () => { for (const checker of checkers.values()) checker.dispose(); } });
}

function deactivate() {
  for (const checker of checkers.values()) checker.dispose();
}

module.exports = { activate, deactivate };

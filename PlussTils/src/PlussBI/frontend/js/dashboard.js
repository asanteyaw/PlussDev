// PlussBI dashboard client script.
//
// This file is the single source of truth for the dashboard's client-side
// behavior. It is embedded verbatim (byte-for-byte, no C++ string
// interpolation) into a generated C++ header at PlussBI's CMake configure
// step -- see ../../CMakeLists.txt and include/pluss/bi/dashboard_assets.h
// -- and is either inlined into a self-contained exported/shown document,
// or served as-is over HTTP at `/assets/dashboard.js` by `Studio::serve()`.
//
// It only depends on element ids/classes that both chrome modes
// (`DashboardChrome::None` and `DashboardChrome::Sidebar`) render:
// `#bi-content`, `#bi-session-chips`, `#bi-live-status`. Sidebar-only
// elements (`#bi-sidebar-tabs`, `.bi-tab`) simply don't exist in the
// chrome-less layout, so `plussBiShowBoard` is never invoked there -- no
// mode branching needed in this file.

var plussBiActiveBoard = null;

function plussBiShowBoard(key) {
  plussBiActiveBoard = key;
  document.querySelectorAll('#bi-content .bi-board').forEach(function (el) {
    el.hidden = el.getAttribute('data-board') !== key;
  });
  document.querySelectorAll('#bi-sidebar-tabs .bi-tab').forEach(function (el) {
    el.classList.toggle('active', el.getAttribute('data-board-tab') === key);
  });
}

(function () {
  var first = document.querySelector('#bi-sidebar-tabs .bi-tab');
  if (first) plussBiActiveBoard = first.getAttribute('data-board-tab');
})();

function plussBiApplyFilter(payload) {
  var status = document.getElementById('bi-live-status');
  if (status) status.textContent = 'Updating…';
  fetch('/api/filter', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(payload)
  }).then(function (resp) {
    if (!resp.ok) throw new Error('bad status');
    return resp.json();
  }).then(function (data) {
    document.getElementById('bi-content').innerHTML = data.content_html;
    document.getElementById('bi-session-chips').innerHTML = data.chips_html;
    if (plussBiActiveBoard) plussBiShowBoard(plussBiActiveBoard);
    if (status) status.textContent = 'Live';
  }).catch(function () {
    if (status) status.textContent = 'Static preview (start with Studio::serve() for live filters)';
  });
}

function plussBiSyncSelect(key) {
  var select = document.getElementById('select-' + key);
  if (!select) return;
  var value = select.value;
  plussBiApplyFilter(value ? { control: key, kind: 'category', value: value }
                            : { control: key, kind: 'clear' });
}

var plussBiDebounce = {};
function plussBiSyncRange(key) {
  clearTimeout(plussBiDebounce[key]);
  plussBiDebounce[key] = setTimeout(function () {
    var low = document.getElementById('low-' + key);
    var high = document.getElementById('high-' + key);
    if (!low || !high) return;
    plussBiApplyFilter({ control: key, kind: 'range', low: low.value, high: high.value });
  }, 350);
}

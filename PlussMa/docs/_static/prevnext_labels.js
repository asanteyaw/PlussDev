/* Shorten prev/next labels: "pluss::subsp::stats::ts::ema" -> "ema", "ts transforms" -> "transforms". */
(function () {
  "use strict";
  function shorten(t) {
    t = (t || "").trim();
    var i = t.lastIndexOf("::");
    if (i !== -1) t = t.slice(i + 2);
    return t.replace(/^ts\s+(?=\S)/, "");
  }
  function run() {
    var nodes = document.querySelectorAll("#pluss-prev-next .title");
    Array.prototype.forEach.call(nodes, function (n) { n.textContent = shorten(n.textContent); });
  }
  if (document.readyState === "loading") document.addEventListener("DOMContentLoaded", function () { setTimeout(run, 0); });
  else setTimeout(run, 0);
})();

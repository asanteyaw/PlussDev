#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

enum class Anim3SweepPlane : std::uint8_t {
  XY,
  XZ,
  YZ,
};

struct Animation3Options {
  std::string title{"PlussPl 3D Animation"};
  std::int64_t width{1280};
  std::int64_t height{820};

  Anim3SweepPlane sweep_plane{Anim3SweepPlane::YZ};
  double base_rot_xy_deg{0.0};
  double base_rot_xz_deg{12.0};
  double base_rot_yz_deg{24.0};
  double sweep_speed_deg_per_sec{18.0};

  double camera_distance{5.4};
  double camera_yaw_deg{30.0};
  double camera_pitch_deg{-14.0};
  double min_perspective_denom{0.15};
  double plot_padding_fraction{0.08};

  bool progressive_reveal{true};
  double reveal_points_per_sec{96.0};
  bool connect_path{true};
  double line_width{1.5};
  double point_radius{2.4};
  bool enable_snapshot{true};
  std::string snapshot_prefix{"plusspl_3d_frame"};
};

namespace detail {

[[nodiscard]] inline std::int64_t anim3_sweep_plane_index(Anim3SweepPlane plane) noexcept {
  switch (plane) {
    case Anim3SweepPlane::XY: return 0;
    case Anim3SweepPlane::XZ: return 1;
    case Anim3SweepPlane::YZ: return 2;
  }
  return 2;
}

[[nodiscard]] inline std::string anim3_js_numeric_array(const std::vector<double>& values) {
  std::ostringstream out;
  out << '[';
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i > 0) {
      out << ',';
    }
    out << std::fixed << std::setprecision(10) << values[i];
  }
  out << ']';
  return out.str();
}

[[nodiscard]] inline std::string anim3_js_string_literal(std::string_view value) {
  std::string out;
  out.reserve(value.size() + 8);
  for (const char c : value) {
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '\'': out += "\\'"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default: out.push_back(c); break;
    }
  }
  return out;
}

inline void normalize_xyz(
    std::vector<double>& x,
    std::vector<double>& y,
    std::vector<double>& z) {
  PLUSS_CHECK(
      x.size() == y.size() && x.size() == z.size(),
      "flash::normalize_xyz: x, y, z must have identical lengths");
  if (x.empty()) {
    return;
  }

  struct MinMax {
    bool any{false};
    double min_v{0.0};
    double max_v{0.0};
  };
  auto finite_minmax = [](const std::vector<double>& values) -> MinMax {
    double min_v = std::numeric_limits<double>::infinity();
    double max_v = -std::numeric_limits<double>::infinity();
    bool any = false;
    for (const double value : values) {
      if (!std::isfinite(value)) {
        continue;
      }
      min_v = std::min(min_v, value);
      max_v = std::max(max_v, value);
      any = true;
    }
    return {any, min_v, max_v};
  };

  const auto x_mm = finite_minmax(x);
  const auto y_mm = finite_minmax(y);
  const auto z_mm = finite_minmax(z);
  if (!x_mm.any || !y_mm.any || !z_mm.any) {
    return;
  }

  const double cx = 0.5 * (x_mm.min_v + x_mm.max_v);
  const double cy = 0.5 * (y_mm.min_v + y_mm.max_v);
  const double cz = 0.5 * (z_mm.min_v + z_mm.max_v);
  const double sx = x_mm.max_v - x_mm.min_v;
  const double sy = y_mm.max_v - y_mm.min_v;
  const double sz = z_mm.max_v - z_mm.min_v;
  const double scale = std::max({sx, sy, sz, 1e-9});
  const double inv_scale = 2.0 / scale;

  auto normalize_component = [inv_scale](std::vector<double>& values, double center) {
    std::transform(values.begin(), values.end(), values.begin(), [&](const double value) {
      if (!std::isfinite(value)) {
        return 0.0;
      }
      return (value - center) * inv_scale;
    });
  };
  normalize_component(x, cx);
  normalize_component(y, cy);
  normalize_component(z, cz);
}

[[nodiscard]] inline std::string build_animation3_html(
    const std::vector<double>& x,
    const std::vector<double>& y,
    const std::vector<double>& z,
    const Animation3Options& options) {
  std::ostringstream html;
  html << "<!doctype html><html><head><meta charset='utf-8'>\n";
  html << "<title>" << options.title << "</title>\n";
  html << "<style>"
          "body{margin:0;background:#f4f6f8;color:#10243f;font-family:system-ui,Segoe UI,sans-serif}"
          ".wrap{max-width:1280px;margin:0 auto;padding:14px 16px 20px}"
          "h2{margin:0 0 8px 0;font-size:22px;font-weight:700}"
          ".row{display:flex;align-items:center;gap:12px;flex-wrap:wrap;margin-bottom:10px}"
          "button{border:1px solid #9eb0c7;background:#fff;border-radius:8px;padding:6px 10px;cursor:pointer}"
          "label{display:flex;align-items:center;gap:8px;font-size:14px}"
          "input[type=range]{width:220px}"
          "#meta{font-size:14px;color:#2a405f}"
          "#view{display:block;width:100%;height:auto;background:#ffffff;border:1px solid #d3dce9;border-radius:10px}"
          "</style></head><body>\n";
  html << "<div class='wrap'>\n";
  html << "<h2>" << options.title << "</h2>\n";
  html << "<div class='row'>"
          "<button id='play'>Pause</button>";
  if (options.enable_snapshot) {
    html << "<button id='snap'>Save PNG</button>";
  }
  html << "<label>Speed <input id='speed' type='range' min='0.2' max='3.0' step='0.1' value='1.0'></label>"
          "<span id='meta'></span>"
          "</div>\n";
  html << "<canvas id='view' width='" << options.width << "' height='" << options.height << "'></canvas>\n";
  html << "</div>\n";
  html << "<script>\n";
  html << "const X3=" << anim3_js_numeric_array(x) << ";\n";
  html << "const Y3=" << anim3_js_numeric_array(y) << ";\n";
  html << "const Z3=" << anim3_js_numeric_array(z) << ";\n";
  html << "const cfg={"
          "sweepPlane:" << anim3_sweep_plane_index(options.sweep_plane) << ","
          "baseXY:" << options.base_rot_xy_deg << ","
          "baseXZ:" << options.base_rot_xz_deg << ","
          "baseYZ:" << options.base_rot_yz_deg << ","
          "sweepDegPerSec:" << options.sweep_speed_deg_per_sec << ","
          "camDist:" << options.camera_distance << ","
          "camYawDeg:" << options.camera_yaw_deg << ","
          "camPitchDeg:" << options.camera_pitch_deg << ","
          "minDenom:" << options.min_perspective_denom << ","
          "padFrac:" << options.plot_padding_fraction << ","
          "progressive:" << (options.progressive_reveal ? 1 : 0) << ","
          "revealPtsPerSec:" << options.reveal_points_per_sec << ","
          "connectPath:" << (options.connect_path ? 1 : 0) << ","
          "lineWidth:" << options.line_width << ","
          "pointRadius:" << options.point_radius << ","
          "snapshotPrefix:'" << anim3_js_string_literal(options.snapshot_prefix) << "'"
       << "};\n";
  html << "const deg2rad=(d)=>d*Math.PI/180.0;\n";
  html << "const rot2=(a,b,deg)=>{const r=deg2rad(deg),c=Math.cos(r),s=Math.sin(r);return [c*a-s*b,s*a+c*b];};\n";
  html << "const clamp=(v,lo,hi)=>Math.max(lo,Math.min(hi,v));\n";
  html << "const canvas=document.getElementById('view');\n";
  html << "const ctx=canvas.getContext('2d');\n";
  html << "const playBtn=document.getElementById('play');\n";
  html << "const speedCtrl=document.getElementById('speed');\n";
  html << "const snapBtn=document.getElementById('snap');\n";
  html << "const meta=document.getElementById('meta');\n";
  html << "let playing=true;\n";
  html << "let t0=null;\n";
  html << "playBtn.onclick=()=>{playing=!playing;playBtn.textContent=playing?'Pause':'Play';};\n";
  html << "const saveSnapshot=()=>{"
          "const a=document.createElement('a');"
          "a.href=canvas.toDataURL('image/png');"
          "a.download=`${cfg.snapshotPrefix}_${Date.now()}.png`;"
          "a.click();"
       "};\n";
  html << "if(snapBtn){snapBtn.onclick=saveSnapshot;}\n";
  html << "window.addEventListener('keydown',(e)=>{if((e.key==='s'||e.key==='S')&&!e.repeat){saveSnapshot();}});\n";
  html << "const applySweep=(rots,deg)=>{switch(cfg.sweepPlane){"
          "case 0: rots.xy=deg; break;"
          "case 1: rots.xz=deg; break;"
          "case 2: rots.yz=deg; break;}};\n";
  html << "const project=(x,y,z,elapsed)=>{"
          "const rots={xy:cfg.baseXY,xz:cfg.baseXZ,yz:cfg.baseYZ};"
          "applySweep(rots,elapsed*cfg.sweepDegPerSec*Number(speedCtrl.value));"
          "[x,y]=rot2(x,y,rots.xy);[x,z]=rot2(x,z,rots.xz);[y,z]=rot2(y,z,rots.yz);"
          "const yaw=deg2rad(cfg.camYawDeg),pitch=deg2rad(cfg.camPitchDeg);"
          "const cy=Math.cos(yaw),sy=Math.sin(yaw),cp=Math.cos(pitch),sp=Math.sin(pitch);"
          "const x1=cy*x+sy*z;const z1=-sy*x+cy*z;const y2=cp*y-sp*z1;const z2=sp*y+cp*z1;"
          "const d=Math.max(cfg.camDist-z2,cfg.minDenom);const s=cfg.camDist/d;"
          "return [x1*s,y2*s,z2,s];};\n";
  html << "const draw=(elapsed)=>{"
          "const width=canvas.width,height=canvas.height;"
          "ctx.clearRect(0,0,width,height);ctx.fillStyle='#ffffff';ctx.fillRect(0,0,width,height);"
          "const pad=Math.round(Math.min(width,height)*cfg.padFrac);"
          "const cx=width*0.5,cy=height*0.53,scale=Math.min(width,height)*0.34;"
          "const n=X3.length;"
          "const visible=cfg.progressive?Math.max(1,Math.min(n,Math.floor(elapsed*cfg.revealPtsPerSec))):n;"
          "const pts=[];"
          "for(let i=0;i<visible;i++){"
            "const p=project(X3[i],Y3[i],Z3[i],elapsed);"
            "const sx=cx+p[0]*scale,sy=cy-p[1]*scale;"
            "if(sx<pad||sx>width-pad||sy<pad||sy>height-pad) continue;"
            "pts.push({x:sx,y:sy,z:p[2],s:p[3]});"
          "}"
          "pts.sort((a,b)=>a.z-b.z);"
          "ctx.strokeStyle='#d5deea';ctx.lineWidth=1.0;ctx.strokeRect(pad,pad,width-2*pad,height-2*pad);"
          "if(cfg.connectPath&&pts.length>1){"
            "ctx.beginPath();ctx.strokeStyle='rgba(42,95,153,0.82)';ctx.lineWidth=Math.max(0.8,cfg.lineWidth);"
            "ctx.moveTo(pts[0].x,pts[0].y);"
            "for(let i=1;i<pts.length;i++){ctx.lineTo(pts[i].x,pts[i].y);}"
            "ctx.stroke();"
          "}"
          "for(const p of pts){"
            "const light=40.0+24.0*clamp((p.s-0.7)/1.6,0,1);"
            "const radius=Math.max(1.0,cfg.pointRadius*p.s);"
            "ctx.beginPath();ctx.fillStyle=`hsl(210,62%,${light}%)`;ctx.arc(p.x,p.y,radius,0,Math.PI*2);ctx.fill();"
          "}"
          "meta.textContent=`points ${pts.length}/${n}`;"
       "};\n";
  html << "const tick=(ts)=>{if(t0===null)t0=ts;const elapsed=(ts-t0)/1000.0;if(playing)draw(elapsed);requestAnimationFrame(tick);};\n";
  html << "draw(0);requestAnimationFrame(tick);\n";
  html << "</script></body></html>\n";
  return html.str();
}

} // namespace detail

template <class X, class Y, class Z>
bool animate3(
    X&& x,
    Y&& y,
    Z&& z,
    const Animation3Options& options = {}) {
  const Series sx = detail::to_series(std::forward<X>(x));
  const Series sy = detail::to_series(std::forward<Y>(y));
  const Series sz = detail::to_series(std::forward<Z>(z));
  detail::require_rank_at_most(sx, 3, "flash::animate3");
  detail::require_rank_at_most(sy, 3, "flash::animate3");
  detail::require_rank_at_most(sz, 3, "flash::animate3");
  PLUSS_CHECK(
      sx.numel() == sy.numel() && sx.numel() == sz.numel(),
      "flash::animate3: x, y, z must have identical lengths");
  PLUSS_CHECK(options.width >= 640 && options.height >= 420, "flash::animate3: window size too small");
  PLUSS_CHECK(options.camera_distance > 0.0, "flash::animate3: camera_distance must be positive");
  PLUSS_CHECK(options.min_perspective_denom > 0.0, "flash::animate3: min_perspective_denom must be positive");
  PLUSS_CHECK(options.plot_padding_fraction >= 0.0 && options.plot_padding_fraction < 0.45, "flash::animate3: invalid plot_padding_fraction");
  PLUSS_CHECK(options.reveal_points_per_sec > 0.0, "flash::animate3: reveal_points_per_sec must be positive");
  PLUSS_CHECK(options.point_radius > 0.0, "flash::animate3: point_radius must be positive");
  PLUSS_CHECK(options.line_width > 0.0, "flash::animate3: line_width must be positive");

  std::vector<double> x3 = detail::to_f64_vector(detail::flatten_f64(sx));
  std::vector<double> y3 = detail::to_f64_vector(detail::flatten_f64(sy));
  std::vector<double> z3 = detail::to_f64_vector(detail::flatten_f64(sz));
  detail::normalize_xyz(x3, y3, z3);

  const std::string html = detail::build_animation3_html(
      x3,
      y3,
      z3,
      options);
  ::pluss::view::HtmlWindowOptions window_options{};
  window_options.title = options.title;
  window_options.width = options.width;
  window_options.height = options.height;
  return ::pluss::view::show_html_window(html, window_options);
}

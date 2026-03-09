#pragma once

#include "pluss/pl/flash/detail/hyper/hyper4d_math.h"

#include <cstdint>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

enum class HyperProjection : std::uint8_t {
  Orthographic,
  PerspectiveW,
};

struct HyperPlotOptions {
  HyperProjection projection{HyperProjection::Orthographic};
  double rot_xy_deg{0.0};
  double rot_xz_deg{0.0};
  double rot_xw_deg{0.0};
  double rot_yz_deg{0.0};
  double rot_yw_deg{0.0};
  double rot_zw_deg{0.0};
  double w_perspective_distance{3.0};
  std::optional<Range1d> w_slice{};
  double point_size{8.0};
};

enum class HyperSweepPlane : std::uint8_t {
  XY,
  XZ,
  XW,
  YZ,
  YW,
  ZW,
};

struct HyperAnimationOptions {
  std::int64_t frames{96};
  HyperSweepPlane plane{HyperSweepPlane::XW};
  double start_deg{0.0};
  double end_deg{360.0};
  bool include_endpoint{false};
};

[[nodiscard]] inline std::vector<HyperPlotOptions> hyper_animation_sequence(
    const HyperPlotOptions& base,
    const HyperAnimationOptions& options = {}) {
  PLUSS_CHECK(options.frames > 1, "flash::hyper_animation_sequence: frames must be > 1");
  std::vector<HyperPlotOptions> out;
  out.reserve(static_cast<std::size_t>(options.frames));
  const std::int64_t denom = options.include_endpoint ? (options.frames - 1) : options.frames;
  const double step = (options.end_deg - options.start_deg) / static_cast<double>(std::max<std::int64_t>(denom, 1));
  for (const auto i : std::views::iota(std::int64_t{0}, options.frames)) {
    const double angle = options.start_deg + step * static_cast<double>(i);
    HyperPlotOptions f = base;
    switch (options.plane) {
      case HyperSweepPlane::XY: f.rot_xy_deg = angle; break;
      case HyperSweepPlane::XZ: f.rot_xz_deg = angle; break;
      case HyperSweepPlane::XW: f.rot_xw_deg = angle; break;
      case HyperSweepPlane::YZ: f.rot_yz_deg = angle; break;
      case HyperSweepPlane::YW: f.rot_yw_deg = angle; break;
      case HyperSweepPlane::ZW: f.rot_zw_deg = angle; break;
    }
    out.push_back(f);
  }
  return out;
}

enum class HyperSimSweepPlane : std::uint8_t {
  XY,
  XZ,
  XW,
  YZ,
  YW,
  ZW,
};

struct HyperSimulationOptions {
  std::string title{"PlussPl 4D Simulation"};
  std::int64_t width{1280};
  std::int64_t height{860};

  HyperProjection projection{HyperProjection::PerspectiveW};
  HyperSimSweepPlane sweep_plane{HyperSimSweepPlane::XW};

  double base_rot_xy_deg{8.0};
  double base_rot_xz_deg{0.0};
  double base_rot_xw_deg{0.0};
  double base_rot_yz_deg{15.0};
  double base_rot_yw_deg{0.0};
  double base_rot_zw_deg{24.0};
  double sweep_speed_deg_per_sec{22.0};

  double w_perspective_distance{3.6};
  double slice_min{0.35};
  double slice_max{1.0};
  double slice_default{0.82};

  double camera_distance{5.6};
  double camera_yaw_deg{28.0};
  double camera_pitch_deg{-16.0};

  double min_perspective_denom{0.15};
  double plot_padding_fraction{0.08};
  double point_radius_base{2.1};
  double point_radius_min{1.0};
  double point_radius_max{5.0};
};

namespace detail {

[[nodiscard]] inline HyperProjectionKind to_hyper_projection_kind(HyperProjection projection) noexcept {
  return projection == HyperProjection::PerspectiveW ? HyperProjectionKind::PerspectiveW
                                                     : HyperProjectionKind::Orthographic;
}

[[nodiscard]] inline std::vector<HyperProjectedPoint> project4_vectors(
    const Series& x,
    const Series& y,
    const Series& z,
    const Series& w,
    const HyperPlotOptions& options,
    std::string_view where) {
  const Series fx = flatten_f64(x);
  const Series fy = flatten_f64(y);
  const Series fz = flatten_f64(z);
  const Series fw = flatten_f64(w);
  PLUSS_CHECK(
      fx.numel() == fy.numel() && fx.numel() == fz.numel() && fx.numel() == fw.numel(),
      std::string(where) + ": x, y, z, w must have identical lengths");

  return project4_batch(
      to_f64_vector(fx),
      to_f64_vector(fy),
      to_f64_vector(fz),
      to_f64_vector(fw),
      options.rot_xy_deg,
      options.rot_xz_deg,
      options.rot_xw_deg,
      options.rot_yz_deg,
      options.rot_yw_deg,
      options.rot_zw_deg,
      to_hyper_projection_kind(options.projection),
      options.w_perspective_distance,
      options.w_slice);
}

[[nodiscard]] inline std::string js_numeric_array(const std::vector<double>& values) {
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

[[nodiscard]] inline std::int64_t sweep_plane_index(HyperSimSweepPlane plane) noexcept {
  switch (plane) {
    case HyperSimSweepPlane::XY: return 0;
    case HyperSimSweepPlane::XZ: return 1;
    case HyperSimSweepPlane::XW: return 2;
    case HyperSimSweepPlane::YZ: return 3;
    case HyperSimSweepPlane::YW: return 4;
    case HyperSimSweepPlane::ZW: return 5;
  }
  return 2;
}

[[nodiscard]] inline std::string build_hyper_simulation_html(
    const std::vector<double>& x4,
    const std::vector<double>& y4,
    const std::vector<double>& z4,
    const std::vector<double>& w4,
    const HyperSimulationOptions& options) {
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
          "<button id='play'>Pause</button>"
          "<label>Speed <input id='speed' type='range' min='0.2' max='3.0' step='0.1' value='1.0'></label>"
          "<label>Slice <input id='slice' type='range' min='"
       << options.slice_min << "' max='" << options.slice_max << "' step='0.01' value='" << options.slice_default
       << "'></label>"
          "<span id='meta'></span>"
          "</div>\n";
  html << "<canvas id='view' width='" << options.width << "' height='" << options.height << "'></canvas>\n";
  html << "</div>\n";
  html << "<script>\n";
  html << "const X4=" << js_numeric_array(x4) << ";\n";
  html << "const Y4=" << js_numeric_array(y4) << ";\n";
  html << "const Z4=" << js_numeric_array(z4) << ";\n";
  html << "const W4=" << js_numeric_array(w4) << ";\n";
  html << "const cfg={"
          "projection:" << (options.projection == HyperProjection::PerspectiveW ? 1 : 0) << ","
          "sweepPlane:" << sweep_plane_index(options.sweep_plane) << ","
          "baseXY:" << options.base_rot_xy_deg << ","
          "baseXZ:" << options.base_rot_xz_deg << ","
          "baseXW:" << options.base_rot_xw_deg << ","
          "baseYZ:" << options.base_rot_yz_deg << ","
          "baseYW:" << options.base_rot_yw_deg << ","
          "baseZW:" << options.base_rot_zw_deg << ","
          "speedDegPerSec:" << options.sweep_speed_deg_per_sec << ","
          "wPerspectiveDistance:" << options.w_perspective_distance << ","
          "sliceMin:" << options.slice_min << ","
          "sliceMax:" << options.slice_max << ","
          "cameraDistance:" << options.camera_distance << ","
          "cameraYawDeg:" << options.camera_yaw_deg << ","
          "cameraPitchDeg:" << options.camera_pitch_deg << ","
          "minPerspectiveDenom:" << options.min_perspective_denom << ","
          "plotPadFrac:" << options.plot_padding_fraction << ","
          "pointRadiusBase:" << options.point_radius_base << ","
          "pointRadiusMin:" << options.point_radius_min << ","
          "pointRadiusMax:" << options.point_radius_max
       << "};\n";
  html << "const deg2rad=(d)=>d*Math.PI/180.0;\n";
  html << "const rot2=(a,b,deg)=>{const r=deg2rad(deg),c=Math.cos(r),s=Math.sin(r);return [c*a-s*b,s*a+c*b];};\n";
  html << "const clamp=(v,lo,hi)=>Math.max(lo,Math.min(hi,v));\n";
  html << "const canvas=document.getElementById('view');\n";
  html << "const ctx=canvas.getContext('2d');\n";
  html << "const playBtn=document.getElementById('play');\n";
  html << "const speedCtrl=document.getElementById('speed');\n";
  html << "const sliceCtrl=document.getElementById('slice');\n";
  html << "const meta=document.getElementById('meta');\n";
  html << "let playing=true;\n";
  html << "let t0=null;\n";
  html << "playBtn.onclick=()=>{playing=!playing;playBtn.textContent=playing?'Pause':'Play';};\n";
  html << "const applySweep=(rots,deg)=>{switch(cfg.sweepPlane){"
          "case 0: rots.xy=deg; break;"
          "case 1: rots.xz=deg; break;"
          "case 2: rots.xw=deg; break;"
          "case 3: rots.yz=deg; break;"
          "case 4: rots.yw=deg; break;"
          "case 5: rots.zw=deg; break;}};\n";
  html << "const project4=(x,y,z,w,elapsed)=>{"
          "const rots={xy:cfg.baseXY,xz:cfg.baseXZ,xw:cfg.baseXW,yz:cfg.baseYZ,yw:cfg.baseYW,zw:cfg.baseZW};"
          "applySweep(rots,elapsed*cfg.speedDegPerSec*Number(speedCtrl.value));"
          "[x,y]=rot2(x,y,rots.xy);[x,z]=rot2(x,z,rots.xz);[x,w]=rot2(x,w,rots.xw);"
          "[y,z]=rot2(y,z,rots.yz);[y,w]=rot2(y,w,rots.yw);[z,w]=rot2(z,w,rots.zw);"
          "if(cfg.projection===0) return [x,y,z,w];"
          "const d=Math.max(cfg.wPerspectiveDistance-w,cfg.minPerspectiveDenom);"
          "const s=cfg.wPerspectiveDistance/d;return [x*s,y*s,z*s,w];};\n";
  html << "const project3=(x,y,z)=>{"
          "const yaw=deg2rad(cfg.cameraYawDeg),pitch=deg2rad(cfg.cameraPitchDeg);"
          "const cy=Math.cos(yaw),sy=Math.sin(yaw),cp=Math.cos(pitch),sp=Math.sin(pitch);"
          "const x1=cy*x+sy*z;const z1=-sy*x+cy*z;const y2=cp*y-sp*z1;const z2=sp*y+cp*z1;"
          "const d=Math.max(cfg.cameraDistance-z2,cfg.minPerspectiveDenom);const s=cfg.cameraDistance/d;"
          "return [x1*s,y2*s,z2,s];};\n";
  html << "const draw=(elapsed)=>{"
          "const width=canvas.width,height=canvas.height;"
          "ctx.clearRect(0,0,width,height);ctx.fillStyle='#ffffff';ctx.fillRect(0,0,width,height);"
          "const pad=Math.round(Math.min(width,height)*cfg.plotPadFrac);"
          "const cx=width*0.5,cy=height*0.53,scale=Math.min(width,height)*0.33;"
          "const slice=Math.max(cfg.sliceMin,Number(sliceCtrl.value));"
          "const pts=[];"
          "for(let i=0;i<X4.length;i++){"
            "const p4=project4(X4[i],Y4[i],Z4[i],W4[i],elapsed);"
            "if(p4[3]<-slice||p4[3]>slice) continue;"
            "const p3=project3(p4[0],p4[1],p4[2]);"
            "const sx=cx+p3[0]*scale,sy=cy-p3[1]*scale;"
            "if(sx<pad||sx>width-pad||sy<pad||sy>height-pad) continue;"
            "pts.push({x:sx,y:sy,z:p3[2],w:p4[3],p:p3[3]});"
          "}"
          "pts.sort((a,b)=>a.z-b.z);"
          "ctx.strokeStyle='#d5deea';ctx.lineWidth=1.0;ctx.strokeRect(pad,pad,width-2*pad,height-2*pad);"
          "for(const p of pts){"
            "const hue=210.0+55.0*((p.w+slice)/(2.0*slice));"
            "const light=42.0+26.0*clamp((p.p-0.7)/1.6,0,1);"
            "const radius=clamp(cfg.pointRadiusBase*p.p,cfg.pointRadiusMin,cfg.pointRadiusMax);"
            "ctx.beginPath();ctx.fillStyle=`hsl(${hue},64%,${light}%)`;ctx.arc(p.x,p.y,radius,0,Math.PI*2);ctx.fill();"
          "}"
          "meta.textContent=`points ${pts.length}/${X4.length}`;"
       "};\n";
  html << "const tick=(ts)=>{if(t0===null)t0=ts;const elapsed=(ts-t0)/1000.0;if(playing)draw(elapsed);requestAnimationFrame(tick);};\n";
  html << "draw(0);requestAnimationFrame(tick);\n";
  html << "</script></body></html>\n";
  return html.str();
}

} // namespace detail

template <class X, class Y, class Z, class W>
Trace& path4(
    Pane& pane,
    X&& x,
    Y&& y,
    Z&& z,
    W&& w,
    const HyperPlotOptions& options = {}) {
  const Series sx = detail::to_series(std::forward<X>(x));
  const Series sy = detail::to_series(std::forward<Y>(y));
  const Series sz = detail::to_series(std::forward<Z>(z));
  const Series sw = detail::to_series(std::forward<W>(w));
  detail::require_rank_at_most(sx, 3, "flash::path4");
  detail::require_rank_at_most(sy, 3, "flash::path4");
  detail::require_rank_at_most(sz, 3, "flash::path4");
  detail::require_rank_at_most(sw, 3, "flash::path4");

  const std::vector<HyperProjectedPoint> projected =
      detail::project4_vectors(sx, sy, sz, sw, options, "flash::path4");
  PLUSS_CHECK(!projected.empty(), "flash::path4: no points remain after projection/slice");

  std::vector<double> px;
  std::vector<double> py;
  std::vector<double> pz;
  px.reserve(projected.size());
  py.reserve(projected.size());
  pz.reserve(projected.size());
  for (const auto& point : projected) {
    px.push_back(point.x);
    py.push_back(point.y);
    pz.push_back(point.z);
  }
  return pane.path3(::pluss::make_series(px), ::pluss::make_series(py), ::pluss::make_series(pz));
}

template <class X, class Y, class Z, class W>
Trace& path4(
    Studio& studio,
    X&& x,
    Y&& y,
    Z&& z,
    W&& w,
    const HyperPlotOptions& options = {},
    std::string title = {}) {
  Pane& pane = studio.pane3(std::move(title));
  return path4(
      pane,
      std::forward<X>(x),
      std::forward<Y>(y),
      std::forward<Z>(z),
      std::forward<W>(w),
      options);
}

template <class X, class Y, class Z, class W>
Trace& cloud4(
    Pane& pane,
    X&& x,
    Y&& y,
    Z&& z,
    W&& w,
    const HyperPlotOptions& options = {}) {
  const Series sx = detail::to_series(std::forward<X>(x));
  const Series sy = detail::to_series(std::forward<Y>(y));
  const Series sz = detail::to_series(std::forward<Z>(z));
  const Series sw = detail::to_series(std::forward<W>(w));
  detail::require_rank_at_most(sx, 3, "flash::cloud4");
  detail::require_rank_at_most(sy, 3, "flash::cloud4");
  detail::require_rank_at_most(sz, 3, "flash::cloud4");
  detail::require_rank_at_most(sw, 3, "flash::cloud4");

  const std::vector<HyperProjectedPoint> projected =
      detail::project4_vectors(sx, sy, sz, sw, options, "flash::cloud4");
  PLUSS_CHECK(!projected.empty(), "flash::cloud4: no points remain after projection/slice");

  std::vector<double> px;
  std::vector<double> py;
  std::vector<double> pz;
  px.reserve(projected.size());
  py.reserve(projected.size());
  pz.reserve(projected.size());
  for (const auto& point : projected) {
    px.push_back(point.x);
    py.push_back(point.y);
    pz.push_back(point.z);
  }
  const double size = std::max(1.0, options.point_size);
  return pane.cloud3(
      ::pluss::make_series(px),
      ::pluss::make_series(py),
      ::pluss::make_series(pz),
      ::pluss::full({static_cast<std::int64_t>(projected.size())}, size, ::pluss::ScalarType::Float64));
}

template <class X, class Y, class Z, class W>
Trace& cloud4(
    Studio& studio,
    X&& x,
    Y&& y,
    Z&& z,
    W&& w,
    const HyperPlotOptions& options = {},
    std::string title = {}) {
  Pane& pane = studio.pane3(std::move(title));
  return cloud4(
      pane,
      std::forward<X>(x),
      std::forward<Y>(y),
      std::forward<Z>(z),
      std::forward<W>(w),
      options);
}

template <class X, class Y, class Z, class W>
bool simulate4(
    X&& x,
    Y&& y,
    Z&& z,
    W&& w,
    const HyperSimulationOptions& options = {}) {
  const Series sx = detail::to_series(std::forward<X>(x));
  const Series sy = detail::to_series(std::forward<Y>(y));
  const Series sz = detail::to_series(std::forward<Z>(z));
  const Series sw = detail::to_series(std::forward<W>(w));
  detail::require_rank_at_most(sx, 3, "flash::simulate4");
  detail::require_rank_at_most(sy, 3, "flash::simulate4");
  detail::require_rank_at_most(sz, 3, "flash::simulate4");
  detail::require_rank_at_most(sw, 3, "flash::simulate4");
  PLUSS_CHECK(
      sx.numel() == sy.numel() && sx.numel() == sz.numel() && sx.numel() == sw.numel(),
      "flash::simulate4: x, y, z, w must have identical lengths");
  PLUSS_CHECK(options.width >= 640 && options.height >= 420, "flash::simulate4: window size too small");
  PLUSS_CHECK(
      options.slice_min > 0.0 && options.slice_max >= options.slice_min && options.slice_default >= options.slice_min &&
          options.slice_default <= options.slice_max,
      "flash::simulate4: invalid slice range/default");
  PLUSS_CHECK(options.w_perspective_distance > 0.0, "flash::simulate4: w_perspective_distance must be positive");
  PLUSS_CHECK(options.camera_distance > 0.0, "flash::simulate4: camera_distance must be positive");
  PLUSS_CHECK(options.min_perspective_denom > 0.0, "flash::simulate4: min_perspective_denom must be positive");
  PLUSS_CHECK(options.plot_padding_fraction >= 0.0 && options.plot_padding_fraction < 0.45, "flash::simulate4: invalid plot_padding_fraction");
  PLUSS_CHECK(
      options.point_radius_min > 0.0 && options.point_radius_max >= options.point_radius_min,
      "flash::simulate4: invalid point radius range");

  const std::string html = detail::build_hyper_simulation_html(
      detail::to_f64_vector(detail::flatten_f64(sx)),
      detail::to_f64_vector(detail::flatten_f64(sy)),
      detail::to_f64_vector(detail::flatten_f64(sz)),
      detail::to_f64_vector(detail::flatten_f64(sw)),
      options);

  ::pluss::view::HtmlWindowOptions window_options{};
  window_options.title = options.title;
  window_options.width = options.width;
  window_options.height = options.height;
  return ::pluss::view::show_html_window(html, window_options);
}

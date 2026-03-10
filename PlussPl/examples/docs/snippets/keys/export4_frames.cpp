    const auto t = linspace(0.0, 2.0 * kPi, 360, ScalarType::Float64);
    const auto hx = t.cos();
    const auto hy = t.sin();
    const auto hz = t.mul(2.0).cos().mul(0.72);
    const auto hw = t.mul(3.0).sin().mul(0.58);

    const auto out_dir = out_svg.parent_path() / "export4_frames_tmp";
    HyperFrameExportOptions opts;
    opts.output_dir = out_dir;
    opts.file_prefix = "frame";
    opts.format = AnimationFrameFormat::Svg;
    opts.width = 960;
    opts.height = 620;
    opts.sweep.frames = 3;
    opts.sweep.plane = HyperSweepPlane::XW;
    opts.sweep.start_deg = 0.0;
    opts.sweep.end_deg = 45.0;
    opts.sweep.include_endpoint = true;
    const auto n = export4_frames(hx, hy, hz, hw, opts);
    if (n <= 0) {
      return false;
    }
    const auto frame0 = out_dir / "frame_0.svg";
    if (!std::filesystem::exists(frame0)) {
      return false;
    }
    std::error_code ec;
    std::filesystem::copy_file(frame0, out_svg, std::filesystem::copy_options::overwrite_existing, ec);
    return !ec;

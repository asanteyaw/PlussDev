    Studio fig("core_save_pdf", Theme::paper());
    auto& pane = fig.pane2("save_pdf");
    pane.trend(x, y).label("signal");
    const auto pdf = out_svg.parent_path() / "core_save_pdf_export.pdf";
    fig.save_pdf(pdf.string());
    save(fig);

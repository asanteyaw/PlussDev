#!/usr/bin/env python3
"""
Modernized Bike Sharing (hour.csv) EDA

This script is a modern, runnable .py equivalent of the original 2017-ish notebook.
It:
- loads hour.csv
- cleans / renames columns
- creates useful time features + categorical labels
- prints summary tables
- generates EDA plots (saved to an output folder, and optionally shown)

Dataset: UCI Bike Sharing Dataset (hour.csv)
"""

from __future__ import annotations

import argparse
from pathlib import Path
import warnings

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

# Optional (time series decomposition / forecasting). If not installed, we skip gracefully.
try:
    from statsmodels.tsa.seasonal import STL
    _HAS_STATSMODELS = True
except Exception:
    _HAS_STATSMODELS = False

try:
    from statsmodels.tsa.statespace.sarimax import SARIMAX
    _HAS_SARIMAX = True
except Exception:
    _HAS_SARIMAX = False


SEASON_MAP = {1: "spring", 2: "summer", 3: "fall", 4: "winter"}
WEATHER_MAP = {
    1: "Clear/Few clouds",
    2: "Mist/Cloudy",
    3: "Light Snow/Rain",
    4: "Heavy Rain/Snow/Fog",
}
WEEKDAY_MAP = {0: "Sunday", 1: "Monday", 2: "Tuesday", 3: "Wednesday", 4: "Thursday", 5: "Friday", 6: "Saturday"}
YEAR_MAP = {0: "2011", 1: "2012"}


def _ensure_output_dir(out_dir: Path) -> Path:
    out_dir.mkdir(parents=True, exist_ok=True)
    return out_dir


def savefig(out_dir: Path, name: str, fig: plt.Figure, *, dpi: int = 160) -> Path:
    out_path = out_dir / name
    fig.tight_layout()
    fig.savefig(out_path, dpi=dpi, bbox_inches="tight")
    return out_path


def load_hour_csv(path: Path) -> pd.DataFrame:
    if not path.exists():
        raise FileNotFoundError(
            f"Could not find {path}. Put hour.csv next to this script or pass --data /path/to/hour.csv"
        )

    # Parse date column up-front (more modern)
    df = pd.read_csv(path, parse_dates=["dteday"], low_memory=False)

    # Normalize column names
    rename = {
        "instant": "rec_id",
        "dteday": "date",
        "holiday": "is_holiday",
        "workingday": "is_workingday",
        "weathersit": "weather_condition",
        "hum": "humidity",
        "mnth": "month",
        "cnt": "total_count",
        "hr": "hour",
        "yr": "year",
    }
    df = df.rename(columns=rename)

    # Build a true timestamp (hourly)
    # hour.csv includes 'date' (day) and 'hour' (0-23)
    df["datetime"] = df["date"] + pd.to_timedelta(df["hour"], unit="h")

    # Categorical labels (more readable for plots)
    df["season"] = df["season"].map(SEASON_MAP).astype("category")
    df["weather_condition"] = df["weather_condition"].map(WEATHER_MAP).astype("category")
    df["weekday_name"] = df["weekday"].map(WEEKDAY_MAP).astype("category")
    df["year_label"] = df["year"].map(YEAR_MAP).astype("category")

    # Cast flags to boolean-like category (keeps seaborn happy)
    for c in ["is_holiday", "is_workingday"]:
        df[c] = df[c].astype("int64").astype("category")

    # Add some helpful derived features
    df["month_name"] = df["datetime"].dt.month_name().str[:3].astype("category")
    df["day_name"] = df["datetime"].dt.day_name().astype("category")
    df["is_weekend"] = df["weekday"].isin([0, 6]).astype("int64").astype("category")

    return df


def print_overview(df: pd.DataFrame) -> None:
    print("\n=== Dataset shape ===")
    print(df.shape)

    print("\n=== Head (first 5 rows) ===")
    print(df.head())

    print("\n=== dtypes ===")
    print(df.dtypes)

    print("\n=== info ===")
    # pandas expects a file-like object for buf on modern versions
    import io
    buf = io.StringIO()
    df.info(buf=buf)
    print(buf.getvalue())

    print("\n=== describe (numeric) ===")
    desc = df.select_dtypes(include=[np.number]).describe().T
    print(desc)

    print("\n=== Missing values (top) ===")
    na = df.isna().sum().sort_values(ascending=False)
    print(na[na > 0].head(20) if (na > 0).any() else "No missing values detected.")


def plot_season_hourly(df: pd.DataFrame, out_dir: Path, show: bool) -> None:
    # Use mean to reduce overplotting (modern approach)
    agg = df.groupby(["season", "hour"], observed=True)["total_count"].mean().reset_index()

    g = sns.relplot(
        data=agg,
        x="hour",
        y="total_count",
        col="season",
        col_wrap=2,
        kind="line",
        height=3.5,
        aspect=1.4,
        facet_kws=dict(sharey=True, sharex=True),
    )
    g.set_axis_labels("Hour of day", "Mean total rentals")
    g.set_titles("{col_name}")
    g.fig.suptitle("Mean hourly bike rentals by season", y=1.02)
    path = savefig(out_dir, "01_mean_hourly_by_season.png", g.fig)
    print(f"Saved: {path}")
    if show:
        plt.show()
    plt.close(g.fig)


def plot_weekday_hourly(df: pd.DataFrame, out_dir: Path, show: bool) -> None:
    agg = df.groupby(["weekday_name", "hour"], observed=True)["total_count"].mean().reset_index()

    fig, ax = plt.subplots(figsize=(12, 5))
    sns.lineplot(data=agg, x="hour", y="total_count", hue="weekday_name", ax=ax)
    ax.set_title("Mean hourly bike rentals by weekday")
    ax.set_xlabel("Hour of day")
    ax.set_ylabel("Mean total rentals")
    ax.legend(title="Weekday", ncol=2, fontsize=9)
    path = savefig(out_dir, "02_mean_hourly_by_weekday.png", fig)
    print(f"Saved: {path}")
    if show:
        plt.show()
    plt.close(fig)

    # Registered vs casual by weekday (mean)
    agg2 = df.groupby(["weekday_name", "hour"], observed=True)[["registered", "casual"]].mean().reset_index()
    fig, axes = plt.subplots(ncols=2, figsize=(14, 5), sharey=True)

    sns.lineplot(data=agg2, x="hour", y="registered", hue="weekday_name", ax=axes[0], legend=False)
    axes[0].set_title("Registered: mean hourly rentals")
    axes[0].set_xlabel("Hour")
    axes[0].set_ylabel("Mean rentals")

    sns.lineplot(data=agg2, x="hour", y="casual", hue="weekday_name", ax=axes[1])
    axes[1].set_title("Casual: mean hourly rentals")
    axes[1].set_xlabel("Hour")
    axes[1].set_ylabel("")
    axes[1].legend(title="Weekday", ncol=2, fontsize=9)

    path = savefig(out_dir, "03_registered_vs_casual_by_weekday.png", fig)
    print(f"Saved: {path}")
    if show:
        plt.show()
    plt.close(fig)


def plot_time_series(df: pd.DataFrame, out_dir: Path, show: bool) -> None:
    # Hourly time series (can be a lot of points; downsample to daily mean for readability)
    daily = df.set_index("datetime").resample("D")["total_count"].mean()

    fig, ax = plt.subplots(figsize=(14, 4))
    ax.plot(daily.index, daily.values)
    ax.set_title("Daily mean bike rentals (from hourly data)")
    ax.set_xlabel("Date")
    ax.set_ylabel("Mean total rentals per hour (daily mean)")
    path = savefig(out_dir, "04_daily_mean_time_series.png", fig)
    print(f"Saved: {path}")
    if show:
        plt.show()
    plt.close(fig)

    # STL decomposition (modern, robust)
    if _HAS_STATSMODELS:
        with warnings.catch_warnings():
            warnings.simplefilter("ignore")
            stl = STL(daily.asfreq("D").interpolate(), period=365, robust=True)
            res = stl.fit()

        fig = res.plot()
        fig.set_size_inches(14, 6)
        path = savefig(out_dir, "05_stl_decomposition.png", fig)
        print(f"Saved: {path}")
        if show:
            plt.show()
        plt.close(fig)
    else:
        print("statsmodels not available: skipping STL decomposition plot.")


def plot_weather_effects(df: pd.DataFrame, out_dir: Path, show: bool) -> None:
    # Weather condition vs total rentals
    fig, ax = plt.subplots(figsize=(10, 4))
    sns.boxplot(data=df, x="weather_condition", y="total_count", ax=ax)
    ax.set_title("Total rentals by weather condition")
    ax.set_xlabel("Weather condition")
    ax.set_ylabel("Total rentals (hourly)")
    ax.tick_params(axis="x", rotation=15)
    path = savefig(out_dir, "06_box_total_by_weather.png", fig)
    print(f"Saved: {path}")
    if show:
        plt.show()
    plt.close(fig)


def plot_monthly(df: pd.DataFrame, out_dir: Path, show: bool) -> None:
    monthly = df.set_index("datetime").resample("MS")["total_count"].mean().reset_index()
    monthly["month_name"] = monthly["datetime"].dt.month_name().str[:3]

    fig, ax = plt.subplots(figsize=(12, 4))
    sns.lineplot(data=monthly, x="datetime", y="total_count", ax=ax)
    ax.set_title("Monthly mean bike rentals (hourly averaged)")
    ax.set_xlabel("Month")
    ax.set_ylabel("Mean total rentals")
    path = savefig(out_dir, "07_monthly_mean.png", fig)
    print(f"Saved: {path}")
    if show:
        plt.show()
    plt.close(fig)

    # Month distribution (as in original notebook, but use order)
    fig, ax = plt.subplots(figsize=(10, 4))
    order = list(range(1, 13))
    # Some seaborn/matplotlib combinations can hit a "color=None" edge case; set an explicit default.
    sns.barplot(
        data=df,
        x="month",
        y="total_count",
        estimator=np.mean,
        errorbar=("ci", 95),
        order=order,
        ax=ax,
        color=sns.color_palette()[0],
    )
    ax.set_title("Mean hourly rentals by month (95% CI)")
    ax.set_xlabel("Month (1–12)")
    ax.set_ylabel("Mean total rentals")
    path = savefig(out_dir, "08_mean_by_month_ci.png", fig)
    print(f"Saved: {path}")
    if show:
        plt.show()
    plt.close(fig)


def plot_holiday_workingday(df: pd.DataFrame, out_dir: Path, show: bool) -> None:
    fig, axes = plt.subplots(ncols=2, figsize=(12, 4), sharey=True)
    sns.barplot(data=df, x="is_holiday", y="total_count", hue="season", estimator=np.mean, errorbar=("ci", 95), ax=axes[0])
    axes[0].set_title("Holiday vs rentals (by season)")
    axes[0].set_xlabel("Holiday (0/1)")
    axes[0].set_ylabel("Mean total rentals")

    sns.barplot(data=df, x="is_workingday", y="total_count", hue="season", estimator=np.mean, errorbar=("ci", 95), ax=axes[1])
    axes[1].set_title("Working day vs rentals (by season)")
    axes[1].set_xlabel("Workingday (0/1)")
    axes[1].set_ylabel("")

    for ax in axes:
        ax.legend(title="Season", fontsize=9)

    path = savefig(out_dir, "09_holiday_workingday_by_season.png", fig)
    print(f"Saved: {path}")
    if show:
        plt.show()
    plt.close(fig)


def plot_boxplots(df: pd.DataFrame, out_dir: Path, show: bool) -> None:
    # Total_count / casual / registered distributions
    fig, axes = plt.subplots(ncols=2, figsize=(12, 4))
    sns.boxplot(data=df[["total_count", "casual", "registered"]], ax=axes[0])
    axes[0].set_title("Distribution: total vs casual vs registered")
    axes[0].set_xlabel("")
    axes[0].set_ylabel("Count")

    sns.boxplot(data=df[["temp", "windspeed", "humidity"]], ax=axes[1])
    axes[1].set_title("Distribution: weather numeric features")
    axes[1].set_xlabel("")
    axes[1].set_ylabel("Value")

    path = savefig(out_dir, "10_boxplots_distributions.png", fig)
    print(f"Saved: {path}")
    if show:
        plt.show()
    plt.close(fig)

    # Hourly boxplot (can be dense; show as violin + box overlay)
    fig, ax = plt.subplots(figsize=(12, 4))
    sns.violinplot(data=df, x="hour", y="total_count", inner="quartile", ax=ax)
    ax.set_title("Hourly distribution of rentals (violin + quartiles)")
    ax.set_xlabel("Hour")
    ax.set_ylabel("Total rentals")
    path = savefig(out_dir, "11_hourly_violin.png", fig)
    print(f"Saved: {path}")
    if show:
        plt.show()
    plt.close(fig)


def plot_correlation(df: pd.DataFrame, out_dir: Path, show: bool) -> None:
    numeric_cols = ["temp", "atemp", "humidity", "windspeed", "casual", "registered", "total_count"]
    corr = df[numeric_cols].corr()

    fig, ax = plt.subplots(figsize=(7, 6))
    sns.heatmap(corr, annot=True, fmt=".2f", cmap="vlag", center=0, square=True, ax=ax)
    ax.set_title("Correlation heatmap (numeric features)")
    path = savefig(out_dir, "12_correlation_heatmap.png", fig)
    print(f"Saved: {path}")
    if show:
        plt.show()
    plt.close(fig)

    # Pairplot can be heavy; keep the sample modest so the script finishes quickly.
    sample_n = min(800, len(df))
    print(f"Building pairplot (sample n={sample_n}) ...")
    sample = df[numeric_cols].sample(n=sample_n, random_state=42)
    # Some seaborn/matplotlib combos can pass color=None into histplot; set a default.
    g = sns.pairplot(
        sample,
        # corner=True,
        diag_kind="hist",
        diag_kws={"color": sns.color_palette()[0]},
    )
    g.fig.suptitle("Pairplot (sampled)", y=1.02)
    path = savefig(out_dir, "13_pairplot_sampled.png", g.figure)
    print(f"Saved: {path}")
    if show:
        plt.show()
    plt.close(g.figure)


def optional_forecast(df: pd.DataFrame, out_dir: Path, show: bool) -> None:
    """
    The original notebook had an ARIMA example. A more modern/default approach for
    seasonal hourly/daily data is SARIMAX.

    We do a lightweight demo on daily mean rentals and forecast the next 30 days.
    """
    if not _HAS_SARIMAX:
        print("statsmodels SARIMAX not available: skipping forecasting demo.")
        return

    daily = df.set_index("datetime").resample("D")["total_count"].mean()
    y = daily.asfreq("D").interpolate()

    # A small, reasonable seasonal model (period=7 for weekly seasonality on daily data)
    # This is not "the best model"—it's an EDA-friendly demo.
    with warnings.catch_warnings():
        warnings.simplefilter("ignore")
        model = SARIMAX(y, order=(1, 1, 1), seasonal_order=(1, 1, 1, 7), enforce_stationarity=False, enforce_invertibility=False)
        res = model.fit(disp=False)

    pred = res.get_forecast(steps=30)
    pred_mean = pred.predicted_mean
    pred_ci = pred.conf_int()

    fig, ax = plt.subplots(figsize=(14, 4))
    ax.plot(y.index, y.values, label="observed")
    ax.plot(pred_mean.index, pred_mean.values, label="forecast")
    ax.fill_between(pred_ci.index, pred_ci.iloc[:, 0], pred_ci.iloc[:, 1], alpha=0.2, label="forecast CI")
    ax.set_title("SARIMAX forecast (daily mean rentals) — next 30 days")
    ax.set_xlabel("Date")
    ax.set_ylabel("Mean rentals")
    ax.legend()
    path = savefig(out_dir, "14_sarimax_forecast.png", fig)
    print(f"Saved: {path}")
    if show:
        plt.show()
    plt.close(fig)


def main() -> None:
    parser = argparse.ArgumentParser(description="Modernized Bike Sharing EDA (hour.csv)")
    parser.add_argument("--data", type=Path, default=Path("hour.csv"), help="Path to hour.csv")
    parser.add_argument("--out", type=Path, default=Path("eda_outputs"), help="Output folder for plots")
    parser.add_argument("--show", action="store_true", help="Show plots interactively (in addition to saving)")
    args = parser.parse_args()

    # Modern plotting defaults
    sns.set_theme(style="whitegrid", context="talk")
    plt.rcParams.update({
        "figure.figsize": (12, 4),
        "axes.titlesize": 16,
        "axes.labelsize": 12,
        "xtick.labelsize": 10,
        "ytick.labelsize": 10,
    })

    out_dir = _ensure_output_dir(args.out)

    df = load_hour_csv(args.data)
    print_overview(df)

    print("\n=== Value counts (season) ===")
    print(df["season"].value_counts())

    # Plots
    plot_season_hourly(df, out_dir, args.show)
    plot_weekday_hourly(df, out_dir, args.show)
    plot_time_series(df, out_dir, args.show)
    plot_monthly(df, out_dir, args.show)
    plot_holiday_workingday(df, out_dir, args.show)
    plot_boxplots(df, out_dir, args.show)
    plot_weather_effects(df, out_dir, args.show)
    plot_correlation(df, out_dir, args.show)
    optional_forecast(df, out_dir, args.show)

    print("\nDone. Plots saved to:", out_dir.resolve())


if __name__ == "__main__":
    main()

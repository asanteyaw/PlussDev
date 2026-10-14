#!/usr/bin/env python3
from __future__ import annotations

import csv
import math
import random
from dataclasses import dataclass
from datetime import datetime, timedelta
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "data" / "flash_datasets"


@dataclass(frozen=True)
class DatasetSpec:
    name: str
    purpose: str
    schema: list[tuple[str, str]]
    rows: list[list[object]]


def iso_day(base: datetime, offset_days: int) -> str:
    return (base + timedelta(days=offset_days)).strftime("%Y-%m-%d")


def iso_hour(base: datetime, offset_hours: int) -> str:
    return (base + timedelta(hours=offset_hours)).strftime("%Y-%m-%dT%H:%M:%SZ")


def rounded(value: float, digits: int = 3) -> float:
    return round(value, digits)


def build_tips() -> DatasetSpec:
    rng = random.Random(101)
    schema = [
        ("total_bill", "numeric"),
        ("tip", "numeric"),
        ("size", "numeric"),
        ("service_score", "numeric"),
        ("wait_minutes", "numeric"),
        ("day", "note"),
        ("meal", "note"),
        ("smoker", "note"),
        ("section", "note"),
    ]
    days = ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"]
    meals = ["Lunch", "Dinner"]
    sections = ["Patio", "Main", "Window", "Booth"]
    rows: list[list[object]] = []
    for i in range(240):
        day = days[i % len(days)]
        meal = meals[(i // 3) % len(meals)]
        size = 2 + (i % 4)
        service = 3.1 + 0.55 * math.sin(i / 9.0) + 0.35 * math.cos(i / 17.0)
        wait = 10.0 + 4.5 * math.sin(i / 8.0) + 1.6 * size + rng.uniform(-1.5, 1.5)
        base_bill = 14.0 + 2.8 * size + 1.1 * (day in {"Fri", "Sat", "Sun"}) + 1.4 * (meal == "Dinner")
        total_bill = base_bill + 2.2 * math.sin(i / 10.0) + rng.uniform(-2.0, 2.0)
        tip_rate = 0.11 + 0.012 * max(service - 3.0, 0.0) + 0.004 * (meal == "Dinner")
        tip = total_bill * tip_rate + rng.uniform(-0.3, 0.35)
        rows.append([
            rounded(max(total_bill, 7.5), 2),
            rounded(max(tip, 0.75), 2),
            float(size),
            rounded(min(max(service, 1.5), 5.0), 2),
            rounded(max(wait, 3.0), 2),
            day,
            meal,
            "yes" if (i % 5 in {1, 4}) else "no",
            sections[(i // 7) % len(sections)],
        ])
    return DatasetSpec("tips", "Mixed numeric/categorical restaurant data for bar, box, regression, and hue demos.", schema, rows)


def build_flights() -> DatasetSpec:
    schema = [
        ("month_start", "temporal"),
        ("passengers", "numeric"),
        ("fare_index", "numeric"),
        ("fuel_index", "numeric"),
        ("load_factor", "numeric"),
        ("delay_minutes", "numeric"),
        ("route", "note"),
        ("season", "note"),
    ]
    rows: list[list[object]] = []
    base = datetime(2014, 1, 1)
    season_map = {
        12: "winter", 1: "winter", 2: "winter",
        3: "spring", 4: "spring", 5: "spring",
        6: "summer", 7: "summer", 8: "summer",
        9: "fall", 10: "fall", 11: "fall",
    }
    for month_idx in range(144):
        year = 2014 + month_idx // 12
        month = 1 + month_idx % 12
        stamp = datetime(year, month, 1).strftime("%Y-%m-%d")
        phase = month_idx / 12.0
        passengers = 135.0 + 19.0 * math.sin(2.0 * math.pi * (month - 1) / 12.0) + 2.6 * phase
        fare = 92.0 + 7.0 * math.cos(2.0 * math.pi * (month - 1) / 12.0 + 0.4) + 0.9 * phase
        fuel = 68.0 + 10.0 * math.sin(phase / 1.7) + 5.0 * math.cos(month_idx / 9.0)
        load = 0.71 + 0.07 * math.sin(2.0 * math.pi * (month - 1) / 12.0 - 0.2) + 0.02 * math.sin(phase / 2.1)
        delay = 11.0 + 2.2 * math.cos(2.0 * math.pi * (month - 1) / 12.0) + 1.7 * math.sin(month_idx / 8.0)
        rows.append([
            stamp,
            rounded(passengers, 2),
            rounded(fare, 2),
            rounded(fuel, 2),
            rounded(load, 4),
            rounded(max(delay, 3.0), 2),
            "north_atlantic" if month_idx % 2 == 0 else "domestic_connector",
            season_map[month],
        ])
    return DatasetSpec("flights", "Monthly airline time series for seasonality, joint, and lag-style analysis.", schema, rows)


def build_petals() -> DatasetSpec:
    rng = random.Random(202)
    schema = [
        ("sepal_length", "numeric"),
        ("sepal_width", "numeric"),
        ("petal_length", "numeric"),
        ("petal_width", "numeric"),
        ("color_score", "numeric"),
        ("species", "note"),
        ("habitat", "note"),
        ("band", "note"),
    ]
    configs = {
        "setosa": (5.0, 3.45, 1.46, 0.24, 0.22),
        "versicolor": (5.94, 2.77, 4.26, 1.33, 0.58),
        "virginica": (6.59, 2.97, 5.55, 2.03, 0.82),
    }
    habitats = {"setosa": "brook", "versicolor": "meadow", "virginica": "ridge"}
    rows: list[list[object]] = []
    for species, (sl, sw, pl, pw, color) in configs.items():
        for i in range(50):
            drift = (i - 24.5) / 60.0
            rows.append([
                rounded(sl + drift + rng.uniform(-0.25, 0.25), 2),
                rounded(sw + 0.3 * math.sin(i / 6.0) + rng.uniform(-0.18, 0.18), 2),
                rounded(pl + 0.45 * drift + rng.uniform(-0.22, 0.22), 2),
                rounded(pw + 0.18 * drift + rng.uniform(-0.08, 0.08), 2),
                rounded(color + 0.09 * math.sin(i / 7.0) + rng.uniform(-0.05, 0.05), 3),
                species,
                habitats[species],
                f"band_{1 + (i % 5)}",
            ])
    return DatasetSpec("petals", "Compact classification-friendly flower measurements for pairplot and class overlays.", schema, rows)


def build_exams() -> DatasetSpec:
    rng = random.Random(303)
    schema = [
        ("exam_date", "temporal"),
        ("assignment", "numeric"),
        ("midterm_1", "numeric"),
        ("midterm_2", "numeric"),
        ("final_exam", "numeric"),
        ("attendance_rate", "numeric"),
        ("lab_score", "numeric"),
        ("cohort", "note"),
        ("instructor", "note"),
        ("section", "note"),
    ]
    rows: list[list[object]] = []
    base = datetime(2025, 1, 15)
    cohorts = ["winter", "spring", "summer"]
    instructors = ["Ada", "Nii", "Esi"]
    sections = ["A", "B", "C", "D"]
    for i in range(180):
        cohort = cohorts[(i // 60) % len(cohorts)]
        attendance = 0.73 + 0.18 * math.sin(i / 14.0) + rng.uniform(-0.04, 0.04)
        assignment = 62.0 + 28.0 * attendance + rng.uniform(-7.0, 6.0)
        mid1 = 58.0 + 31.0 * attendance + 4.0 * math.sin(i / 9.0) + rng.uniform(-8.0, 8.0)
        mid2 = 60.0 + 29.0 * attendance + 3.5 * math.cos(i / 11.0) + rng.uniform(-7.0, 7.0)
        lab = 64.0 + 24.0 * attendance + rng.uniform(-5.0, 5.0)
        final = 0.22 * assignment + 0.29 * mid1 + 0.27 * mid2 + 0.22 * lab + rng.uniform(-5.5, 5.5)
        rows.append([
            iso_day(base, i * 4),
            rounded(min(max(assignment, 35.0), 100.0), 2),
            rounded(min(max(mid1, 30.0), 100.0), 2),
            rounded(min(max(mid2, 30.0), 100.0), 2),
            rounded(min(max(final, 28.0), 100.0), 2),
            rounded(min(max(attendance, 0.45), 1.0), 4),
            rounded(min(max(lab, 35.0), 100.0), 2),
            cohort,
            instructors[(i // 30) % len(instructors)],
            sections[i % len(sections)],
        ])
    return DatasetSpec("exams", "Assessment data for grouped bars, grade distributions, and policy demonstrations.", schema, rows)


def build_retail_daily() -> DatasetSpec:
    schema = [
        ("date", "temporal"),
        ("revenue", "numeric"),
        ("orders", "numeric"),
        ("avg_ticket", "numeric"),
        ("promo_spend", "numeric"),
        ("footfall", "numeric"),
        ("web_sessions", "numeric"),
        ("conversion_rate", "numeric"),
        ("store", "note"),
        ("region", "note"),
        ("channel", "note"),
    ]
    rows: list[list[object]] = []
    base = datetime(2016, 1, 1)
    stores = ["flagship", "market", "harbor", "uptown"]
    regions = ["north", "south", "east", "west"]
    channels = ["in_store", "online"]
    for i in range(3650):
        season = math.sin(2.0 * math.pi * (i % 365) / 365.0)
        trend = 1.0 + 0.00042 * i
        orders = 180.0 * trend + 35.0 * season + 12.0 * math.cos(i / 17.0)
        promo = 18.0 + 6.0 * (i % 14 == 0) + 4.5 * max(season, 0.0)
        avg_ticket = 41.0 + 3.2 * math.cos(i / 28.0) + 0.002 * i
        revenue = orders * avg_ticket * (0.92 + 0.04 * math.sin(i / 29.0))
        footfall = orders * (1.8 + 0.15 * math.sin(i / 13.0))
        web = 640.0 * trend + 150.0 * max(season, 0.0) + 30.0 * math.cos(i / 9.0)
        conversion = min(max(orders / max(web, 1.0), 0.02), 0.45)
        rows.append([
            iso_day(base, i),
            rounded(revenue, 2),
            rounded(max(orders, 35.0), 2),
            rounded(avg_ticket, 2),
            rounded(promo, 2),
            rounded(max(footfall, 70.0), 2),
            rounded(max(web, 120.0), 2),
            rounded(conversion, 4),
            stores[(i // 365) % len(stores)],
            regions[(i // 730) % len(regions)],
            channels[i % len(channels)],
        ])
    return DatasetSpec("retail_daily", "Long daily retail panel for time series, promotions, seasonality, and grouped summaries.", schema, rows)


def build_weather_hourly() -> DatasetSpec:
    schema = [
        ("timestamp", "temporal"),
        ("temp_c", "numeric"),
        ("dew_point_c", "numeric"),
        ("humidity", "numeric"),
        ("wind_kph", "numeric"),
        ("pressure_hpa", "numeric"),
        ("rain_mm", "numeric"),
        ("solar_wm2", "numeric"),
        ("demand_mw", "numeric"),
        ("station", "note"),
        ("season", "note"),
        ("condition", "note"),
    ]
    rows: list[list[object]] = []
    base = datetime(2012, 1, 1)
    stations = ["north_ridge", "central_yard", "harbor_point"]
    for i in range(120_000):
        day_of_year = (i // 24) % 365
        hour = i % 24
        annual = math.sin(2.0 * math.pi * day_of_year / 365.0 - 0.7)
        diurnal = math.sin(2.0 * math.pi * hour / 24.0 - 1.4)
        temp = 8.0 + 14.0 * annual + 5.5 * diurnal
        dew = temp - (5.0 + 3.0 * math.cos(i / 37.0))
        humidity = 62.0 - 12.0 * diurnal + 18.0 * max(-annual, 0.0)
        wind = 9.0 + 4.0 * math.sin(i / 11.0) + 2.0 * math.cos(i / 31.0)
        pressure = 1012.0 + 7.0 * math.cos(i / 53.0) + 4.0 * math.sin(i / 91.0)
        rain = max(0.0, 1.6 * math.sin(i / 19.0) + 0.8 * math.cos(i / 7.0) - 1.0)
        solar = max(0.0, 820.0 * max(diurnal, 0.0) * (0.55 + 0.45 * max(annual, 0.0)))
        demand = 610.0 + 12.0 * abs(temp - 18.0) + 26.0 * max(diurnal, 0.0) + 18.0 * rain
        month = 1 + ((i // (24 * 30)) % 12)
        if month in {12, 1, 2}:
            season = "winter"
        elif month in {3, 4, 5}:
            season = "spring"
        elif month in {6, 7, 8}:
            season = "summer"
        else:
            season = "fall"
        if rain > 1.0:
            condition = "rain"
        elif solar > 500.0:
            condition = "clear"
        elif humidity > 75.0:
            condition = "cloud"
        else:
            condition = "mixed"
        rows.append([
            iso_hour(base, i),
            rounded(temp, 2),
            rounded(dew, 2),
            rounded(min(max(humidity, 18.0), 100.0), 2),
            rounded(max(wind, 0.0), 2),
            rounded(pressure, 2),
            rounded(rain, 3),
            rounded(solar, 2),
            rounded(demand, 2),
            stations[(i // 40_000) % len(stations)],
            season,
            condition,
        ])
    return DatasetSpec("weather_hourly", "Large hourly temporal dataset for performance, lag, resample, and long-run flash demos.", schema, rows)


def build_factor_blocks() -> DatasetSpec:
    rng = random.Random(404)
    schema = [(f"factor_{i:02d}", "numeric") for i in range(1, 13)] + [("regime", "note"), ("cluster", "note")]
    rows: list[list[object]] = []
    for i in range(5000):
        g1 = math.sin(i / 21.0) + 0.7 * math.cos(i / 33.0)
        g2 = math.cos(i / 17.0) - 0.5 * math.sin(i / 29.0)
        g3 = math.sin(i / 13.0 + 0.8)
        row = []
        for j in range(12):
            if j < 4:
                base = 1.4 * g1 + 0.15 * j
            elif j < 8:
                base = 1.2 * g2 + 0.12 * (j - 4)
            else:
                base = 1.5 * g3 + 0.1 * (j - 8)
            row.append(rounded(base + rng.uniform(-0.22, 0.22), 4))
        regime_score = g1 + g2 + g3
        row.append("risk_on" if regime_score > 0.8 else "risk_off" if regime_score < -0.8 else "neutral")
        row.append(f"block_{1 + (i % 5)}")
        rows.append(row)
    return DatasetSpec("factor_blocks", "Correlated numeric blocks for correlation heatmaps, clustering, and pairwise structure.", schema, rows)


def build_sensor64() -> DatasetSpec:
    schema = [(f"sensor_{i:02d}", "numeric") for i in range(1, 61)] + [
        ("machine", "note"),
        ("cell", "note"),
        ("shift", "note"),
        ("state", "note"),
    ]
    rows: list[list[object]] = []
    for i in range(1024):
        row: list[object] = []
        base_a = math.sin(i / 18.0)
        base_b = math.cos(i / 29.0)
        for j in range(60):
            band = j // 10
            value = (
                0.8 * base_a
                + 0.6 * base_b * ((band % 3) - 1)
                + 0.12 * math.sin(i / (5.0 + band))
                + 0.03 * (j % 10)
            )
            row.append(rounded(value, 5))
        row.extend([
            f"machine_{1 + (i % 8)}",
            f"cell_{1 + (i % 4)}",
            ["day", "swing", "night"][i % 3],
            "alert" if i % 23 == 0 else "nominal",
        ])
        rows.append(row)
    return DatasetSpec("sensor64", "High-dimensional sensor frame for parallel coordinates, radviz, and dimensional analysis.", schema, rows)


def build_options_chain() -> DatasetSpec:
    schema = [
        ("quote_time", "temporal"),
        ("expiry_date", "temporal"),
        ("strike", "numeric"),
        ("moneyness", "numeric"),
        ("iv", "numeric"),
        ("delta", "numeric"),
        ("gamma", "numeric"),
        ("vega", "numeric"),
        ("theta", "numeric"),
        ("rho", "numeric"),
        ("volume", "numeric"),
        ("open_interest", "numeric"),
        ("bid", "numeric"),
        ("ask", "numeric"),
        ("right", "note"),
        ("underlying", "note"),
        ("expiry_bucket", "note"),
    ]
    rows: list[list[object]] = []
    base = datetime(2026, 1, 2, 14, 0, 0)
    expiries = [21, 45, 90, 180]
    for i in range(4000):
        quote = iso_hour(base, i // 5)
        days = expiries[i % len(expiries)]
        expiry = (base + timedelta(days=days)).strftime("%Y-%m-%d")
        spot = 100.0 + 4.0 * math.sin(i / 70.0)
        strike = 70.0 + 2.0 * (i % 31)
        mny = spot / strike
        tenor = days / 365.0
        iv = 0.14 + 0.18 * abs(math.log(max(mny, 1e-6))) + 0.03 * math.sqrt(tenor)
        delta = max(-0.98, min(0.98, 1.15 * (mny - 1.0)))
        gamma = 0.01 + 0.22 * math.exp(-8.0 * abs(mny - 1.0)) / math.sqrt(max(days, 1))
        vega = 0.07 + 0.4 * tenor * math.exp(-5.0 * abs(mny - 1.0))
        theta = -(0.02 + 0.35 * tenor + 0.06 * abs(mny - 1.0))
        rho = 0.01 + 0.08 * tenor * (1.0 if i % 2 == 0 else -1.0)
        volume = 20.0 + 420.0 * math.exp(-4.5 * abs(mny - 1.0))
        oi = 80.0 + 1200.0 * math.exp(-2.5 * abs(mny - 1.0))
        mid = max(0.15, spot * iv * math.sqrt(tenor) * 0.09)
        spread = 0.04 + 0.02 * abs(mny - 1.0)
        rows.append([
            quote,
            expiry,
            rounded(strike, 2),
            rounded(mny, 4),
            rounded(iv, 4),
            rounded(delta, 4),
            rounded(gamma, 5),
            rounded(vega, 5),
            rounded(theta, 5),
            rounded(rho, 5),
            rounded(volume, 2),
            rounded(oi, 2),
            rounded(max(0.05, mid - spread / 2.0), 4),
            rounded(mid + spread / 2.0, 4),
            "call" if i % 2 == 0 else "put",
            "ACME",
            "short" if days <= 30 else "mid" if days <= 90 else "long",
        ])
    return DatasetSpec("options_chain", "Options-surface sample data with temporals, smiles, and Greeks for 2D/3D views.", schema, rows)


def build_customer_funnel() -> DatasetSpec:
    rng = random.Random(505)
    schema = [
        ("signup_date", "temporal"),
        ("sessions_7d", "numeric"),
        ("pages_7d", "numeric"),
        ("emails_30d", "numeric"),
        ("support_tickets", "numeric"),
        ("revenue_90d", "numeric"),
        ("segment", "note"),
        ("acquisition", "note"),
        ("country", "note"),
        ("plan", "note"),
        ("outcome", "note"),
    ]
    rows: list[list[object]] = []
    base = datetime(2023, 1, 1)
    segments = ["student", "pro", "team", "enterprise"]
    acquisitions = ["search", "partner", "community", "campaign"]
    countries = ["GH", "KE", "NG", "ZA", "UK"]
    plans = ["trial", "basic", "growth", "scale"]
    for i in range(5000):
        sessions = 3.0 + 8.0 * max(math.sin(i / 19.0) + 1.0, 0.0) + rng.uniform(0.0, 2.0)
        pages = sessions * (4.5 + rng.uniform(0.8, 2.2))
        emails = 1.0 + (i % 6) + rng.uniform(0.0, 1.0)
        support = max(0.0, rng.gauss(1.4, 1.0))
        revenue = max(0.0, 9.0 * sessions + 3.2 * pages - 18.0 * support + rng.uniform(-15.0, 25.0))
        if revenue > 220.0:
            outcome = "expand"
        elif revenue > 80.0:
            outcome = "convert"
        elif revenue > 20.0:
            outcome = "active"
        else:
            outcome = "churn"
        rows.append([
            iso_day(base, i % 730),
            rounded(sessions, 2),
            rounded(pages, 2),
            rounded(emails, 2),
            rounded(support, 2),
            rounded(revenue, 2),
            segments[i % len(segments)],
            acquisitions[(i // 5) % len(acquisitions)],
            countries[(i // 17) % len(countries)],
            plans[(i // 11) % len(plans)],
            outcome,
        ])
    return DatasetSpec("customer_funnel", "Lifecycle-style customer data for count, bar, and class-conditioned plots.", schema, rows)


def build_commute_modes() -> DatasetSpec:
    schema = [
        ("survey_date", "temporal"),
        ("distance_km", "numeric"),
        ("duration_min", "numeric"),
        ("cost", "numeric"),
        ("transfers", "numeric"),
        ("satisfaction", "numeric"),
        ("delay_min", "numeric"),
        ("city", "note"),
        ("mode", "note"),
        ("commuter_type", "note"),
        ("weather_bucket", "note"),
    ]
    rows: list[list[object]] = []
    base = datetime(2024, 1, 1)
    cities = ["accra", "nairobi", "lagos", "capetown"]
    modes = ["bus", "rail", "drive", "bike"]
    commuters = ["student", "worker", "contractor"]
    weather = ["clear", "rain", "windy", "humid"]
    for i in range(3000):
        mode = modes[i % len(modes)]
        distance = 2.5 + 22.0 * abs(math.sin(i / 41.0))
        speed = {"bus": 0.42, "rail": 0.68, "drive": 0.55, "bike": 0.32}[mode]
        duration = distance / speed + 4.0 * math.sin(i / 21.0)
        transfers = 0.0 if mode in {"drive", "bike"} else float(1 + (i % 3))
        cost = {"bus": 1.6, "rail": 2.4, "drive": 5.2, "bike": 0.6}[mode] * distance / 5.0
        delay = max(0.0, 6.0 * math.sin(i / 15.0) + (2.0 if mode == "bus" else 0.8))
        satisfaction = 4.7 - 0.05 * duration - 0.08 * delay + (0.25 if mode == "rail" else 0.0)
        rows.append([
            iso_day(base, i % 365),
            rounded(distance, 2),
            rounded(max(duration, 8.0), 2),
            rounded(cost, 2),
            transfers,
            rounded(min(max(satisfaction, 1.0), 5.0), 3),
            rounded(delay, 2),
            cities[(i // 750) % len(cities)],
            mode,
            commuters[(i // 9) % len(commuters)],
            weather[(i // 13) % len(weather)],
        ])
    return DatasetSpec("commute_modes", "Urban mobility data for grouped comparisons, categorical plots, and continuous-vs-temporal views.", schema, rows)


def build_regimes() -> DatasetSpec:
    schema = [
        ("timestamp", "temporal"),
        ("signal", "numeric"),
        ("trend", "numeric"),
        ("seasonality", "numeric"),
        ("volatility", "numeric"),
        ("drawdown", "numeric"),
        ("turnover", "numeric"),
        ("regime", "note"),
        ("asset", "note"),
        ("cycle_phase", "note"),
    ]
    rows: list[list[object]] = []
    base = datetime(2018, 1, 1)
    assets = ["equity", "rates", "fx"]
    for i in range(3000):
        trend = 0.002 * i + 0.8 * math.sin(i / 90.0)
        seasonal = 0.6 * math.sin(2.0 * math.pi * (i % 252) / 252.0)
        vol = 0.14 + 0.07 * abs(math.sin(i / 43.0))
        signal = trend + seasonal - 1.8 * vol + 0.2 * math.cos(i / 17.0)
        drawdown = min(0.0, signal - (0.4 + 0.8 * vol))
        turnover = 0.18 + 0.11 * abs(math.sin(i / 11.0))
        if signal > 1.0:
            regime = "trend_up"
        elif signal < -0.9:
            regime = "trend_down"
        else:
            regime = "range"
        rows.append([
            iso_day(base, i),
            rounded(signal, 4),
            rounded(trend, 4),
            rounded(seasonal, 4),
            rounded(vol, 4),
            rounded(drawdown, 4),
            rounded(turnover, 4),
            regime,
            assets[(i // 1000) % len(assets)],
            "expansion" if (i % 504) < 252 else "cooldown",
        ])
    return DatasetSpec("regimes", "Regime-labelled time series for diagnostics, lag structure, and state-conditioned plots.", schema, rows)


def build_quality_batches() -> DatasetSpec:
    rng = random.Random(606)
    schema = [
        ("batch_time", "temporal"),
        ("thickness", "numeric"),
        ("density", "numeric"),
        ("moisture", "numeric"),
        ("tensile", "numeric"),
        ("impurity_ppm", "numeric"),
        ("defect_rate", "numeric"),
        ("temperature", "numeric"),
        ("line_speed", "numeric"),
        ("plant", "note"),
        ("line", "note"),
        ("material", "note"),
        ("shift", "note"),
    ]
    rows: list[list[object]] = []
    base = datetime(2025, 1, 1, 6, 0, 0)
    plants = ["north", "coast", "delta"]
    materials = ["alloy_a", "alloy_b", "alloy_c"]
    for i in range(2000):
        thickness = 1.05 + 0.05 * math.sin(i / 16.0) + rng.uniform(-0.02, 0.02)
        density = 7.8 + 0.08 * math.cos(i / 21.0) + rng.uniform(-0.03, 0.03)
        moisture = 0.9 + 0.2 * max(math.sin(i / 14.0), -0.2) + rng.uniform(-0.04, 0.04)
        temp = 410.0 + 18.0 * math.sin(i / 33.0) + rng.uniform(-4.0, 4.0)
        speed = 18.0 + 2.5 * math.cos(i / 18.0) + rng.uniform(-0.8, 0.8)
        impurity = 22.0 + 7.0 * abs(math.sin(i / 19.0)) + rng.uniform(-2.0, 2.0)
        tensile = 540.0 + 24.0 * thickness - 5.5 * moisture + rng.uniform(-8.0, 8.0)
        defect = max(0.0, 0.003 + 0.00035 * impurity + 0.0008 * abs(temp - 415.0) + rng.uniform(-0.001, 0.001))
        rows.append([
            iso_hour(base, i * 3),
            rounded(thickness, 4),
            rounded(density, 4),
            rounded(max(moisture, 0.2), 4),
            rounded(tensile, 2),
            rounded(max(impurity, 4.0), 3),
            rounded(defect, 5),
            rounded(temp, 2),
            rounded(speed, 3),
            plants[(i // 700) % len(plants)],
            f"line_{1 + (i % 4)}",
            materials[(i // 9) % len(materials)],
            ["day", "swing", "night"][i % 3],
        ])
    return DatasetSpec("quality_batches", "Industrial quality-control data for box, violin, outlier, and process comparisons.", schema, rows)


def generate_all() -> list[DatasetSpec]:
    return [
        build_tips(),
        build_flights(),
        build_petals(),
        build_exams(),
        build_retail_daily(),
        build_weather_hourly(),
        build_factor_blocks(),
        build_sensor64(),
        build_options_chain(),
        build_customer_funnel(),
        build_commute_modes(),
        build_regimes(),
        build_quality_batches(),
    ]


def write_dataset(spec: DatasetSpec) -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    csv_path = OUT_DIR / f"{spec.name}.csv"
    schema_path = OUT_DIR / f"{spec.name}.schema.csv"

    with csv_path.open("w", newline="", encoding="utf-8") as f:
      writer = csv.writer(f)
      writer.writerow([name for name, _ in spec.schema])
      writer.writerows(spec.rows)

    with schema_path.open("w", newline="", encoding="utf-8") as f:
      writer = csv.writer(f)
      writer.writerow(["name", "type"])
      writer.writerows(spec.schema)


def write_manifest(specs: list[DatasetSpec]) -> None:
    manifest_path = OUT_DIR / "manifest.csv"
    with manifest_path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(["name", "rows", "columns", "purpose"])
        for spec in specs:
            writer.writerow([spec.name, len(spec.rows), len(spec.schema), spec.purpose])


def main() -> None:
    specs = generate_all()
    for spec in specs:
        write_dataset(spec)
    write_manifest(specs)
    print(f"wrote {len(specs)} flash datasets to {OUT_DIR}")


if __name__ == "__main__":
    main()

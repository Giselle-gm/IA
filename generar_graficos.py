#!/usr/bin/env python3
"""
Genera gráficos para el análisis experimental de Greedy + Simulated Annealing.

Ubicación esperada de los CSV (directorio raíz del repositorio):
  - greedy_sa_runs.csv
  - greedy_sa_by_instance.csv
  - greedy_sa_convergence.csv
  - greedy_sa_restarts.csv (opcional; se reserva para análisis posteriores)

Uso:
  python3 generar_graficos.py

Dependencias:
  python3 -m pip install pandas matplotlib

Los gráficos se guardan en ./graficos_generados como PDF y PNG.
El tiempo de convergencia corresponde exclusivamente a la ejecución de SA,
no al tiempo total de Greedy + SA.
"""

from pathlib import Path

import matplotlib
matplotlib.use("Agg")

import matplotlib.pyplot as plt
import pandas as pd


ROOT = Path(__file__).resolve().parent
OUTPUT_DIR = ROOT / "graficos_generados"

RUNS_CSV = ROOT / "greedy_sa_runs.csv"
CONVERGENCE_CSV = ROOT / "greedy_sa_convergence.csv"
BY_INSTANCE_CSV = ROOT / "greedy_sa_by_instance.csv"


def read_csv(path: Path, required_columns: set[str]) -> pd.DataFrame:
    """Lee un CSV y comprueba que contenga las columnas necesarias."""
    if not path.exists():
        raise FileNotFoundError(
            f"No se encontró {path.name}. Debe estar en el directorio raíz."
        )

    data = pd.read_csv(path)
    missing = required_columns.difference(data.columns)
    if missing:
        raise ValueError(
            f"El archivo {path.name} no contiene las columnas: "
            + ", ".join(sorted(missing))
        )
    return data


def save_figure(fig: plt.Figure, filename: str) -> None:
    """Guarda cada gráfico en PDF y PNG."""
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    fig.tight_layout()
    fig.savefig(OUTPUT_DIR / f"{filename}.pdf", bbox_inches="tight")
    fig.savefig(OUTPUT_DIR / f"{filename}.png", dpi=300, bbox_inches="tight")
    plt.close(fig)


def plot_convergence_by_iteration() -> None:
    data = read_csv(
        CONVERGENCE_CSV,
        {"estrategia", "iteracion", "mejor_objetivo"},
    )
    summary = (
        data.groupby(["estrategia", "iteracion"], as_index=False)
        ["mejor_objetivo"].mean()
    )

    fig, ax = plt.subplots(figsize=(8, 5))
    for strategy, group in summary.groupby("estrategia"):
        group = group.sort_values("iteracion")
        ax.plot(
            group["iteracion"],
            group["mejor_objetivo"],
            marker="o",
            markersize=3,
            label=strategy,
        )

    ax.set_title("Convergencia promedio de Simulated Annealing")
    ax.set_xlabel("Iteración")
    ax.set_ylabel("Mejor valor objetivo promedio")
    ax.grid(True, alpha=0.3)
    ax.legend()
    save_figure(fig, "convergencia_iteraciones")


def plot_convergence_by_time() -> None:
    data = read_csv(
        CONVERGENCE_CSV,
        {"estrategia", "tiempo_segundos", "iteracion", "mejor_objetivo"},
    )
    summary = (
        data.groupby(["estrategia", "iteracion"], as_index=False)
        .agg(
            tiempo_promedio=("tiempo_segundos", "mean"),
            objetivo_promedio=("mejor_objetivo", "mean"),
        )
    )

    fig, ax = plt.subplots(figsize=(8, 5))
    for strategy, group in summary.groupby("estrategia"):
        group = group.sort_values("iteracion")
        ax.plot(
            group["tiempo_promedio"],
            group["objetivo_promedio"],
            marker="o",
            markersize=3,
            label=strategy,
        )

    ax.set_title("Convergencia promedio de SA en función del tiempo")
    ax.set_xlabel("Tiempo transcurrido de SA (s)")
    ax.set_ylabel("Mejor valor objetivo promedio")
    ax.grid(True, alpha=0.3)
    ax.legend()
    save_figure(fig, "convergencia_tiempo_sa")


def plot_runtime_comparison() -> None:
    data = read_csv(RUNS_CSV, {"estrategia", "greedy_segundos", "sa_segundos"})
    summary = (
        data.groupby("estrategia")[["greedy_segundos", "sa_segundos"]]
        .mean()
    )

    fig, ax = plt.subplots(figsize=(8, 5))
    summary.rename(
        columns={
            "greedy_segundos": "Greedy",
            "sa_segundos": "Simulated Annealing",
        }
    ).plot(kind="bar", ax=ax)

    ax.set_title("Tiempo promedio por ejecución")
    ax.set_xlabel("Estrategia de construcción")
    ax.set_ylabel("Tiempo promedio (s)")
    ax.tick_params(axis="x", rotation=0)
    ax.grid(axis="y", alpha=0.3)
    ax.legend(title="Etapa")
    save_figure(fig, "tiempo_promedio_por_etapa")


def plot_objective_by_seed() -> None:
    data = read_csv(
        RUNS_CSV,
        {"estrategia", "semilla_greedy", "objetivo_final"},
    )
    summary = (
        data.groupby(["estrategia", "semilla_greedy"], as_index=False)
        ["objetivo_final"].mean()
    )

    fig, ax = plt.subplots(figsize=(8, 5))
    for strategy, group in summary.groupby("estrategia"):
        group = group.sort_values("semilla_greedy")
        ax.plot(
            group["semilla_greedy"],
            group["objetivo_final"],
            marker="o",
            label=strategy,
        )

    ax.set_title("Valor objetivo final promedio por semilla")
    ax.set_xlabel("Semilla aleatoria")
    ax.set_ylabel("Valor objetivo final promedio")
    ax.grid(True, alpha=0.3)
    ax.legend()
    save_figure(fig, "objetivo_por_semilla")


def plot_improvement_by_strategy() -> None:
    data = read_csv(
        BY_INSTANCE_CSV,
        {"estrategia", "mejora_porcentual_promedio"},
    )
    summary = (
        data.groupby("estrategia")["mejora_porcentual_promedio"]
        .mean()
        .sort_values(ascending=False)
    )

    fig, ax = plt.subplots(figsize=(8, 5))
    summary.plot(kind="bar", ax=ax)
    ax.set_title("Mejora porcentual promedio obtenida por SA")
    ax.set_xlabel("Estrategia de construcción")
    ax.set_ylabel("Mejora porcentual promedio (%)")
    ax.tick_params(axis="x", rotation=0)
    ax.grid(axis="y", alpha=0.3)
    save_figure(fig, "mejora_porcentual_estrategia")


def main() -> None:
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    plot_convergence_by_iteration()
    plot_convergence_by_time()
    plot_runtime_comparison()
    plot_objective_by_seed()
    plot_improvement_by_strategy()
    print(f"Gráficos generados correctamente en: {OUTPUT_DIR}")


if __name__ == "__main__":
    main()

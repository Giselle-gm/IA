#!/usr/bin/env python3
"""Genera gráficos de convergencia de SA desde greedy_sa_convergence.csv.

Uso:
    python3 -m pip install pandas matplotlib
    python3 generar_graficos_convergencia.py [ruta_csv]

El tiempo registrado corresponde al tiempo transcurrido dentro de SA,
no al tiempo total de Greedy + SA.
"""
from pathlib import Path
import sys

import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = Path(__file__).resolve().parent
CSV = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "greedy_sa_convergence.csv"
OUT = ROOT / "graficos_generados"


def main():
    if not CSV.exists():
        raise FileNotFoundError(f"No se encontró el archivo: {CSV}")

    data = pd.read_csv(CSV)
    required = {"estrategia", "iteracion", "tiempo_segundos", "mejor_objetivo"}
    missing = required - set(data.columns)
    if missing:
        raise ValueError(f"Faltan columnas: {sorted(missing)}")

    summary = (
        data.groupby(["estrategia", "iteracion"], as_index=False)
        .agg(
            tiempo_promedio=("tiempo_segundos", "mean"),
            objetivo_promedio=("mejor_objetivo", "mean"),
        )
    )
    OUT.mkdir(parents=True, exist_ok=True)

    plots = [
        ("iteracion", "Iteración", "convergencia_iteraciones"),
        ("tiempo_promedio", "Tiempo transcurrido de SA (s)", "convergencia_tiempo_sa"),
    ]

    for x, xlabel, filename in plots:
        fig, ax = plt.subplots(figsize=(8.2, 5))
        for strategy, group in summary.groupby("estrategia"):
            group = group.sort_values("iteracion")
            ax.plot(
                group[x],
                group["objetivo_promedio"],
                marker="o",
                markersize=2.8,
                label=strategy,
            )

        ax.set_title(
            "Convergencia promedio de Simulated Annealing"
            if x == "iteracion"
            else "Convergencia promedio de SA en función del tiempo"
        )
        ax.set_xlabel(xlabel)
        ax.set_ylabel("Mejor valor objetivo promedio")
        ax.grid(True, alpha=0.3)
        ax.legend()
        fig.tight_layout()

        for extension, options in [
            ("pdf", {}),
            ("png", {"dpi": 300}),
            ("svg", {}),
        ]:
            fig.savefig(
                OUT / f"{filename}.{extension}",
                bbox_inches="tight",
                **options,
            )
        plt.close(fig)

    print(f"Gráficos guardados en: {OUT}")


if __name__ == "__main__":
    main()

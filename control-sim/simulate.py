#!/usr/bin/env python3
"""OTV — simulation en boucle fermée de la boucle pression (mode PC).

Modèle : turbine = source de pression du 1er ordre (tau 40 ms) ; poumon
monocompartimental (compliance C, résistance R) ; régulation PID à 100 Hz
(REQ-PERF-009), physique intégrée à 1 kHz. La logique PID et le cadencement
reproduisent `firmware/core/` (référence croisée dans les tests CI).

Vérifie sur les gabarits adulte de l'ISO 80601-2-12 :
  - exactitude de la pression de plateau : ±(2 cmH2O + 4 %)  (REQ-PERF-003)
  - tenue de la PEEP : ±(2 cmH2O + 4 %)                       (REQ-PERF-005)
  - temps de montée à 90 % < 200 ms (cible interne)

Sortie : control-sim/results/*.png + métriques sur stdout.
Code de retour != 0 si un critère échoue (utilisé en CI avec --headless).
"""
import sys
from dataclasses import dataclass
from pathlib import Path

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

DT = 0.001          # pas physique : 1 kHz
CTRL_DIV = 10       # boucle de contrôle à 100 Hz (REQ-PERF-009)
TAU_TURBINE = 0.040 # constante de temps de la turbine, s


@dataclass
class Settings:
    rate_cpm: float = 15.0
    ti_s: float = 1.0
    p_insp: float = 15.0   # au-dessus de la PEEP, cmH2O
    peep: float = 5.0
    p_max: float = 60.0


@dataclass
class Lung:
    name: str
    c_ml_cmh2o: float   # compliance
    r_cmh2o_lps: float  # résistance


class Pid:
    """Miroir de otv_pid.c : anti-windup conditionnel, dérivée sur mesure."""

    def __init__(self, kp, ki, kd, out_min, out_max):
        self.kp, self.ki, self.kd = kp, ki, kd
        self.out_min, self.out_max = out_min, out_max
        self.integ = 0.0
        self.prev_meas = 0.0
        self.primed = False

    def step(self, setpoint, measure, dt):
        err = setpoint - measure
        deriv = -(measure - self.prev_meas) / dt if (self.primed and dt > 0) else 0.0
        self.prev_meas, self.primed = measure, True
        out = self.kp * err + self.integ + self.kd * deriv
        sat_hi = out >= self.out_max and err > 0
        sat_lo = out <= self.out_min and err < 0
        if not (sat_hi or sat_lo):
            self.integ += self.ki * err * dt
        out = self.kp * err + self.integ + self.kd * deriv
        return min(max(out, self.out_min), self.out_max)


def simulate(lung: Lung, s: Settings, t_end=12.0):
    n = int(t_end / DT)
    t = np.arange(n) * DT
    paw = np.zeros(n)    # pression voies aériennes = pression machine, cmH2O
    flow = np.zeros(n)   # L/s, > 0 vers le patient
    vol = np.zeros(n)    # L au-dessus de la CRF

    pid = Pid(kp=3.0, ki=30.0, kd=0.0, out_min=0.0, out_max=80.0)
    period = 60.0 / s.rate_cpm
    c_l = lung.c_ml_cmh2o / 1000.0  # L/cmH2O

    pt = 0.0   # pression turbine
    v = 0.0
    cmd = 0.0
    for i in range(1, n):
        tc = t[i] % period
        target = s.peep + s.p_insp if tc < s.ti_s else s.peep
        if i % CTRL_DIV == 0:  # boucle de contrôle à 100 Hz
            cmd = pid.step(target, pt, DT * CTRL_DIV)
        pt += (cmd - pt) / TAU_TURBINE * DT
        palv = v / c_l
        q = (pt - palv) / lung.r_cmh2o_lps
        v = max(v + q * DT, 0.0)
        paw[i], flow[i], vol[i] = pt, q, v
    return t, paw, flow, vol


def metrics(lung: Lung, s: Settings, t, paw, flow, vol):
    """Mesures sur le dernier cycle complet (régime établi)."""
    period = 60.0 / s.rate_cpm
    t0 = t[-1] - (t[-1] % period) - period  # début de l'avant-dernier cycle
    cyc = (t >= t0) & (t < t0 + period)
    insp = cyc & ((t - t0) % period < s.ti_s)
    exp_tail = cyc & ((t - t0) % period >= period - 0.2 * (period - s.ti_s))
    insp_tail = cyc & ((t - t0) % period > 0.8 * s.ti_s) & ((t - t0) % period < s.ti_s)

    plateau = float(np.mean(paw[insp_tail]))
    peep_held = float(np.mean(paw[exp_tail]))
    vt_ml = float((vol[cyc].max() - vol[cyc].min()) * 1000.0)

    target = s.peep + s.p_insp
    rise = paw[cyc & insp] - s.peep
    idx = np.nonzero(rise >= 0.9 * s.p_insp)[0]
    t90 = float(idx[0] * DT) if idx.size else float("inf")

    tol_plateau = 2.0 + 0.04 * target
    tol_peep = 2.0 + 0.04 * s.peep
    checks = {
        f"plateau {plateau:.1f} / cible {target:.0f} cmH2O (tol ±{tol_plateau:.1f})":
            abs(plateau - target) <= tol_plateau,
        f"PEEP {peep_held:.1f} / cible {s.peep:.0f} cmH2O (tol ±{tol_peep:.1f})":
            abs(peep_held - s.peep) <= tol_peep,
        f"montee 90 % en {t90 * 1000:.0f} ms (< 200 ms)": t90 < 0.200,
    }
    return vt_ml, checks


def plot(lung: Lung, s: Settings, t, paw, flow, vol, vt_ml, out: Path):
    sel = t >= t[-1] - 8.0  # les 2 derniers cycles
    fig, axes = plt.subplots(3, 1, figsize=(9, 7), sharex=True)
    axes[0].plot(t[sel], paw[sel], lw=1.2)
    axes[0].axhline(s.peep + s.p_insp, ls="--", lw=0.8, color="grey")
    axes[0].axhline(s.peep, ls="--", lw=0.8, color="grey")
    axes[0].set_ylabel("Paw (cmH2O)")
    axes[0].set_title(
        f"Mode PC — {lung.name} (C={lung.c_ml_cmh2o:.0f} mL/cmH2O, "
        f"R={lung.r_cmh2o_lps:.0f}) — Vt livre ≈ {vt_ml:.0f} mL")
    axes[1].plot(t[sel], flow[sel] * 60.0, lw=1.2)
    axes[1].axhline(0, lw=0.6, color="grey")
    axes[1].set_ylabel("Debit (L/min)")
    axes[2].plot(t[sel], vol[sel] * 1000.0, lw=1.2)
    axes[2].set_ylabel("Volume (mL)")
    axes[2].set_xlabel("t (s)")
    fig.tight_layout()
    fig.savefig(out, dpi=130)
    plt.close(fig)


SCENARIOS = [
    Lung("adulte normal", c_ml_cmh2o=50.0, r_cmh2o_lps=5.0),
    Lung("adulte restrictif (type SDRA)", c_ml_cmh2o=20.0, r_cmh2o_lps=20.0),
]


def main():
    results = Path(__file__).parent / "results"
    results.mkdir(exist_ok=True)
    s = Settings()
    ok = True
    for lung in SCENARIOS:
        t, paw, flow, vol = simulate(lung, s)
        vt_ml, checks = metrics(lung, s, t, paw, flow, vol)
        slug = lung.name.split(" (")[0].replace(" ", "_")
        plot(lung, s, t, paw, flow, vol, vt_ml, results / f"pc_{slug}.png")
        print(f"\n=== {lung.name} — PC {s.p_insp:.0f}/{s.peep:.0f}, "
              f"{s.rate_cpm:.0f} c/min, Ti {s.ti_s:.1f} s ===")
        print(f"  Vt livre ≈ {vt_ml:.0f} mL")
        for label, passed in checks.items():
            print(f"  [{'OK' if passed else 'ECHEC'}] {label}")
            ok = ok and passed
    print("\nverdict :", "tous les criteres passent" if ok else "ECHEC")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())

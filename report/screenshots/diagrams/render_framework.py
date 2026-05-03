#!/usr/bin/env python3
"""Render the K-Shell module framework diagram as a PNG.

Produces a clean, high-DPI matplotlib figure showing:
  - the three-stage pipeline (Parser -> Dispatcher -> Executor)
  - the four built-ins reachable from the dispatcher
  - the three support modules (History, Prompt, Introspect)
  - data-flow arrows annotated with the contract (return type, key fn name)

Output: framework.png in the same directory.
"""
from pathlib import Path
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from matplotlib.patches import FancyBboxPatch, FancyArrowPatch


HERE = Path(__file__).parent

# Color palette --- muted, print-friendly
NAVY    = "#1F2933"
TEAL    = "#3F8FAA"
WARM    = "#C77B4F"
SAGE    = "#7B9F77"
SAND    = "#E8DEC4"
LIGHT   = "#F4F1EA"
TEXT    = "#1F2933"
ARROW   = "#52606D"


def box(ax, x, y, w, h, label, sublabel="", fc=LIGHT, ec=NAVY, lw=1.3):
    p = FancyBboxPatch((x, y), w, h,
                       boxstyle="round,pad=0.04,rounding_size=0.18",
                       linewidth=lw, edgecolor=ec, facecolor=fc, zorder=2)
    ax.add_patch(p)
    cx, cy = x + w / 2, y + h / 2
    if sublabel:
        ax.text(cx, cy + 0.18, label, ha="center", va="center",
                fontsize=11, weight="bold", color=TEXT)
        ax.text(cx, cy - 0.22, sublabel, ha="center", va="center",
                fontsize=8.5, style="italic", color=TEXT)
    else:
        ax.text(cx, cy, label, ha="center", va="center",
                fontsize=11, weight="bold", color=TEXT)


def arrow(ax, p1, p2, label="", style="-|>", color=ARROW, lw=1.4, ls="-",
          rad=0.0, label_offset=(0, 0.12)):
    a = FancyArrowPatch(p1, p2, arrowstyle=style, mutation_scale=12,
                        color=color, linewidth=lw, linestyle=ls,
                        connectionstyle=f"arc3,rad={rad}", zorder=1)
    ax.add_patch(a)
    if label:
        mx = (p1[0] + p2[0]) / 2 + label_offset[0]
        my = (p1[1] + p2[1]) / 2 + label_offset[1]
        ax.text(mx, my, label, ha="center", va="center",
                fontsize=8, color=TEXT,
                bbox=dict(boxstyle="round,pad=0.20",
                          fc="white", ec="none", alpha=0.92))


def main():
    fig, ax = plt.subplots(figsize=(13, 8), dpi=180)
    ax.set_xlim(0, 13)
    ax.set_ylim(0, 8)
    ax.set_aspect("equal")
    ax.axis("off")

    ax.text(6.5, 7.65, "K-Shell --- Module Framework",
            ha="center", va="center", fontsize=15, weight="bold", color=TEXT)
    ax.text(6.5, 7.25,
            "Pipeline modules (top row) + support modules (bottom row). "
            "Solid arrows: runtime data path. Dashed: side-channel calls.",
            ha="center", va="center", fontsize=9, style="italic", color=TEXT)

    # ---- Top row: pipeline ------------------------------------------------
    box(ax, 0.4, 5.0, 2.0, 1.2, "main.c", "REPL loop",                     fc=SAND)
    box(ax, 3.0, 5.0, 2.0, 1.2, "parser.c", "ks_parse_line",              fc=LIGHT, ec=TEAL, lw=1.6)
    box(ax, 5.6, 5.0, 2.2, 1.2, "dispatcher.c", "ks_dispatch + strip",    fc=LIGHT, ec=TEAL, lw=1.6)
    box(ax, 8.4, 5.0, 2.2, 1.2, "executor.c", "fork/execvp/wait4",        fc=LIGHT, ec=TEAL, lw=1.6)
    box(ax,11.2, 5.0, 1.4, 1.2, "kernel",   "POSIX",                       fc="#E5E5E5")

    # Built-ins panel (right of dispatcher, below)
    box(ax, 5.6, 3.0, 2.2, 1.2, "builtins.c", "cd / exit / help / history", fc=LIGHT, ec=WARM, lw=1.4)

    # ---- Bottom row: support modules -------------------------------------
    box(ax, 0.4, 1.4, 2.0, 1.0, "history.c",   "ring buffer (cap 100)",   fc="#EFE7D6")
    box(ax, 3.0, 1.4, 2.0, 1.0, "prompt.c",    "ks_render_prompt",        fc="#EFE7D6")
    box(ax, 8.4, 1.4, 2.2, 1.0, "introspect.c","--inspect probe + format",fc="#DFE9D8", ec=SAGE, lw=1.6)

    # Header label
    box(ax, 0.0, 6.55, 13.0, 0.55, "include/kshell.h --- public contracts", fc="#FFFAF0", ec=NAVY, lw=1.0)

    # ---- Pipeline arrows -------------------------------------------------
    arrow(ax, (2.4, 5.6), (3.0, 5.6), label="line buf",          label_offset=(0, 0.18))
    arrow(ax, (5.0, 5.6), (5.6, 5.6), label="argc, argv[]",      label_offset=(0, 0.18))
    arrow(ax, (7.8, 5.6), (8.4, 5.6), label="argv (external)",   label_offset=(0, 0.18))
    arrow(ax, (10.6, 5.6), (11.2, 5.6), label="syscall",         label_offset=(0, 0.18))

    # Dispatcher -> builtins
    arrow(ax, (6.7, 5.0), (6.7, 4.2), label="argv (built-in)", label_offset=(0.85, 0))

    # Built-ins -> main (return up)
    arrow(ax, (6.0, 3.6), (1.4, 5.0), rad=-0.3,
          label="KS_OK / KS_EXIT / KS_ERR_*", label_offset=(0, -0.30))

    # ---- Side-channel arrows (dashed) ------------------------------------
    arrow(ax, (1.4, 5.0), (1.4, 2.4), label="record line", ls="--",
          label_offset=(-0.92, 0.0))
    arrow(ax, (1.4, 1.4), (4.0, 1.4), label="read for `history`", ls="--",
          label_offset=(0, -0.18))
    arrow(ax, (4.0, 2.4), (4.0, 5.0), label="render", ls="--",
          label_offset=(-0.45, 0))
    arrow(ax, (9.5, 5.0), (9.5, 2.4), label="probe (when --inspect set)",
          ls="--", label_offset=(1.55, 0))

    # ---- Legend ----------------------------------------------------------
    legend_handles = [
        mpatches.Patch(facecolor=LIGHT, edgecolor=TEAL, label="Pipeline module"),
        mpatches.Patch(facecolor=LIGHT, edgecolor=WARM, label="Built-in handlers"),
        mpatches.Patch(facecolor="#DFE9D8", edgecolor=SAGE, label="Introspection (Phase 3)"),
        mpatches.Patch(facecolor="#EFE7D6", edgecolor=NAVY, label="Support module"),
    ]
    ax.legend(handles=legend_handles, loc="lower center",
              bbox_to_anchor=(0.5, -0.04), ncol=4, frameon=False, fontsize=9)

    plt.tight_layout()
    out = HERE / "framework.png"
    fig.savefig(out, dpi=180, bbox_inches="tight",
                facecolor="white", edgecolor="none")
    print(f"wrote {out}")


if __name__ == "__main__":
    main()

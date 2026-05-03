#!/usr/bin/env python3
"""Render the K-Shell process model + --inspect probe-point diagram.

Two side-by-side swim lanes show:
  - left:  the default external-command path (fork -> execvp -> waitpid)
  - right: the instrumented --inspect path with three labeled probe points
           (CLOCK_MONOTONIC bracket, /dev/fd snapshot, wait4 + rusage)

Output: process_model.png in the same directory.
"""
from pathlib import Path
import matplotlib.pyplot as plt
from matplotlib.patches import FancyBboxPatch, FancyArrowPatch, Rectangle


HERE = Path(__file__).parent

NAVY    = "#1F2933"
TEAL    = "#3F8FAA"
SAGE    = "#7B9F77"
WARM    = "#C77B4F"
SAND    = "#E8DEC4"
LIGHT   = "#F4F1EA"
PROBE   = "#D4A857"
TEXT    = "#1F2933"
ARROW   = "#52606D"
GHOST   = "#9AA5B1"


def event(ax, x, y, label, fc=LIGHT, ec=NAVY, w=2.2, h=0.5):
    p = FancyBboxPatch((x - w / 2, y - h / 2), w, h,
                       boxstyle="round,pad=0.04,rounding_size=0.10",
                       linewidth=1.2, edgecolor=ec, facecolor=fc, zorder=2)
    ax.add_patch(p)
    ax.text(x, y, label, ha="center", va="center",
            fontsize=9, color=TEXT)


def probe(ax, x, y, label, side="right"):
    p = FancyBboxPatch((x - 1.1, y - 0.30), 2.2, 0.60,
                       boxstyle="round,pad=0.05,rounding_size=0.18",
                       linewidth=1.4, edgecolor=PROBE, facecolor="#FFF8E2", zorder=3)
    ax.add_patch(p)
    ax.text(x, y, label, ha="center", va="center",
            fontsize=8.5, weight="bold", color=TEXT)


def vline(ax, x, y0, y1, color=ARROW, lw=1.0, ls="-"):
    ax.plot([x, x], [y0, y1], color=color, linewidth=lw, linestyle=ls, zorder=1)


def hline(ax, x0, x1, y, color=ARROW, lw=1.0, ls="-"):
    ax.plot([x0, x1], [y, y], color=color, linewidth=lw, linestyle=ls, zorder=1)


def main():
    fig, ax = plt.subplots(figsize=(13.5, 9.5), dpi=180)
    ax.set_xlim(0, 14)
    ax.set_ylim(0, 11)
    ax.set_aspect("equal")
    ax.axis("off")

    ax.text(7, 10.55, "K-Shell --- Process Model + --inspect Probe Points",
            ha="center", va="center", fontsize=15, weight="bold", color=TEXT)
    ax.text(7, 10.15,
            "Default path (left) is the textbook fork/execvp/waitpid sequence. "
            "The instrumented path (right) inserts three labeled probe points.",
            ha="center", va="center", fontsize=9, style="italic", color=TEXT)

    # ----------------------------------------------------------------------
    # LEFT --- Default path
    # ----------------------------------------------------------------------
    px = 2.5   # parent timeline x
    cx = 5.0   # child timeline x

    ax.add_patch(Rectangle((0.4, 0.6), 6.0, 9.0,
                           facecolor="#F8F5EF", edgecolor="none", zorder=0))
    ax.text(3.4, 9.4, "Default path (no --inspect)",
            ha="center", va="center", fontsize=11, weight="bold", color=NAVY)

    ax.text(px, 9.0, "Parent (kshell)", ha="center", va="center",
            fontsize=9, weight="bold", color=TEAL)
    ax.text(cx, 9.0, "Child", ha="center", va="center",
            fontsize=9, weight="bold", color=WARM)

    vline(ax, px, 0.9, 8.7, color=TEAL, lw=2)
    vline(ax, cx, 4.6, 7.0, color=WARM, lw=2)

    event(ax, px, 8.4, "ks_dispatch -> ks_execute", fc=SAND)
    event(ax, px, 7.6, "fork()", fc=LIGHT, ec=TEAL)

    # fork arrow
    a = FancyArrowPatch((px + 0.45, 7.6), (cx - 0.45, 7.0),
                        arrowstyle="-|>", mutation_scale=12,
                        color=WARM, linewidth=1.4, zorder=2)
    ax.add_patch(a)
    ax.text((px + cx) / 2, 7.45, "fork", fontsize=8,
            color=TEXT, ha="center")

    event(ax, cx, 6.4, "execvp(argv[0], argv)", fc=LIGHT, ec=WARM)
    event(ax, cx, 5.4, "(child runs program)", fc="#FAF6EC", ec=GHOST)
    event(ax, cx, 4.6, "_exit(status)", fc=LIGHT, ec=WARM)

    event(ax, px, 6.7, "waitpid(pid, &st, 0)", fc=LIGHT, ec=TEAL)
    event(ax, px, 5.6, "(blocked, EINTR retry)", fc="#FAF6EC", ec=GHOST)
    event(ax, px, 4.4, "decode status -> exit code", fc=LIGHT, ec=TEAL)

    # exit -> wait
    a = FancyArrowPatch((cx - 0.45, 4.6), (px + 0.45, 4.4),
                        arrowstyle="-|>", mutation_scale=12,
                        color=ARROW, linewidth=1.2, zorder=2)
    ax.add_patch(a)
    ax.text((px + cx) / 2, 4.55, "SIGCHLD + status",
            fontsize=8, color=TEXT, ha="center")

    event(ax, px, 3.2, "return KS_OK -> REPL", fc=SAND)

    # ----------------------------------------------------------------------
    # RIGHT --- Instrumented --inspect path
    # ----------------------------------------------------------------------
    pxR = 8.4
    cxR = 11.0

    ax.add_patch(Rectangle((7.6, 0.6), 6.0, 9.0,
                           facecolor="#EEF7EE", edgecolor="none", zorder=0))
    ax.text(10.6, 9.4, "Instrumented path (--inspect set)",
            ha="center", va="center", fontsize=11, weight="bold", color=NAVY)

    ax.text(pxR, 9.0, "Parent (kshell)", ha="center", va="center",
            fontsize=9, weight="bold", color=TEAL)
    ax.text(cxR, 9.0, "Child", ha="center", va="center",
            fontsize=9, weight="bold", color=WARM)

    vline(ax, pxR, 0.9, 8.7, color=TEAL, lw=2)
    vline(ax, cxR, 4.4, 6.7, color=WARM, lw=2)

    event(ax, pxR, 8.4, "ks_dispatch -> strip --inspect", fc=SAND)
    event(ax, pxR, 7.7, "pipe(pipefd)", fc=LIGHT, ec=TEAL)

    probe(ax, pxR + 1.7, 7.7, "PROBE 1\nclock_gettime(MONO) t0")

    event(ax, pxR, 6.95, "fork()", fc=LIGHT, ec=TEAL)
    a = FancyArrowPatch((pxR + 0.45, 6.95), (cxR - 0.45, 6.65),
                        arrowstyle="-|>", mutation_scale=12,
                        color=WARM, linewidth=1.4, zorder=2)
    ax.add_patch(a)

    event(ax, cxR, 6.3, "ks_introspect_child_snapshot", fc=LIGHT, ec=SAGE)
    probe(ax, cxR + 1.5, 6.3, "PROBE 2\nopendir(/dev/fd)\n-> pipe write end")

    event(ax, cxR, 5.5, "execvp(argv[0], argv)", fc=LIGHT, ec=WARM)
    event(ax, cxR, 4.9, "(child runs program)", fc="#FAF6EC", ec=GHOST)
    event(ax, cxR, 4.4, "_exit(status)", fc=LIGHT, ec=WARM)

    event(ax, pxR, 6.0, "ks_introspect_read_fds", fc=LIGHT, ec=SAGE)
    event(ax, pxR, 5.2, "wait4(pid,&st,0,&rusage)", fc=LIGHT, ec=TEAL)
    probe(ax, pxR - 1.7, 5.2, "PROBE 3\nper-child rusage\nat reap time")

    a = FancyArrowPatch((cxR - 0.45, 4.4), (pxR + 0.45, 5.2),
                        arrowstyle="-|>", mutation_scale=12,
                        color=ARROW, linewidth=1.2, zorder=2)
    ax.add_patch(a)

    event(ax, pxR, 4.2, "clock_gettime(MONO) t1", fc=LIGHT, ec=TEAL)
    event(ax, pxR, 3.4, "ks_introspect_format(table)", fc=LIGHT, ec=SAGE)
    event(ax, pxR, 2.6, "fputs(table) + return KS_OK", fc=SAND)

    # Legend
    ax.text(7, 1.5,
            "PROBE 1: brackets the entire transaction with CLOCK_MONOTONIC.    "
            "PROBE 2: child enumerates /dev/fd between fork and exec, writes through pipe.    "
            "PROBE 3: wait4 returns per-child rusage atomically with the reap.",
            ha="center", va="center", fontsize=8.5, style="italic",
            color=TEXT, wrap=True)

    plt.tight_layout()
    out = HERE / "process_model.png"
    fig.savefig(out, dpi=180, bbox_inches="tight",
                facecolor="white", edgecolor="none")
    print(f"wrote {out}")


if __name__ == "__main__":
    main()

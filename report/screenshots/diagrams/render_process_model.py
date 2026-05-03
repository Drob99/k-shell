#!/usr/bin/env python3
"""Render the K-Shell process model + --inspect probe-point diagram.

Two side-by-side swim lanes show:
  - left:  the default external-command path (fork -> execvp -> waitpid)
  - right: the instrumented --inspect path with three labeled probe points
           (CLOCK_MONOTONIC bracket, /dev/fd snapshot, wait4 + rusage)

Probe annotations are placed BELOW the timeline events with curved
connectors, to avoid horizontal overlap with the parent/child columns.

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


def event(ax, x, y, label, fc=LIGHT, ec=NAVY, w=2.6, h=0.5):
    p = FancyBboxPatch((x - w / 2, y - h / 2), w, h,
                       boxstyle="round,pad=0.04,rounding_size=0.10",
                       linewidth=1.2, edgecolor=ec, facecolor=fc, zorder=2)
    ax.add_patch(p)
    ax.text(x, y, label, ha="center", va="center",
            fontsize=8.5, color=TEXT)


def probe_card(ax, x, y, num, body, w=3.2, h=1.4):
    p = FancyBboxPatch((x - w / 2, y - h / 2), w, h,
                       boxstyle="round,pad=0.05,rounding_size=0.18",
                       linewidth=1.4, edgecolor=PROBE, facecolor="#FFF8E2", zorder=3)
    ax.add_patch(p)
    ax.text(x, y + 0.45, f"PROBE {num}", ha="center", va="center",
            fontsize=9, weight="bold", color="#8C6213")
    ax.text(x, y - 0.08, body, ha="center", va="center",
            fontsize=7.8, color=TEXT)


def vline(ax, x, y0, y1, color=ARROW, lw=1.0, ls="-"):
    ax.plot([x, x], [y0, y1], color=color, linewidth=lw, linestyle=ls, zorder=1)


def connector(ax, p1, p2, color=PROBE, lw=1.0, rad=0.2):
    a = FancyArrowPatch(p1, p2, arrowstyle="-", color=color,
                        linewidth=lw, linestyle="--",
                        connectionstyle=f"arc3,rad={rad}", zorder=2)
    ax.add_patch(a)


def main():
    fig, ax = plt.subplots(figsize=(15, 11), dpi=180)
    ax.set_xlim(0, 16)
    ax.set_ylim(0, 13)
    ax.set_aspect("equal")
    ax.axis("off")

    ax.text(8, 12.55, "K-Shell --- Process Model + --inspect Probe Points",
            ha="center", va="center", fontsize=15, weight="bold", color=TEXT)
    ax.text(8, 12.10,
            "Default path (left) is the textbook fork/execvp/waitpid sequence. "
            "The instrumented path (right) inserts three labeled probe points.",
            ha="center", va="center", fontsize=9, style="italic", color=TEXT)

    # ----------------------------------------------------------------------
    # LEFT --- Default path
    # ----------------------------------------------------------------------
    px = 2.1   # parent timeline x
    cx = 5.4   # child timeline x

    ax.add_patch(Rectangle((0.4, 0.6), 6.8, 11.0,
                           facecolor="#F8F5EF", edgecolor="none", zorder=0))
    ax.text(3.75, 11.4, "Default path (no --inspect)",
            ha="center", va="center", fontsize=11, weight="bold", color=NAVY)

    ax.text(px, 11.0, "Parent (kshell)", ha="center", va="center",
            fontsize=9, weight="bold", color=TEAL)
    ax.text(cx, 11.0, "Child", ha="center", va="center",
            fontsize=9, weight="bold", color=WARM)

    vline(ax, px, 0.9, 10.7, color=TEAL, lw=2)
    vline(ax, cx, 6.5, 9.0, color=WARM, lw=2)

    event(ax, px, 10.4, "ks_dispatch -> ks_execute", fc=SAND)
    event(ax, px, 9.6,  "fork()", fc=LIGHT, ec=TEAL)

    a = FancyArrowPatch((px + 0.6, 9.6), (cx - 0.6, 9.0),
                        arrowstyle="-|>", mutation_scale=12,
                        color=WARM, linewidth=1.4, zorder=2)
    ax.add_patch(a)
    ax.text((px + cx) / 2, 9.45, "fork", fontsize=8, color=TEXT, ha="center")

    event(ax, cx, 8.4, "execvp(argv[0], argv)", fc=LIGHT, ec=WARM)
    event(ax, cx, 7.5, "(child runs program)", fc="#FAF6EC", ec=GHOST)
    event(ax, cx, 6.5, "_exit(status)", fc=LIGHT, ec=WARM)

    event(ax, px, 8.6, "waitpid(pid, &st, 0)", fc=LIGHT, ec=TEAL)
    event(ax, px, 7.5, "(blocked, EINTR retry)", fc="#FAF6EC", ec=GHOST)
    event(ax, px, 6.3, "decode status -> exit code", fc=LIGHT, ec=TEAL)

    a = FancyArrowPatch((cx - 0.6, 6.5), (px + 0.6, 6.3),
                        arrowstyle="-|>", mutation_scale=12,
                        color=ARROW, linewidth=1.2, zorder=2)
    ax.add_patch(a)
    ax.text((px + cx) / 2, 6.45, "SIGCHLD + status",
            fontsize=7.5, color=TEXT, ha="center")

    event(ax, px, 5.0, "return KS_OK -> REPL", fc=SAND)

    # ----------------------------------------------------------------------
    # RIGHT --- Instrumented --inspect path
    # ----------------------------------------------------------------------
    pxR = 9.6
    cxR = 13.4

    ax.add_patch(Rectangle((8.4, 0.6), 7.2, 11.0,
                           facecolor="#EEF7EE", edgecolor="none", zorder=0))
    ax.text(12.0, 11.4, "Instrumented path (--inspect set)",
            ha="center", va="center", fontsize=11, weight="bold", color=NAVY)

    ax.text(pxR, 11.0, "Parent (kshell)", ha="center", va="center",
            fontsize=9, weight="bold", color=TEAL)
    ax.text(cxR, 11.0, "Child", ha="center", va="center",
            fontsize=9, weight="bold", color=WARM)

    vline(ax, pxR, 0.9, 10.7, color=TEAL, lw=2)
    vline(ax, cxR, 6.0, 9.0, color=WARM, lw=2)

    event(ax, pxR, 10.4, "ks_dispatch -> strip --inspect", fc=SAND)
    event(ax, pxR, 9.6,  "pipe(pipefd) + clock t0", fc=LIGHT, ec=TEAL)

    event(ax, pxR, 8.6,  "fork()", fc=LIGHT, ec=TEAL)
    a = FancyArrowPatch((pxR + 0.6, 8.6), (cxR - 0.6, 9.0),
                        arrowstyle="-|>", mutation_scale=12,
                        color=WARM, linewidth=1.4, zorder=2)
    ax.add_patch(a)

    event(ax, cxR, 8.6, "ks_introspect_child_snapshot", fc=LIGHT, ec=SAGE, w=2.8)
    event(ax, cxR, 7.7, "execvp(argv[0], argv)", fc=LIGHT, ec=WARM)
    event(ax, cxR, 6.8, "(child runs program)", fc="#FAF6EC", ec=GHOST)
    event(ax, cxR, 6.0, "_exit(status)", fc=LIGHT, ec=WARM)

    event(ax, pxR, 7.6, "ks_introspect_read_fds", fc=LIGHT, ec=SAGE)
    event(ax, pxR, 6.6, "wait4(pid,&st,0,&rusage)", fc=LIGHT, ec=TEAL)

    a = FancyArrowPatch((cxR - 0.6, 6.0), (pxR + 0.6, 6.6),
                        arrowstyle="-|>", mutation_scale=12,
                        color=ARROW, linewidth=1.2, zorder=2)
    ax.add_patch(a)

    event(ax, pxR, 5.6, "clock t1 + format table", fc=LIGHT, ec=SAGE)
    event(ax, pxR, 4.8, "fputs(table) + return KS_OK", fc=SAND)

    # ----------------------------------------------------------------------
    # PROBE annotations --- placed in dedicated band below both panels
    # ----------------------------------------------------------------------
    ax.text(8.0, 3.85, "Probe points",
            ha="center", va="center", fontsize=11, weight="bold", color=NAVY)

    probe_card(ax, 2.5, 2.7, 1,
               "clock_gettime(CLOCK_MONOTONIC)\nbrackets entire transaction\n[markers (1) on right panel]")
    probe_card(ax, 8.0, 2.7, 2,
               "child opens /dev/fd between\nfork and exec, writes via pipe\n[marker (2) on right panel]")
    probe_card(ax, 13.5, 2.7, 3,
               "wait4 returns per-child rusage\natomically with the reap\n[marker (3) on right panel]")

    # Inline pointers on the timeline (no crisscross connectors). The
    # probe cards' bodies match the timeline labels by keyword:
    #   PROBE 1 -> "pipe(pipefd) + clock t0" and "clock t1 + format table"
    #   PROBE 2 -> "ks_introspect_child_snapshot"
    #   PROBE 3 -> "wait4(pid,&st,0,&rusage)"
    # Adding small (1)/(2)/(3) markers next to those events instead.
    def marker(x, y, n):
        ax.text(x, y, f"({n})", ha="center", va="center",
                fontsize=9, weight="bold", color="#8C6213")
    marker(pxR + 1.7, 9.6, 1)   # clock t0 line
    marker(pxR + 1.7, 5.6, 1)   # clock t1 line
    marker(cxR + 1.7, 8.6, 2)   # child snapshot
    marker(pxR + 1.7, 6.6, 3)   # wait4

    plt.tight_layout()
    out = HERE / "process_model.png"
    fig.savefig(out, dpi=180, bbox_inches="tight",
                facecolor="white", edgecolor="none")
    print(f"wrote {out}")


if __name__ == "__main__":
    main()

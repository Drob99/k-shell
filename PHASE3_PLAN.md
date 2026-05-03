# K-Shell — Phase 3 Plan

**Goal:** lock in 100% on Phase 3 (30 pts, due 2026-05-04 23:59 AST).

This plan is built from three inputs:
1. The Phase 1 proposal (what we promised: §1.1–§1.4, pipes/redir/bg/quotes/env explicitly excluded as MVP cuts)
2. The Phase 2 report and code (what we delivered: full §1.1–§1.4, 22 unit tests, ASAN-clean, zero-warning build)
3. Phase 2 grader feedback: **add framework diagram, add results table, make more presentable** — plus the professor's standing emphasis on **novelty and efficiency/speed**

---

## 1. Where we are

| Area | Status |
|---|---|
| §1.1 REPL loop | Done — `src/main.c`, EINTR-safe, 1024-char input cap, drain-on-truncate |
| §1.2 fork/execvp/waitpid | Done — `src/executor.c`, errno→exit-code routing, `_exit` not `exit` |
| §1.3 cd / exit / help | Done — `src/builtins.c`, plus `history` (over-delivery from §1.4) |
| §1.4 dynamic prompt | Done — `src/prompt.c`, HOME→`~` with boundary check |
| §1.4 command history | Done — `src/history.c`, ring buffer, dup suppression |
| Tests | 22 / 22 pass (parser 12, builtins 4, history 6) |
| Memory safety | ASAN clean across normal + edge paths |
| Build hygiene | Zero warnings under `-Wall -Wextra -Wpedantic -Werror` |
| Phase 2 report | 9 figures, 234 lines, eisvogel PDF — but is a *progress* report, not final |

LOC: ~993 across `src/`, `include/`, `tests/`. Small enough to read end-to-end in the report.

---

## 2. Where we need to go

### 2.1 Phase 3 rubric coverage matrix

| Rubric requirement | Current state | Gap |
|---|---|---|
| Introduction | Embedded in "Project Recap" | Reframe as standalone Intro |
| Problem statement & motivation | Embedded in recap | Pull out, expand the *why* |
| System Design — architecture diagrams | One ASCII pipeline diagram | **Need a real graphical framework diagram + a module dependency diagram** |
| System Design — modules explanation | M0–M6 milestones (chronological) | Reframe as module reference (per-file) |
| Implementation — algorithms used | Scattered across "Design Highlights" | Consolidate: ring buffer, EINTR loop, errno dispatch, tri-state returns |
| Implementation — tools/technologies | **Missing** | Add: C11, clang, GNU Make, AddressSanitizer, POSIX libc, pandoc/eisvogel, git |
| Results & testing — test cases | 22 tests documented in prose | **Need a results table** (test name, what it covers, pass/fail, runtime) |
| Output screenshots | 9 figures in place | Keep, possibly add benchmark figure |
| Performance | Memory + zombies only | **Add benchmark numbers — this is also our novelty/efficiency play** |
| Challenges & solutions | "Challenges Faced" section exists | Keep, sharpen |
| Future improvements | **Missing** | Add: pipes, redirection, bg jobs, quoting, env expansion, line editing |
| Team contribution | Done | Keep |
| README — how to run + requirements | Done | Verify still accurate after any code changes |

### 2.2 Professor's priorities (novelty + efficiency)

What we already claim (micro-engineering novelty, all real, all defensible):
- Tri-state return code convention (`KS_EXIT` positive sentinel)
- Parameterized ring-buffer capacity for testability
- Stale-errno disambiguation in REPL on EOF vs EINTR
- HOME-prefix boundary check in prompt rendering
- Async-signal-safe SIGINT handler (`write(2)` not `printf`)
- EINTR-retry loop around `waitpid`

What we're adding to make novelty *visible* to a non-systems reader:
- **Benchmark harness** (see §3.3) producing hard numbers on parser throughput, fork+exec round-trip, REPL fixed cost — compared against bash on the same machine
- **Persistent history** with cross-session deduplication (optional Tier 3)

### 2.3 Phase 2 feedback explicitly addressed

| Feedback | Action |
|---|---|
| Add framework diagram | Build a proper graphical diagram (Mermaid → PNG, or hand-drawn → exported), embed in §System Design |
| Add results table | Build the test-results table AND the benchmark-results table |
| Make more presentable | Restructured headings, consistent figure captions, table-of-figures, polished cover page, no orphaned sub-bullets |

---

## 3. Strategy — three tiers

### Tier 1 — Required for full marks on rubric (must land)

T1.1. **Reframe report → final report** at `report/final_report.md`. Section order: Intro → Problem & Motivation → System Design (architecture + modules) → Implementation Details (algorithms + tools) → Results & Testing → Performance → Challenges & Solutions → Future Improvements → Team Contribution. Keep eisvogel template.

T1.2. **Framework diagram**. Two diagrams actually:
   - *Module framework diagram* — boxes for Parser/Dispatcher/Executor/Built-ins, with History+Prompt as side modules, arrows labeled with the contract (`int return code`, `argv[]`, etc.)
   - *Process model diagram* — fork/exec/wait timeline showing parent/child, SIGINT delivery to the foreground process group

T1.3. **Results table** for unit tests. Columns: # | Test name | Module | What it asserts | Result | Runtime (ms).

T1.4. **Tools & technologies** subsection. Concrete versions (`clang --version`, `make --version`).

T1.5. **Future improvements** section. Prioritized list mapping back to the proposal's "out of scope" list.

T1.6. **Polish pass**: consistent figure captions ("Figure N — ..."), monospace for all code identifiers, no widow lines, list of figures at front.

T1.7. **README verification** — confirm `make`, `make test`, `make asan` still work after any changes.

T1.8. **Rebuild PDF** via existing pandoc + eisvogel pipeline.

### Tier 2 — Novelty (the play for 100%)

**Embedded OS-introspection mode.** A `--inspect` modifier that turns every command into a kernel-eye-view report. The novelty bar the professor cares about is "not done before at course-shell level" — micro-engineering decisions and bash benchmarks don't clear it. Showing the *kernel cost* of every command does.

T2.1. **Introspection module** (`src/introspect.c`, ~80–120 LOC).
   - Pre-exec snapshot in the child: enumerate `/dev/fd` to capture inherited file descriptors right before `execvp`; write the list back to the parent through a shared pipe.
   - Post-wait snapshot in the parent: `getrusage(RUSAGE_CHILDREN)` deltas → user-CPU, sys-CPU, max RSS, page faults (minor/major), voluntary/involuntary context switches, FS block reads/writes.
   - Pretty-printer that lays out the metrics as a labeled table.

T2.2. **Wire-in via `--inspect` flag.** The dispatcher strips `--inspect` from `argv` before exec, sets a per-command flag. Default path (no flag) is byte-for-byte unchanged — zero risk to the working shell.

T2.3. **Mini-benchmark** (`bench/bench_fork_exec.c`, ~30 LOC). One benchmark: fork+execvp+waitpid round-trip latency, mean/p50/p99 over 1000 iterations. Honest framing: we're quantifying the kernel cost of process creation, not racing bash. Output goes in the Performance section as a side table.

T2.4. **Performance section** in report. Combines the bench numbers with an introspection-mode table showing what `ls`, `cat /etc/hosts`, `sleep 1`, and a built-in actually cost the kernel. This *is* the novel contribution.

### Tier 3 — Stretch novelty (only if Tier 1+2 are in by EOD)

T3.1. **Persistent history** — load `~/.kshell_history` on startup, append on `exit`. Dedup against the last N entries on load. Adds ~40 LOC to `history.c`, ~10 to `main.c`. Genuinely novel relative to Phase 1's "in-memory tracking mechanism" wording.

T3.2. **`time` builtin** — wraps an external command, prints `getrusage(RUSAGE_CHILDREN)` deltas. ~50 LOC. Demonstrates a kernel-level concept (process accounting) the proposal didn't promise.

**Default: skip Tier 3.** Only pick T3.1 *or* T3.2 if Tier 1+2 are done by ~6pm with confidence. Today is the due date — risk profile favors landing T1+T2 cleanly over adding code that could destabilize the build.

---

## 4. Branching & merge strategy

```
main (Phase 2 final, frozen)
  │
  └── phase3 (integration branch — what we ship from)
        │
        ├── phase3/report-restructure   ──┐
        ├── phase3/framework-diagrams   ──┤  T1 sub-branches
        ├── phase3/results-table        ──┤  each PR-merged into phase3
        ├── phase3/future-improvements  ──┘
        │
        ├── phase3/benchmark-suite      ──── T2: benchmarks + perf section
        │
        └── phase3/persistent-history   ──── T3 (optional)
```

Rules:
- `phase3` is created off `main` (already done — we're on it now).
- Every Tier task gets its own sub-branch off `phase3`. Small, focused, mergeable.
- Each sub-branch merges back into `phase3` via fast-forward or `--no-ff` merge with a one-line summary commit.
- We do **not** touch `main` until Tier 1+2 are fully on `phase3`, the PDF builds, all tests pass, the demo session works clean.
- Final step: `git checkout main && git merge --no-ff phase3` with the Phase 3 submission commit message. Tag as `phase3-final`.
- If the local `phase3` branch and remote `origin/main` diverge for any reason, prefer rebasing `phase3` onto `main` rather than force-pushing — we want a clean linear history for the grader to read.

---

## 5. Step-by-step execution

Dependency-ordered. Code lands first because the report needs to *describe* it; diagrams and tables come after the data exists.

### Phase A — Lock plan (5 min)

**Step 0.** Commit `PHASE3_PLAN.md` on `phase3` branch. Both of us aligned before any code moves.
- Branch: `phase3`
- DoD: plan committed, scope frozen.

### Phase B — Ship the introspection feature (Tier 2, ~2.5 hr)

**Step 1.** Branch off: `phase3/introspection-mode`. Add `src/introspect.c` skeleton with three public functions in `kshell.h`:
- `ks_introspect_begin(int *out_pipe_fd)` — parent-side: open a pipe for child→parent FD-snapshot transfer.
- `ks_introspect_child_snapshot(int pipe_fd)` — child-side, after fork before exec: enumerate `/dev/fd`, write to pipe.
- `ks_introspect_finalize(pid_t, int pipe_fd, struct rusage *delta, char *out_buf, size_t buflen)` — parent-side, after waitpid: read FD snapshot from pipe, format table into `out_buf`.

DoD: builds clean under `-Werror`, all 22 existing tests still pass, no behavior change to default path.

**Step 2.** Wire `--inspect` flag.
- Dispatcher detects `--inspect` anywhere in `argv`, strips it, sets a static flag.
- `ks_execute` checks the flag. If set, calls `getrusage(RUSAGE_CHILDREN)` before+after waitpid (delta = post − pre), opens the pipe, calls `ks_introspect_child_snapshot` in child between fork and exec, calls `ks_introspect_finalize` in parent, prints the table.
- If `--inspect` not set: zero new code path executes. Existing `ls` is byte-for-byte identical.

DoD: `./kshell` → `ls --inspect` prints a labeled table. `./kshell` → `ls` unchanged. Existing tests still 22/22 green.

**Step 3.** Unit-test the formatter.
- `tests/test_introspect.c` — feeds synthetic `struct rusage` values into the print function, asserts the rendered string contains the expected fields.
- 4–6 tests covering: zero-rusage, all-fields-set, FD list empty, FD list with stdin/stdout/stderr.

DoD: `make test` runs 26+ tests, all green.

**Step 4.** Manual smoke + screenshot.
- Interactive session: `ls --inspect`, `cat /etc/hosts --inspect`, `sleep 1 --inspect`, `pwd --inspect` (built-in path doesn't fork — should report "no child", graceful), `nosuch --inspect` (exec failure path).
- Screenshot: `report/screenshots/07_inspect_mode.png`.

DoD: 5 commands tested, screenshot saved.

**Step 5.** ASAN soak.
- `make asan && ./kshell-asan` interactive session including `--inspect` paths, exit cleanly.
- Verify zero leaks reported on exit.

DoD: ASAN clean.

**Step 6.** Merge `phase3/introspection-mode` → `phase3`.

### Phase C — Mini benchmark (~45 min)

**Step 7.** Branch `phase3/bench-mini`. Add `bench/bench_fork_exec.c`:
- Loop N=1000 iterations of fork + execvp(`/usr/bin/true`) + waitpid.
- Use `clock_gettime(CLOCK_MONOTONIC)` for timing.
- Print mean / p50 / p99 / min / max in microseconds.
- Add `make bench` target.

DoD: `make bench` produces a table of latency numbers. Save output to `report/bench_results.txt`.

**Step 8.** Merge `phase3/bench-mini` → `phase3`.

### Phase D — Report restructure (~2 hr)

**Step 9.** Branch `phase3/report-restructure`. Create `report/final_report.md` with full Phase 3 rubric skeleton:
1. Introduction
2. Problem Statement & Motivation
3. System Design (architecture + per-module reference)
4. Implementation Details (algorithms + tools/technologies)
5. Results & Testing (test suite + introspection demo + bench numbers)
6. Performance Analysis
7. Challenges & Solutions
8. Future Improvements
9. Team Contribution
10. Appendix: build & run instructions

Each section gets a one-line stub. The eisvogel template stays.

**Step 10.** Migrate Phase 2 prose. The M0–M6 chronological narrative is reframed as a per-module reference (one subsection per `src/*.c` file). Content stays, framing changes.

**Step 11.** New content:
- Introduction (recast from Project Recap)
- Problem Statement (the *why*: why a custom shell vs using bash)
- Implementation → Algorithms subsection (ring buffer, EINTR retry loop, errno→exit-code mapping, tri-state return convention)
- Implementation → Tools & Technologies (clang version, make version, ASAN, pandoc, eisvogel, git, POSIX libc, macOS Darwin 25.3.0)
- Implementation → Introspection Module subsection (the novelty — what it does, how it works, why it matters)
- Performance Analysis (bench numbers + interpretation; introspection table for sample commands)
- Future Improvements (pipes, redirection, backgrounding, quoted strings, escape chars, env expansion, line editing, completion, persistent history)

**Step 12.** Merge `phase3/report-restructure` → `phase3`.

### Phase E — Diagrams (~1 hr)

**Step 13.** Branch `phase3/diagrams`. Build two diagrams:
- **Module framework diagram** — boxes for each `src/*.c` file, arrows labeled with the public function and return type. Side modules (History, Prompt, Introspect) shown as services to the pipeline. Mermaid source → exported PNG.
- **Introspection probe-point diagram** — fork/exec/wait timeline showing parent and child swimlanes, with probe points marked: `getrusage(pre)` in parent, `/dev/fd` snapshot in child between fork and exec, `getrusage(post)` in parent after waitpid. This visually justifies the novelty.

Save to `report/screenshots/diagrams/framework.png` and `introspect_flow.png`. Embed in System Design section.

**Step 14.** Merge `phase3/diagrams` → `phase3`.

### Phase F — Tables (~45 min)

**Step 15.** Branch `phase3/tables`. Add three tables to the report:
- **Test results table** — # / name / module / what it asserts / pass/fail / runtime (26+ rows).
- **Introspection demo table** — sample commands (`ls`, `cat /etc/hosts`, `sleep 1`, `pwd`) × metrics (user-CPU µs, sys-CPU µs, max RSS KB, page faults, ctx switches, FDs inherited). Captured from actual runs.
- **Bench latency table** — fork+exec round-trip mean / p50 / p99 / min / max.

This addresses the Phase 2 "add results table" feedback explicitly and triply.

**Step 16.** Merge `phase3/tables` → `phase3`.

### Phase G — Polish & PDF (~1 hr)

**Step 17.** Branch `phase3/polish`. Run a presentability pass:
- List of Figures and List of Tables at front (eisvogel supports `lof` and `lot` flags).
- Consistent figure caption format ("Figure N — short description").
- Cover page polish: confirm group members, course code, semester, submission date.
- Walk every section: no orphan bullets, no broken refs, no rotted "Phase 2" wording.

**Step 18.** Rebuild PDF: `pandoc report/final_report.md -o report/final_report.pdf --template=report/eisvogel.latex --pdf-engine=xelatex --listings --toc --lof --lot`.

**Step 19.** Eyeball pagination — no page-break weirdness on tables or code blocks.

**Step 20.** Merge `phase3/polish` → `phase3`.

### Phase H — Final integration (~30 min)

**Step 21.** From a clean clone (or `make clean`), verify:
- `make` builds with zero warnings.
- `make test` passes 26+/26+.
- `make asan && ./kshell-asan` interactive session leaves zero leaks.
- `make bench` produces numbers.

**Step 22.** Update `README.md`: add `make bench`, mention `--inspect` flag.

**Step 23.** Final merge: `git checkout main && git merge --no-ff phase3 -m "phase3: final submission"`. Tag: `git tag phase3-final`.

**Step 24.** Build submission bundle: zip containing `k-shell/` source tree + `final_report.pdf`. Confirm zip extracts and builds on a fresh checkout.

DoD: every box in §7 (Definition of Done) checked.

---

### Time budget

| Phase | Wall time |
|---|---|
| A. Lock plan | 5 min |
| B. Introspection feature | 2.5 hr |
| C. Mini benchmark | 45 min |
| D. Report restructure | 2 hr |
| E. Diagrams | 1 hr |
| F. Tables | 45 min |
| G. Polish & PDF | 1 hr |
| H. Final integration | 30 min |
| **Total** | **~8.5 hr** |

Your hands-on time: ~30 min total — reviewing diffs at end of B/D/G, running benchmarks on your hardware (so the numbers are honest, not from my sandbox).

---

## 6. Risk register

| Risk | Likelihood | Mitigation |
|---|---|---|
| Benchmark code introduces a regression | Low | `bench/` is a separate target with its own `main()`. Production binary unchanged. Run `make test` after every commit. |
| PDF build fails on new diagrams | Med | Keep diagrams as plain PNG (not inline TikZ). Test pandoc build after each diagram lands. |
| Persistent history (T3) breaks ASAN | Med-High | That's why T3 is optional. Skip unless we have an hour of slack. |
| Framework diagram looks AI-generated / generic | Med | Use the *actual* module names from `kshell.h`, not generic "Parser/Lexer/Eval". Label arrows with real return types. |
| Grader counts micro-novelty as "not novel enough" | Med | Tier 2 benchmark numbers are the hedge — concrete µs/ns figures are hard to argue against. |
| Time crunch (due tonight) | High | Tier 3 dropped by default. Tier 1+2 fits in the budget. |

---

## 7. Definition of done

Phase 3 ships when **all** of the following are true:

- [ ] `final_report.pdf` exists, builds clean, every Phase 3 rubric section is present
- [ ] Two diagrams (framework + process model) are real PNGs, embedded in the report
- [ ] At least one results table (unit tests) and one benchmark table are in the report
- [ ] `make`, `make test`, `make asan`, `make bench` all green on a fresh clone
- [ ] `phase3` branch merged into `main`, tagged `phase3-final`
- [ ] Submission bundle assembled (final PDF + code zip)
- [ ] Phase 2 grader feedback items each have a corresponding visible change (diagram, table, polish pass)

---

## 8. Open questions for you

1. **Confirm Option A (introspection mode) is the chosen novelty.** ✓ if yes, we proceed; otherwise call it before Phase B starts.
2. **PDF build environment** — your Mac (you already have the eisvogel template + xelatex) or Docker pandoc?
3. **Submission format** — Blackboard says "Upload Files" plural. One zip, or PDF + code zip separately?
4. **Benchmark machine** — Mac only is fine (report says "tested on macOS, Apple Silicon").

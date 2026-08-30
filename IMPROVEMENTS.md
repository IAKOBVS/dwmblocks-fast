# Codebase Audit — Improvements & Test Gaps

Generated 2026-08-23 by automated exploration of HEAD `bb57d60` + working tree.
Findings marked **[verified]** were reproduced empirically with GCC 16.2.1 / glibc 2.44 in `/tmp/opencode` scratch builds (no workspace files modified).

## Status (updated after first implementation pass)

**FIXED in this pass** — M1, M2/B1, M3/B2, M4 (+ regression test edge 9), E1 (incl. bare `DIE()`), S1, S2, TS0, TS1 (stress-test SIGUSR1 noise → unused RT signal; was racing with SIGTERM delivery), obs.h prototype mismatch, M8 div-by-zero guards, M6 `localtime` NULL check, PT7 midnight/noon meridiem, PT8 "Agu" typo, E5 doubled semicolon, dup webcam include, B6 clean leftovers, B8 .PHONY gaps, `-Wempty-body` and `-Wsign-compare` warnings, PID buffer sizing (`sizeof * 8` → `* 3` in procfs.c/obs.c).
**Reverted by decision:** bounded `u_ulltoa_p_b()` + dst_size honoring in disk.c/ram.c — writers stay unbounded; block writers implicitly require adequately sized dst (production rows are 32 bytes). Edge-4 test reworked to a realistic buffer accordingly. M5 remains open for the remaining blocks.
**Verification:** full clean rebuild = 0 warnings; `make test-all` exit 0; stress tests 5/5 stable runs; NVML fallback path (`USE_NVML_DEVICEGETTEMPERATUREV=0`) compiles; header-touch rebuilds verified (utils.h → 9 objects rebuilt; config.h touch → scripts regenerated).

**Third pass (2026-08-23):** E2 audio sentinel collision fixed — `b_read_audio_alsa_vol` returns `-1` on failure (percent can't be negative); the mic path's `vol == 1` suppression hack removed so a genuine 1% mic level now displays; speaker/mic writers degrade to icon-only rows with retry intervals on error. PT9: date-block interval clamped instead of wrapping (~5.8 h spurious re-runs). PT3: false `.POSIX:` claim dropped (GNU make required; AGENTS.md corrected). **New P0 tests:** `tests/test-time.c` — TZ-pinned content tests (midnight=12 AM, noon=12 PM, 1 PM conversion, full date oracle incl. month names) with fork isolation per case (the production `utc_off` cache freezes the first TZ) and clock-tick staleness guards; would have caught the "Agu"/midnight bugs. `tests/test-status.c` — includes the production TU directly to test statics: comparator ordering, `g_getcmds_init` interval sort + `internal_tostatus_idx` permutation integrity + OBS order constraint, and slow-path status composition with pads/empty-row skipping. Edge-10 utils boundary matrix added; **it immediately caught live bug M9**: `u_utoa_le3_p(>999)` emitted non-digit garbage from truncated division (>999 W systems got corrupted output). Fixed by contract ownership, not a silent fallback: `u_utoa_le3_p` asserts `num <= 999` (free under `-DNDEBUG`) and the one violating caller — `b_write_cpu_usage_power`, watts are unbounded on HEDT — switched to `u_utoa_p`. Exhaustive sweep of the contract domain (0..999) verified exact. All wired into `make test-all`.

**Fourth pass (2026-08-24): event-driven scheduler replacing the 1 Hz tick.** Old mainloop: `pselect(1s)` + full block-array sweep every second regardless of need; `g_time += 1` assumed each iteration cost exactly 1 s (drifted with render time). New: absolute CLOCK_MONOTONIC deadlines (`g_sched_ns`), sleep until the nearest due block. `g_wake_min` (smallest countdown) is maintained by whichever pass touches countdowns — `g_getcmds`, `g_getcmds_sig`, `b_init` — so the mainloop never rescans; countdown math lives solely in `g_ticks_advance` (ns remainder carried across passes → ticks track wall time exactly). **Signal realtime preserved (after a caught regression):** the first version retried its sleep on EINTR, deferring block updates until deadline expiry; fixed by returning from `g_sleep_till` on interruption and deriving the deadline as a pure function of state (`g_next_deadline()` = tick-bucket start + `g_wake_min + 1`), so early signal passes neither lose nor extend the wait. Measured end-to-end: volume change + `RTMIN+1` mid-wait → status write ~50 ms later (incl. the amixer call), periodic ticks undisturbed. Restart (`b_init`) zeroes sleeps for an immediate refresh; missed deadlines (stall/suspend) collapse into one catch-up pass. **Benchmarked per policy** (12 s windows, strace-counted pselect6, 3 trials): old 11/12/11 wakeups vs new **5/5/5** on this config (min interval 2 s); sys CPU over 20 s: 0.17 s → 0.12 s. Configs with only slow blocks win proportionally more.

*Verification note:* a `[test 2] child killed by signal 6` flake during re-verification was environmental — `/tmp` disk-quota exhaustion (gcc couldn't write temp files), not scheduler behavior; reproduces cleanly and exits 0 once quota is freed. Also fixed standing warnings: `-Wsign-compare` in tests/test-stress.c:337 and unused `dst_size` in blocks/audio.c `b_write_speaker_vol`; full clean rebuild now emits 0 warnings.

**Still open:** R6 audio-alsa descriptor-table unification (unblocked now that E2 landed; needs hardware validation), R9 status-composer clarity pass, M5 dst_size invariant (documented), PT2 BSD include flags, PT4 RT-detection hardening note, S3/S4, DOC2 README refresh, remaining P0 batch items (real signal-mask dispatch via TEST hooks).

**Second pass (2026-08-23):** P1 startup sleep-retry loops removed (`b_cpu_init` single attempt; gpu NVSPEED loop deleted) — no more up-to-20 s frozen bar at startup on machines without RAPL/temp sensors. E3/M7: temp blocks render `?` placeholder instead of DIE on missing sensor/short read; retry happens naturally next tick. P4: `g_getcmds_sig` now renders into tmp and suppresses unchanged output (mirrors `g_getcmds`). PT1: `<bits/time.h>` removed. M10: `atoi` → `u_atou10` in procfs scan. E4: `b_proc_read_file` checks read error before close error. Dead code: macros.h unlocked-io zoo deleted (269→152 lines; only `io_fileno` was used), unused `SIGPLUS`, `U_ZIB..U_QIB` + commented branches, stale FIX comment, duplicate clang-format marker. D1: shared `b_proc_pid_path()` deduplicates procfs.c/obs.c path building. Docs: AGENTS.md `-fanalyzer` and install-description drift fixed; TODO.md items 1–3 marked done.

---

## 1. Fix now (high severity)

### M1. Compile error in NVML fallback path [verified]
- `blocks/gpu-nvidia.c:98` — `return nvmlDeviceGetTemperature(buf.dev, sensorType, temp);`
- Inside the `#else` of `#if USE_NVML_DEVICEGETTEMPERATUREV`. No `buf` exists in scope; the parameter is `dev`. Disabling `USE_NVML_DEVICEGETTEMPERATUREV` (sanctioned by `config.def.h:36`) breaks the build.
- **Fix:** `buf.dev` → `dev`.

### M2 / B1. Incremental builds keep stale objects for half the codebase
- `Makefile:163` attaches header prerequisites (`$(REQ_H)`) only to `$(OBJS)`, `main.o`, `test.o`. The REQ objects — `blocks/temp.o`, `blocks/procfs.o`, `blocks/shell.o` (`Makefile:57-60`) — have **zero header deps**, and no `blocks/*.h` appears anywhere.
- Editing `macros.h`, `utils.h`, `config.h`, or `blocks/procfs.h` relinks against stale objects → silent misbehavior.
- **Fix:** auto-generated deps (`-MMD -MP` + `-include $(OBJS:.o=.d) $(REQ:.o=.d)`), or minimally give every object `$(REQ_H)` + `blocks/*.h`.

### M3 / B2. Stale generated scripts embed old signal numbers
- `Makefile:165-166`: target `$(SCRIPTS)` has no prerequisites. Changing `SIG_AUDIO`/`SIG_OBS`/… in `config.h` leaves installed scripts doing `pkill -RTMIN+<old-N>` — signals silently stop working.
- **Fix:** `$(SCRIPTS): $(CONFIG) updatesig` or a stamp file.

### M4. `value_get()` length bookkeeping is wrong [verified]
- `blocks/procfs.c:36-44`: loop update `value_len -= value - procfs_buf` subtracts the absolute offset from buffer start cumulatively each iteration while `value` keeps advancing — scan window shrinks faster than data.
- Repro: input `"xx xx xx xx xx xx KEY:v"` returns NULL instead of `"KEY:v"`.
- Currently latent (only RAM's `MemAvailable` near top of meminfo, `ram.c:102`), but any future key past ~first few hundred bytes is silently lost.
- **Fix:** track previous pointer and subtract delta, or restructure as `while ((size_t)(end - value) >= key_len)`.

### E1. `DIE()` silently degrades under `-DNDEBUG`; malformed call sites
- `macros.h:38-44`: with `NDEBUG`, `assert(0)` vanishes and execution falls through:
  - Bare `DIE();` at `blocks/audio-alsa.c:159` continues into following `if` with undefined state;
  - `DIE_DO(b_gpu_err())` sites (`blocks/gpu-nvidia.c:110,113,117,127,136,146,162`) free `b_gpu.buf` without NULL-ing (`gpu-nvidia.c:72-77`) → double-free/UAF if execution continues.
- Also `blocks/gpu-nvidia.c:107`: `DIE_DO(nvmlErrorString(b_gpu.ret));` evaluates and **discards** the error string — actual NVML message never printed.
- **Fix:** make DIE NDEBUG-proof (forbid parameterless DIE); print NVML string properly; NULL after free.

### S1. Non-atomic read-then-clear of signal mask; docs claim otherwise
- `dwmblocks-fast.c:504-505`:
  ```c
  const sig_atomic_t mask = g_signal_mask;
  g_signal_mask = 0;
  ```
  RT-signal handlers can't fire here (blocked outside `pselect`), but **SIGHUP is never blocked** (`g_init_signals`, lines 305-347) and `g_handler_restart` writes `g_signal_mask = ~0` (line 570). HUP landing in that window loses pending RT bits.
- **Doc drift:** AGENTS.md claims atomic `__sync_fetch_and_and` clear — no such builtin exists in the tree [verified].
- **Fix:** `mask = __sync_fetch_and_and(&g_signal_mask, 0);` (or block SIGHUP too); update AGENTS.md.

---

## 2. Next (medium severity)

### Correctness

| # | Issue | Location | Fix |
|---|---|---|---|
| S2 | `G_SIGNAL_MAX` computed from macro defined 20 lines later → always 31, real RT range is 30. Config `.signal = 31` passes `b_init` then aborts at startup | `dwmblocks-fast.c:36-40` vs `55-57`; test uses 30 (`tests/test-stress.c:94`) | move `HAVE_RT_SIGNALS` def above line 36 |
| M5 | Block functions ignore `dst_size`; `b_init` accepts pads equal to row size (`pad_len > sizeof(...)` should be `<`) | `dwmblocks-fast.c:183-185`; `time.c:93`, `obs.c:79`, `cpu.c:157-159`, `ram.c:118-120`, … | strict `<`, clamp returned ptr, honor `dst_size` |
| E2 | Sentinel collisions: ALSA vol returns **1** on init failure — indistinguishable from real 1% volume; mic path suppresses genuine 1% (`audio.c:64-68`); muted() overloads 0 | `audio-alsa.c:121-125,164`; `audio.c:42,64-68` | out-param / separate status return |
| E3/M7 | Transient read failures assert-abort the whole bar (short temp reads, failed `/proc/stat` reads, webcam, ram) | `temp.c:30-38,50-61`, `cpu.c:60-65`, `webcam.c:37-38`, `ram.c:54-59` | render placeholder ("N/A"), retry next tick |
| M8 | Division-by-zero windows: `(curr-last)` jiffy diff may be 0 → `(int)` cast of inf is UB; same for `clock_diff==0` RAPL power | `cpu.c:82,139,143` | guard denominators |
| M6 | Unchecked `localtime()` dereferenced; also `tm_gmtoff` cached forever → DST shows time off by 1h until restart | `time.c:34-35,26,41-45` | NULL check; invalidate on `tm_isdst` change |
| §8 | **Prototype mismatch**: header declares 9 params, definition has 10 with different order — survives only because `obs.c` never includes its own header | `blocks/obs.h:25` vs `blocks/obs.c:28` | sync header |

### Performance

| # | Issue | Location | Fix |
|---|---|---|---|
| P1 | Retry loops sleep inside tick loop at startup: up to **20 s frozen bar** on machines lacking RAPL + temp sensor (10 s each) | `cpu.c:38-45,177-181`; `gpu-nvidia.c:62-69` | single attempt + "N/A" + periodic retry via interval mechanism |
| P4 | `g_getcmds_sig()` never compares new content vs stored row — repeated signals trigger full status rewrite | `dwmblocks-fast.c:266-281` | cheap memcmp mirroring `g_getcmds` fast-path (242-247) |
| P5 | OBS initial `/proc` scan O(n-procs) per appearance/disappearance cycle | `procfs.c:183-227` | already TODO item 4 |
| P2/P3 | Fixed 1 s wakeups regardless of schedule; long double/double for integer percentages | `dwmblocks-fast.c:527`; `cpu.c:82,139`, `ram.c:93`, `disk.c:79` | TODO items 5/7 cover these |

### Portability

| # | Issue | Location | Fix |
|---|---|---|---|
| PT1 | Private glibc header `<bits/time.h>` included directly — breaks musl/BSD | `cpu.c:22` | delete (`<time.h>` already included) |
| PT2 | BSD include flags placed in LDFLAGS where they do nothing → BSD support as written cannot compile | `config.def.mk:17-20` | move `-I/usr/local/include` etc. into CFLAGS/CPPFLAGS |
| PT4 | `_POSIX_REALTIME_SIGNALS` detection fragile — defined by glibc only after `<unistd.h>` pulls in posix_opt.h; include reshuffle silently flips to non-RT scheme | `dwmblocks-fast.c:55` | detect `SIGRTMAX > SIGRTMIN`, or define `_POSIX_C_SOURCE >= 199309L` explicitly |
| PT3 | `.POSIX:` claimed but uses `+=`, `$^`, grouped targets, `include config.mk` | `Makefile:20,22,25,29,96,98,165` | drop `.POSIX:` or de-GNUismize |
| S3 | Non-RT fallback maps `.signal=1..4` onto SIGUSR1/USR2/SIGPIPE/SIGALRM — swallowing SIGPIPE process-wide | `dwmblocks-fast.c:62-65` | `#error` instead of risky fallback |

### Tests (suite quality)

| # | Issue | Location |
|---|---|---|
| TS0 | **Incompatible pointer types** (confirmed by LSP): 6 sites pass `unsigned int *` where `unsigned short *interval` param expected — works only on little-endian by accident, reads garbage on BE; also `-Werror`-clean builds break | `tests/test-edge-cases.c:69,90,111,132,154,155` vs decls at `:31,33,41` |
| TS1 | Stress fork tests send SIGUSR1 as "noise", but child doesn't handle it → default disposition kills child → tests 2+3 **fail wherever `probe_binary()` succeeds** [verified on this machine] | `tests/test-stress.c:235,245-248` |
| TS2 | Three divergent max-signal constants across runtime/test/docs (31 / 30 / "G_SIGNAL_BITMASK_MAX") | see S2 |
| B3 | `make check` requires sudo+setcap and fails outright on headless hosts (X11 build's `XOpenDisplay(NULL)` failure → DIE) | `Makefile:73-78`, `dwmblocks-fast.c:400-406` |

---

## 3. Hygiene batch (low)

### Cosmetic bugs visible in the bar
- Month table typo `"Agu "` for August — `time.c:127`.
- Midnight renders `"0:MM AM"` instead of `"12:MM AM"` — `time.c:61-72`.
- Date-block interval overflow: computed seconds reach 86399 but `interval` is `unsigned short` (max 65535) → wraps to ≈20863 s, date re-runs ~every 5.8 h instead of daily — `time.c:138`.
- `u_utoa_le3_p` emits garbage digits above 999 (>999 W systems get corrupted output) — `utils.h:80-87`, caller `cpu.c:167`.
- Doubled semicolon + unchecked write: `dwmblocks-fast.c:562`.

### Dead code / unused inventory

| Item | Location |
|---|---|
| `SIGPLUS` macro never used | `dwmblocks-fast.c:60,63` |
| Typo'd unlocked-stdio wrappers (`fgetb_unlocked`, `getb_unlocked`, …) — none exist in glibc | `macros.h:149,159,174,190,210,225` |
| Entire unlocked-io zoo unreachable (`USE_UNLOCKED_IO*` never defined; all `HAVE_*_UNLOCKED` defined nowhere) + self-referential `#define getc(s) getc(s)` shadowing | `macros.h:143-254` |
| Orphaned `b_write_mic_exists` | `audio.c:79-93`, `audio.h:35-36` |
| Stray data file `dummy_temp` (contains `45000`) referenced nowhere | repo root |
| `#if 0` NVML experiment | `gpu-nvidia.c:152-157` |
| Unused size constants `U_ZIB..U_QIB` | `utils.h:182-185` |
| Stale FIX comment describing already-present change | `dwmblocks-fast.c:330` |
| Duplicate `clang-format off` marker (second should be `on`) | `cpu.c:68,77` |
| Duplicate `#include "blocks/webcam.h"` | `blocks.def.h:24,32` |
| Missing prototypes for externs `b_proc_name_match`, `compare_interval_and_signal` | `procfs.c:103`, `dwmblocks-fast.c:163` |
| `LEN(X)` locally redefined — belongs in macros.h | `dwmblocks-fast.c:67` |
| Unused includes / commented-out code | `time.c:19` (sysinfo), `ram.c:22-23` |

### Code duplication worth factoring
- **D1:** `/proc/[pid]/comm|status` path construction duplicated verbatim — `procfs.c:188-213` vs `obs.c:47-63` → extract `b_proc_pid_path()`.
- **D2:** retry-open fd initializers triplicated with variants — `cpu.c:38-45`, `gpu-nvidia.c:62-69`, `ram.c:38-45`, `disk.c:34-40` → shared helper with policy flag.
- **D3:** temperature-string post-processing duplicated — `temp.c:27-41` vs `43-63`.
- **D4:** humanize-and-print tail duplicated — `disk.c:88-95` vs `ram.c:129-135` → `u_write_humanized()`.
- **D5:** unreachable `(void)x;` suppression idiom ~15× after `return` — prefer unnamed params.

### Build system (low)
- B4: `disable-{cuda,x11,alsa}` sed recipes brittle (assume byte-exact formatting; regex double-comments commented-out lines on repeat runs) — `Makefile:124-149`.
- B6/B8: `clean` misses `tests/*-bin` leftovers; `.PHONY` omits test/disable targets — `Makefile:91-92,183`.
- B9: env `CFLAGS` silently discarded — `Makefile:25,29` (use `?=`).
- B10: `getcpufile` help text names wrong file; unquoted word-splitting; unanchored sed substitution — `getcpufile:3-14`.
- B11: install strips nothing (`# strip` commented) while AGENTS.md says "strip + setcap".

### Documentation drift
- DOC1: AGENTS.md claims atomic mask clear (false), `-fanalyzer` (removed), wrong constant name, wrong install description.
- DOC2: README examples use ancient positional struct API; `README.md:70` says `dwmblocks -p` (wrong binary name).
- DOC3: TODO.md item 1 is already fixed in working tree; items 2, 3, 11 still open as described.

### Warning-flag opportunities
Current clean-build warnings: `-Wempty-body` at `dwmblocks-fast.c:547`, `-Wunused-function` at `gpu-nvidia.c:63`.
Recommended additions that would surface real issues:
- `-Wmissing-prototypes` (two externs listed above)
- `-Wconversion/-Wsign-conversion` (pervasive narrowings, e.g. `(unsigned int)` passed to `u_ulltoa_p(unsigned long long)` at `ram.c:131`, `disk.c:107`)
- `-Wundef` (`XGLIBC_PREREQ` expands undefined token on non-glibc)
- `-fsanitize=address,undefined` on test binaries — would catch M4-class bugs and E1 UAF cheaply

---

## 4. Test coverage audit

### What exists
- 3 Makefile targets (`check`, `test-stress`, `test-edge-cases`, `test-all`) → ~19 subtests across `tests/test-stress.c` (10) and `tests/test-edge-cases.c` (8), plus a `-DTEST=1` smoke iteration via `dwmblocks-fast.c:524-526`.
- Only 4 subtests have real assertion depth: procfs iterator spans, `u_strtoull10` 64-bit regression, RAPL wrap math, interval-mutation range check.
- Good recent additions: RAPL-wrap regression (`test-edge-cases.c:266-299`), 64-bit parse guard (239-256), table-driven iterator test (169-234).

### Top gaps (prioritized)

**P0 — highest coverage per line, all unprivileged:**
1. **Status-string construction harness** — fast/slow paths `dwmblocks-fast.c:350-381` entirely untested.
2. **Real signal-mask unit tests** — stress tests validate copies/mimics of the logic, not the artifact (`dwmblocks-fast.c:500-519,551-573`).
3. **qsort + layout preservation** — interval-sorting and `internal_tostatus_idx` mapping (`dwmblocks-fast.c:163-220`) untested.
4. **TZ-pinned time/date content assertions** — would have caught the `"Agu "` typo and midnight bug today (`time.c:61-72,127`).
5. **utils boundary matrix** — `u_utoa_p`, `u_stpcpy`, `u_humanize` edge cases (zero, UINT max, dst exactly N bytes). Note: existing edge-case test 4 performs an *undetected* stack-buffer-overflow (`test-edge-cases.c:128-133` vs unchecked `u_ulltoa_p`, `utils.h:29-33`).

**P1:**
- `path_sysfs_resolve` rewriter (`path.h:43-101`) — pure function, trivially unit-testable.
- `value_get()` regression for the M4 bug once fixed.
- Real signal handler/dispatch/restart paths behind a TEST ifdef.
- Suite robustness: timeouts on all tests, fix dead FAIL branches under `set -e`, fix TS1 SIGUSR1 flaw.

**P2 — privileged/integration (behind skip guards):**
- RAPL integration with real `cap_dac_read_search` files.
- Hardware smokes (ALSA, NVML) gated on device presence.
- Mocked-procfs per-block unit tests (TODO item 10).

### Hermeticity problems
Tests currently require sudo/setcap, X11+ALSA+NVML link libs, and live `/proc`/`/sys` — not CI-runnable bare. Recommend: unit-test tier with no external deps + optional integration tier.

---

## 5. Benchmark: block dispatch layouts & scheduling (2026-08-23)

Questions: (a) is sorted+SoA (`b_*` parallel arrays) dispatch faster than plain AoS? (b) would SoA win under real cache pressure? (c) is `g_time % interval == 0` scheduling cheaper than per-block sleep counters?

Harness: `tests/bench-blocks.c` (`make bench-blocks`, rounds via `BENCH_ROUNDS=`). Stub writers shaped like real ones; identical logical blocks per variant; median of rounds; production flags (-O2 -flto -march=native); GCC 16.2.1, Zen-family desktop (L1d 32K, L2 ~512K-1M).

Variants: `aos` unsorted records w/ inline sleep counter · `aos-sort` same sorted · `soa-sort` production layout · `soa-mod` modulo scheduling (no counters, `gtime % itv[i] == 0`).

**Dispatch ns/tick (median, one binary):**

| n | aos KB | aos | aos-sort | soa-sort | soa-mod |
|---|---|---|---|---|---|
| 20 (realistic) | 0 | 12.7 | 12.8 | 19.2 | 52.8 |
| 1024 | 40 | 671 | 718 | 977 | 2642 |
| 4096 | 160 | 3072 | 2786 | 3818 | 9990 |
| **16384** | **640 (>L2)** | 16046 | 16513 | **15168 (1.06x)** | 41453 |
| **65536** | **2560 (L3/DRAM)** | 65142 | 66709 | **60761 (1.07x)** | 161918 |

Findings:
- **Cache-pressure crossover confirmed**: below L2-sized working sets SoA loses ~20% (extra AGU work + register spills, see asm in session notes); at n≥16384, once the AoS array exceeds L2, SoA's line-economy engages and it wins 1.06–1.07x. The crossover sits around AoS > L2 (~13-16k blocks).
- **But that regime requires ~16k+ blocks.** A statusbar config has ~20 (800 bytes). Real-world eviction during the 1 s sleep re-warms ~13 cache lines ≈ tens of ns once per second. Layout is irrelevant to this program's performance either way; keep sorted+SoA for ordering semantics only.
- **Modulo scheduling rejected**: ~0.37x of soa-sort at every size (flat ~2.5 ns/block = runtime `div`). It also breaks semantics: dynamic intervals (`b_write_time`/`b_write_date`/obs set `*interval`), staggered firing (modulo aligns all interval-N blocks into bursts), and alignment across `g_time` wrap unless interval divides 2³².
- **Measurement caveat**: absolute numbers shift between binaries (adding an unrelated function moved soa-sort 13.9→19.2 ns/tick via register-allocation luck); relative orderings within a single binary are reliable. Sub-cycle microbenchmarks are fragile — compare variants inside one build.

Conclusion: no layout or scheduler change justified on performance; current design kept for correctness/ordering. Harness exists to vet any future proposal.

---

## 6. Refactor plan: readability, composability, duplication

| # | Item | Action | Status |
|---|---|---|---|
| R1 | temp.c read+strip duplicated in `b_write_tempfd_internal`/`b_write_temp_internal` (D3) | extract `b_temp_format(dst, read_sz)` | done |
| R2 | humanize-and-print tail duplicated in disk.c/ram.c (D4) | extract `u_write_humanized()` in utils.h | done |
| R4 | unreachable `(void)x;` clusters after `return` in ~13 block writers (D5) | move casts to top of function body; mechanical, behavior-preserving | done |
| R5 | cpu.c 7-field `/proc/stat` parse column list | **rejected** — explicit columns document the format better than a field-pointer loop | — |
| R6 | audio-alsa speaker/mic near-duplicates (`vol`/`muted` × playback/capture) | descriptor-table unification — **deferred**: needs ALSA hardware to validate; sentinel redesign (E2) should land first | deferred |
| R7 | fd-init helpers across ram/disk/cpu | **rejected** — different domains (file fd vs mountpoint fd vs retry policy); forced abstraction | — |
| R8 | proc-icon writer unification (obs/webcam/mic-exists) | **rejected** — three different discovery mechanisms (pid-cache vs `/proc/modules` grep vs `/proc/asound/cards` grep); a shared writer would be false abstraction | — |
| R9 | `g_status_get` fast/slow path pointer arithmetic | clarity pass candidate — deferred until fast-path regression tests exist (P0 test batch) | deferred |

Principle applied throughout: composability means extracting genuinely shared *mechanisms* (path building, number formatting), never merging coincidentally similar *policies*.

---

## 7. Positive notes


Worth preserving — these designs are sound:
- Handlers are properly async-signal-safe (`write` + `_Exit` + volatile only; fprintf deliberately avoided, `dwmblocks-fast.c:545`); `sigfillset` + `SA_RESTART` correctly applied.
- Per-second caches keyed on `g_time` (meminfo `ram.c:47-62`, statvfs `disk.c:52-70`), persistent fds for sysfs, ALSA handle reuse with lazy reconnect.
- Incremental fast-path redraw (`dwmblocks-fast.c:371-378`).
- PID-reuse defended against via comm re-validation (`obs.c:65-73`).
- Interval-sort/print-order mapping design.

---

## 8. Recommended action order

1. M1 (one-char fix, unbreaks NVML fallback builds) — DONE
2. M2/B1 + M3/B2 (build-system dependency graph) — DONE
3. M4 (`value_get` math) + regression test — DONE
4. E1 (DIE/NDEBUG semantics + GPU cleanup) — DONE
5. S1 + S2 (signal mask clearing + constant definition order) + AGENTS.md update — DONE
6. TS0 pointer-type mismatches in `tests/test-edge-cases.c` (6 sites) — DONE
7. P0 test batch (status builder, real mask tests, sort/layout, TZ-pinned content, utils matrix)
8. Medium correctness batch (E2 sentinels, E3 placeholder rendering; M8 div guards, obs.h prototype — DONE)
9. Hygiene sweep (typos, dead code, dup factors, README refresh)

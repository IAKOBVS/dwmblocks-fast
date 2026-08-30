# TODO

Status: ○ not started  🔶 partial  ✓ done

## ✓ 1. Fix `main()` guard — init/mainloop inside `#ifdef USE_X11`

`dwmblocks-fast.c:565-575`: The entire `g_status_init()` / `g_status_mainloop()` call chain is inside `#ifdef USE_X11`. Without X11, the program is a no-op (returns immediately). The `-p` stdout path should work without X11.

- **Fix**: Move init/mainloop outside the `#ifdef USE_X11` guard; only wrap the X11-specific `g_init_x11()` call and arg-parsing for `-p`.
- **Pros**: Fixes a functional bug; enables standalone stdout mode without linking X11.
- **Cons**: None.
- **Performance gain**: N/A (correctness fix).

---

## ✓ 2. Fix typo'd unlocked IO macros in `macros.h`

`macros.h:149,159,174,190,210,225`: Several `_unlocked` wrappers use wrong function names:
- `fgetb_unlocked` → `fgetc_unlocked`
- `getb_unlocked` → `getc_unlocked`
- `getwb_unlocked` → `getwc_unlocked`
- `fputwb_unlocked` → `fputwc_unlocked`
- `putb_unlocked` → `putc_unlocked`
- `fputb_unlocked` → `fputc_unlocked`

Currently dead code (`USE_UNLOCKED_IO` not defined in default config), but would break if enabled.

- **Fix**: Correct function names.
- **Pros**: Enables `USE_UNLOCKED_IO` without breakage; removes latent footgun.
- **Cons**: None.
- **Performance gain**: Would reduce stdio locking overhead if `USE_UNLOCKED_IO` is enabled (~5-10% on stdio-heavy paths, none by default).

---

## ✓ 3. Removed duplicate `#include "blocks/webcam.h"` in `blocks.def.h`

`blocks.def.h:24` and `blocks.def.h:32` both include `"blocks/webcam.h"`.

- **Fix**: Remove line 32.
- **Pros**: Cleaner code; avoids redundant guard macro expansion.
- **Cons**: None.
- **Performance gain**: Negligible (< 1 ns per compile).

---

## ○ 4. PID cache with generation counter for `/proc` scans

`blocks/procfs.c:145-189`: `b_proc_exist()` iterates every `/proc/[pid]/` entry. On a system with 500+ processes, this is ~500 `open`+`read`+`close` cycles per scan. OBS blocks trigger this on every signal.

Currently OBS caches the PID once found (`blocks/obs.c:31-46`), but the **initial** scan is always O(n). If the cache is invalidated (process dies), the next scan is again O(n). A generation counter could detect `/proc` changes (by comparing `stat()` on `/proc` mtime or a sequence counter) and only re-scan when needed.

- **Fix**: Add `b_proc_exist_cached()` that validates the cache via a generation counter (e.g., `g_time` comparison) before falling through to the full scan.
- **Pros**: ~99% reduction in `/proc` scan time in steady state; O(1) for cached lookups.
- **Cons**: Adds ~8 bytes of state per cached PID; slightly more complex invalidation logic.
- **Performance gain**: **~90-99%** reduction in `/proc` scan time. Overall program impact is 0.1-1% unless signals fire very frequently.

---

## ○ 5. Fixed-point math for CPU usage

`blocks/cpu.c:81`: CPU usage calculation uses `long double` arithmetic:
```c
const int usage = (int)((long double)100 * ((long double)(curr.cpu_time - last.cpu_time) / (long double)(curr.time - last.time)));
```

`long double` (80-bit extended precision on x86) is overkill for a 0-100 integer percentage. Replace with `uint64_t` scaled-integer math:
```c
const int usage = (int)((curr.cpu_time - last.cpu_time) * 100 / (curr.time - last.time));
```

Similarly, `b_read_cpu_usage_power()` (`blocks/cpu.c:105-106`) uses `double` for power calculation.

- **Fix**: Replace `long double`/`double` with fixed-point integer arithmetic.
- **Pros**: Avoids FPU save/restore overhead; deterministic; no soft-float fallback on archs without hardware FPU.
- **Cons**: Must be careful with overflow (unlikely for 64-bit).
- **Performance gain**: **~40-50%** reduction in CPU usage calculation time. Overall program impact ~0.01% (runs every 2s, already fast).

---

## ○ 6. `writev` for stdout output path

`dwmblocks-fast.c:433`: Already has a TODO comment:
```c
/* TODO: optimize stdout path, use writev from uio.h. */
```

The current stdout path writes `pad_left + content + pad_right + newline` as a single pre-built string. `writev` could avoid building the string upfront by writing from multiple buffers in one syscall.

- **Fix**: Replace `write()` with `writev()` using an iovec array for the status block segments.
- **Pros**: Reduces memcpy overhead in the status string builder (less copying, just point into existing buffers).
- **Cons**: Requires ensuring buffers remain valid during `writev`; marginal complexity increase.
- **Performance gain**: **~5-15%** reduction in status output syscall overhead for stdout mode (negligible for X11 mode).

---

## ○ 7. Timer coalescing (dynamic pselect timeout)

`dwmblocks-fast.c:379-387`: `g_sleep()` always sleeps exactly 1 second. The program wakes every second even if no blocks need to run (e.g., all blocks have 30s+ intervals). Could compute the minimum remaining sleep across all blocks and sleep only until the next block needs to run.

- **Fix**: After `g_getcmds()`, compute `next_sleep = MIN(b_sleeps[i])` for all blocks, cap at `INTERVAL_UPDATE`, pass to `pselect`.
- **Pros**: Reduces unnecessary wakeups; saves power on battery-powered systems.
- **Cons**: Slightly more computation per loop iteration; `pselect` resolution may limit benefit.
- **Performance gain**: **~50-95%** reduction in wakeups depending on block configuration. From always-waking-every-1s to matching actual block schedules.

---

## ○ 8. Cache time formatting

`blocks/time.c:53`: Already has a TODO comment:
```c
/* TODO: optimize, cache time formatting */
```

`b_write_time()` is called every 59s, but `b_write_date()` every 3600s. The time block's output only changes once per minute. Could cache the formatted time string and only regenerate when the minute changes. Also could use `strftime` with a precomputed format string, but the current manual formatting is already fast.

- **Fix**: Cache the last formatted time string and `tm_min`; skip regeneration if minute hasn't changed.
- **Pros**: Eliminates redundant formatting work (typically 59 out of 60 calls are wasted).
- **Cons**: Adds ~64 bytes of cache state.
- **Performance gain**: **~98%** reduction in time block work (wasted work eliminated). Overall program impact negligible.

---

## ○ 9. Prefault signal handler stack

Signal handler `g_handler_sig()` runs with all signals blocked. If the handler's stack page hasn't been touched, a page fault occurs during signal delivery, adding latency and risking failure in low-memory conditions.

- **Fix**: At init, `memset()` a large stack buffer or use `sigaltstack()` with pre-touched alternate stack pages.
- **Pros**: Eliminates page fault latency on first signal delivery; more robust under memory pressure.
- **Cons**: Uses ~4KB of BSS/stack.
- **Performance gain**: Eliminates **~100μs latency spikes** on first signal delivery. Steady-state: 0%.

---

## 🔶 10. Expand test coverage

`tests/test-run`: Only a single smoke test — one mainloop iteration, any non-zero exit = fail. No unit tests for individual block functions.

**Done**: Added `tests/test-stress.c` (10 tests: signal bitmask accumulation, fork/kill rapid-fire stress, rapid re-signalling, edge-case resilience, mock-block edge cases) and `tests/test-edge-cases.c` (5 tests: NULL arg, empty arg, interval mutation, zero-size dst, trivial determinism). Both have run-scripts and `Makefile` targets (`test-stress`, `test-edge-cases`, `test-all`).

**Still missing**: Unit tests for individual block functions (CPU, RAM, disk, GPU, audio, OBS, webcam) with mock `/proc`/sysfs data. No CI integration.

- **Fix**: Add per-block unit tests with mock filesystem for each block function. Parameterize inputs, verify outputs.
- **Pros**: Catches regressions in block parsing logic; enables CI; documents expected behavior.
- **Cons**: Increases maintenance surface.
- **Performance gain**: N/A (reliability). Hidden value: enables safe refactoring.

---

## ○ 11. `u_strstr_len` uses wrong feature guard

`utils.h:167`: `#ifdef HAVE_STPCPY` guards `memmem`, but `memmem` is gated by `_GNU_SOURCE` independently. Should be `#ifdef HAVE_MEMMEM`.

- **Fix**: Change to `#ifdef HAVE_MEMMEM`.
- **Pros**: Correctness on platforms with `stpcpy` but not `memmem`.
- **Cons**: None (both are GNU extensions, practically never separated).
- **Performance gain**: N/A (correctness).

---

## Summary table

| # | Improvement | Category | Status | Est. perf. gain | Effort |
|---|---|---|---|---|---|---|
| 1 | Fix `main()` USE_X11 guard | Bug fix | ○ | N/A | very low |
| 2 | Fix typo'd unlocked IO macros | Bug fix | ○ | N/A\* | very low |
| 3 | Remove duplicate `webcam.h` include | Cleanup | ○ | <0.001% | trivial |
| 4 | PID cache with generation counter | Performance | ○ | 90-99% (/proc scan) | medium |
| 5 | Fixed-point CPU math | Performance | ○ | 40-50% (CPU calc) | low |
| 6 | `writev` stdout path | Performance | ○ | 5-15% (stdout write) | low |
| 7 | Timer coalescing | Performance/Power | ○ | 50-95% (wakeup reduction) | medium |
| 8 | Cache time formatting | Performance | ○ | 98% (time block) | low |
| 9 | Prefault signal stack | Robustness | ○ | 100μs spike elim. | low |
| 10 | Expand tests | Testing | 🔶 | N/A | high |
| 11 | Fix `HAVE_MEMMEM` guard | Bug fix | ○ | N/A | trivial |

\* Would improve stdio paths if `USE_UNLOCKED_IO` is enabled.

## Already implemented (not in original TODO)

These were already in place when the TODO was written, or added in uncommitted working-tree changes:

| Item | Where |
|---|---|
| Signal bitmask (`g_signal` → `g_signal_mask`) | `dwmblocks-fast.c` |
| Bitmask iteration in mainloop | `dwmblocks-fast.c` |
| `SA_RESTART` flag on sigaction | `dwmblocks-fast.c` |
| NULL func pointer check in `b_init()`, `g_getcmds()`, `g_getcmds_sig()` | `dwmblocks-fast.c` |
| `G_SIGNAL_BITMASK_MAX` validation in `b_init()` / `g_getcmds_sig()` | `dwmblocks-fast.c` |
| Signal range check in `g_getcmds_sig()` early return | `dwmblocks-fast.c` |
| `tests/test-stress.c` — 10 stress/edge tests | `tests/test-stress.c` |
| `tests/test-edge-cases.c` — 5 block-function edge-case tests | `tests/test-edge-cases.c` |
| `Makefile` targets: `test-stress`, `test-edge-cases`, `test-all` | `Makefile` |
| Removed `-fanalyzer` from CFLAGS (clangd compat) | `Makefile` |

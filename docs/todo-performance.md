# Performance TODO — Bela Gem Multi / Sushi Reactive

Items to validate before shipping a latency-critical product on PocketBeagle 2 + AM6232.

---

## 1. Verify Twine worker threads are running EVL-RT

**Why it matters**

Twine spawns a pool of worker threads for multicore plugin processing.
On Xenomai 4, those threads must be attached to the EVL core (SCHED_FIFO /
EVL_SCHED_FIFO) or they fall back to Linux CFS, causing unbounded
scheduling jitter that shows up as audio glitches under CPU load.

**How to check**

```bash
# On the running board, list EVL threads:
evl ps -t

# You should see entries like:
#   THREAD           CPU  PRI  STATE
#   sushi-worker-0   1    80   wait
#   sushi-worker-1   1    80   wait
# If they are absent, Twine was built without EVL support.
```

**Fix if absent**

Rebuild libsushi (and the bundled Twine) with:

```
-DSUSHI_BUILD_TWINE=ON -DTWINE_WITH_EVL=ON -DTWINE_WITH_XENOMAI=OFF
```

Confirm at runtime:

```cpp
// In your setup() after sushi->start():
// Twine 0.4+ exposes a query API:
assert(twine::is_current_thread_realtime() == false); // main thread
// Inside render(), assert the opposite:
// assert(twine::is_current_thread_realtime() == true);
```

---

## 2. Enable NEON SIMD code generation for your DSP code

**Why it matters**

The AM6232 Cortex-A53 cores have full ARMv8-A SIMD (NEON / Advanced SIMD).
Without explicit `-march`, GCC defaults to a conservative baseline that
avoids SIMD autovectorisation.  DSP loops (mixing, metering, plugin
processing in Brickworks plugins) run 2–4× faster with SIMD enabled.

**Recommended compiler flags**

Add to your CMake toolchain file or `CPPFLAGS`:

```cmake
# In CMakeLists.txt or toolchain file:
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -march=armv8-a+simd -mtune=cortex-a53")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8-a+simd -mtune=cortex-a53")
```

Or in the Bela IDE **Custom compiler flags**:

```
-march=armv8-a+simd -mtune=cortex-a53
```

**Verify autovectorisation fired**

```bash
# Compile with -fopt-info-vec-optimized and check the output:
cmake .. <your flags> -DCMAKE_CXX_FLAGS="... -fopt-info-vec-optimized"
cmake --build . 2>&1 | grep "vectorized"
```

**Note on libsushi**

libsushi itself is compiled at CMake time; the flags above only apply to
your project code.  To get SIMD in libsushi's audio path, pass the flags
when you build libsushi (Method A / B in `bela-readme.md`).

---

## 3. IRQ affinity — prevent USB/Ethernet from stealing the audio core

**Why it matters**

Linux assigns interrupt service routines (ISRs) to CPU cores by default
using `irqbalance` or a static assignment.  On a dual-core AM6232, if the
USB or Ethernet IRQs land on the same core as Sushi's `render()` thread,
every packet reception adds a worst-case latency spike of 50–200 µs —
enough to cause xruns at 64-frame block sizes.

**Identify your audio thread's core**

```bash
# Find the PID of the render / EVL thread:
evl ps -t | grep sushi
# Or: ps -eLo pid,comm,psr | grep sushi
```

**Move IRQs away from the audio core**

Assume the audio thread runs on CPU 1.  Move all USB and Ethernet IRQs to
CPU 0:

```bash
# List IRQ numbers for USB and Ethernet:
grep -E "usb|eth|dwc" /proc/interrupts

# Set affinity for each IRQ (replace <N> with the IRQ number):
echo 1 > /proc/irq/<N>/smp_affinity   # bitmask: CPU 0 only
```

Make it permanent via a startup script:

```bash
# /usr/local/bin/irq-affinity.sh
#!/bin/sh
for irq in $(grep -E "usb|eth|xhci" /proc/interrupts | awk -F: '{print $1}' | tr -d ' '); do
    echo 1 > /proc/irq/${irq}/smp_affinity 2>/dev/null
done
```

Add this script to your startup sequence (see `todo-shipping.md` for the
full startup hardening checklist).

**Disable irqbalance**

```bash
systemctl disable irqbalance
systemctl stop irqbalance
```

**Verify**

Monitor xruns via Sushi's `/proc` output or the OSC interface while
hammering the USB port (e.g. `dd if=/dev/urandom of=/dev/null` from a USB
stick).  Xrun count should not increase.

---

## Quick-reference checklist

| Item | Command to verify | Status |
|---|---|---|
| Twine EVL threads | `evl ps -t \| grep sushi` | [ ] |
| NEON vectorisation | `-fopt-info-vec-optimized` output | [ ] |
| irqbalance disabled | `systemctl is-enabled irqbalance` | [ ] |
| USB/Eth IRQs on CPU 0 | `cat /proc/irq/*/smp_affinity` | [ ] |
| Audio thread on CPU 1 | `ps -eLo pid,comm,psr \| grep sushi` | [ ] |

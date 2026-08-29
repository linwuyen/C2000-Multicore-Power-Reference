# F28388D Digital Power Learning Roadmap

## Goal

把 YXDSP-F28388D 從 peripheral demo board 轉成一個可長期累積的 digital-power reference platform。

路線優先順序不是「把所有 Lab 跑完」，而是：

```text
board survival
  ↓
deterministic timing
  ↓
synchronous measurement
  ↓
control authority
  ↓
protection authority
  ↓
real plant
  ↓
optimization
  ↓
multicore
```

---

## Phase 0 — Board survival

### Learn

- rails；
- reset；
- JTAG；
- GPIO mux；
- connector orientation；
- PWM buffer path。

### Deliverable

Retained board bring-up checklist + measured rails + confirmed safe GPIO99 behavior。

### Exit condition

Board can be repeatedly powered, connected, reset, halted and run without unexplained external PWM behavior。

---

## Phase 1 — ePWM as timing engine

### Learn

- TBCLK / TBPRD；
- up/down counting；
- CMPA；
- AQ；
- deadband；
- shadow load；
- Trip Zone。

### Experiment

```text
20 kHz
10% → 25% → 50% → 75%
complementary A/B
~500 ns deadtime
```

### Exit condition

Scope proves timing, deadtime, update boundary and trip behavior。

---

## Phase 2 — ADC measurement path

### Learn

- SOC；
- acquisition window；
- EOC；
- ADCRESULT；
- reference / scaling；
- board RC / source impedance。

### Experiment

J7-17 / ADCINA0 static points：

```text
0.5 V
1.5 V
2.5 V
```

### Exit condition

Gain, offset, monotonicity and stability are measured and documented。

---

## Phase 3 — Synchronous control timing

### Learn

- ePWM SOCA；
- ADC trigger timing；
- PIE / ADC ISR；
- latency；
- execution time；
- jitter；
- overflow / missed event。

### Experiment

```text
ePWM1 SOCA → ADCA SOC0 → ADCINT1 → debug GPIO
```

### Exit condition

10-second and 30-minute runs show deterministic timing with no unexplained overflow / miss at the selected bring-up frequency。

---

## Phase 4 — Pure control core

### Learn

- PI；
- limiter；
- slew；
- saturation；
- anti-windup；
- numerical testability。

### Rule

Control modules must not know F28388D registers or physical ADC channel numbers。

### Host tests

```text
step error
zero error
positive saturation
negative saturation
recovery
slew limit
```

### Exit condition

Pure numerical behavior is deterministic and independently host-testable。

---

## Phase 5 — Safe control plumbing

### Integrate

```text
ADC
  ↓
scale
  ↓
PI
  ↓
limit
  ↓
CMPA shadow
```

Use a low-risk simulated / passive feedback setup before a switching power stage。

### Exit condition

The loop responds in the correct direction, remains bounded, and STOP/FAULT always overrides control output。

---

## Phase 6 — Protection architecture

### L0

```text
fault → CMPSS / digital fault → XBAR → Trip Zone → PWM safe
```

### L1

Fast sampled-domain protection in ISR。

### L2

Supervisory policy：temperature, fan, timeout, plausibility, startup timeout, recovery。

### Exit condition

Fault latency and recovery ownership are explicit；CPU is not required for the fastest shutdown path where hardware protection is required。

---

## Phase 7 — Low-voltage Buck

This is the first recommended real plant。

### Stage A — Open loop

```text
12 V input
10 / 20 / 30 / 40% duty
measure Vout
```

Validate basic plant direction and safe gate-drive behavior。

### Stage B — Closed-loop CV

Measure：

- steady-state error；
- load step；
- line step；
- overshoot；
- settling time；
- duty limit behavior。

### Stage C — CC / CV

Add current sensing and explicit arbitration / authority between voltage and current regulation。

### Exit condition

Buck reference has startup, bounded control, fault shutdown and retained dynamic evidence。

---

## Phase 8 — CLA / DMA optimization

Only optimize after CPU reference is stable。

### Compare

```text
CPU ISR reference
vs
CLA task / DMA-assisted path
```

Measure：

- latency；
- execution time；
- jitter；
- CPU load；
- numerical equivalence；
- failure behavior。

### Exit condition

Optimization does not change control semantics or safety authority and can be rolled back to the CPU reference。

---

## Phase 9 — Topology references

Recommended order：

```text
Buck
  ↓
Boost / PFC
  ↓
PSFB
  ↓
LLC
  ↓
Inverter
  ↓
Bidirectional power
```

Do not force common physics into one universal controller。

Share mechanisms：

- PI / 2P2Z / 3P3Z；
- limiter；
- slew；
- filter；
- diagnostics；
- scheduler；
- protection framework；
- communication protocol。

Keep topology-specific laws local：

- PFC current reference / feed-forward；
- PSFB phase-shift law；
- LLC frequency-control law；
- SPWM / SVPWM / PLL。

---

## Phase 10 — Multicore

Use multicore only after a measured reason exists。

Possible split candidates：

```text
CPU1
- safety authority
- primary sensing
- state machine
- primary regulation

CPU2
- waveform / DDS
- secondary calculation
- observer / background control
```

The actual split must be justified by：

- WCET / CPU budget；
- data ownership；
- fault authority；
- inter-core latency；
- stale-data behavior；
- reset behavior；
- safe fallback。

Existing `examples/multicore_reference/` should be reused for command/applied-state semantics rather than inventing a second ownership model。

---

## Suggested Git milestone sequence

```text
M0 docs/source authority
M1 deterministic safe power-control loop
M2 host-tested control core
M3 hardware protection path
M4 low-voltage Buck open loop
M5 Buck closed-loop CV
M6 Buck CC/CV + fault policy
M7 CLA/DMA optimization
M8 multicore integration
M9 PFC reference
M10 PSFB / LLC / inverter references
```

Each milestone should have：

- exact branch + commit；
- explicit DoD；
- retained measurement evidence；
- rollback point；
- known limitations。

---

## Low-ROI items to defer

Unless required by a later reference, do not prioritize：

- RTC；
- LCD / 7-segment；
- buzzer；
- step motor；
- generic DC motor demo；
- SD card；
- unrelated peripheral showcases。

QEP / CAP / Ethernet / EtherCAT / external memory should be learned when a power-control use case actually needs them。

---

## Next smallest action

After these documents are reviewed, implementation should begin with only：

```text
CPU1
+ safe GPIO99 state
+ ePWM1A/B
+ 20 kHz center-aligned timing
+ deadband
+ CMPA shadow
+ OST trip
```

No ADC, PI, CLA, DMA, CPU2 or real MOSFET in the first code patch。

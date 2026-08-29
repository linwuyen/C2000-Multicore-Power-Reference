# F28388D Power-Control Bench Validation Plan

## Principle

每一層只增加一個主要 failure mode；失敗就回到上一個已 PASS 狀態。

```text
change one thing
  ↓
measure
  ↓
PASS?
  ├─ yes → retain evidence → next stage
  └─ no  → stop → rollback → isolate cause
```

M1 不接真正 power MOSFET。

---

## Required instruments

Minimum：

- oscilloscope；
- DMM；
- current-limited DC source for ADC input；
- JTAG / CCS；
- optional logic analyzer。

Recommended scope assignment：

```text
CH1 = PWM1A
CH2 = PWM1B or ADC analog input
CH3 = ISR debug GPIO
CH4 = later gate / switch node only with suitable probe
```

---

## P0 — Power-off inspection

### Action

- identify board revision；
- record JP4 / JP5 fitted state；
- confirm no real power stage connected；
- verify J5 / J7 orientation；
- inspect GPIO99 / buffer path against schematic。

### PASS

Hardware state is unambiguous and documented。

---

## P1 — Rails / reset / JTAG

### Measure

- 5 V；
- 3.3 V；
- 1.2 V；
- REF3.0；
- RESET behavior；
- JTAG connection stability。

### PASS

- rails are within board-appropriate limits；
- reset releases normally；
- repeated connect / halt / run is stable。

Do not continue if REF3.0 or reset behavior is unexplained。

---

## P2 — Safe boot state

### Target behavior

```text
boot
  ↓
GPIO99 keeps external PWM path disabled
  ↓
ePWM initialized
  ↓
OST trip forced
  ↓
READY
```

### Measure

Observe J5 and internal/debug state through reset and run transitions。

### PASS

No unexpected energizing PWM appears during：

- reset；
- firmware initialization；
- READY；
- debugger halt / resume sequence used in the test。

---

## P3 — PWM timing

### Initial target

```text
20 kHz
center aligned
10% duty
about 500 ns deadtime
CMPA shadow update
```

### Measure

- period；
- duty；
- A/B relationship；
- deadtime；
- overlap；
- polarity。

### PASS

- measured frequency matches configuration；
- A/B do not overlap in the intended complementary mode；
- deadtime is visible and consistent；
- polarity is understood and recorded。

---

## P4 — ADC static calibration

Inject：

```text
0.5 V
1.5 V
2.5 V
```

Ideal 12-bit codes with 3.0 V reference：

```text
0.5 V → ~683
1.5 V → ~2048
2.5 V → ~3413
```

### PASS

- monotonic；
- proportional；
- stable；
- no clipping；
- measured REF3.0 recorded；
- gain/offset error noted rather than hidden。

---

## P5 — SOCA → ADC → ISR

Configure：

```text
ePWM1 SOCA
  ↓
ADCA SOC0
  ↓
ADCINT1
  ↓
PowerControlISR
```

Instrument ISR entry / exit with a debug GPIO。

### Measure

- expected ISR count；
- ADC interrupt overflow count；
- missed event count；
- SOCA-to-ISR entry latency；
- ISR execution time；
- ISR jitter。

For 20 kHz one-sample-per-cycle bring-up：

```text
expected ISR rate ≈ 20,000 / s
10 s expected count ≈ 200,000
```

### PASS

```text
0 ADC interrupt overflow
0 known missed event
bounded execution time
bounded jitter
```

Do not infer 100 kHz capability from a 20 kHz PASS；raise frequency in a separate controlled experiment。

---

## P6 — Arm / stop authority

### Test

From READY：

```text
ARM
  ↓
reviewed trip release
  ↓
reviewed external buffer release
  ↓
RUNNING
```

Then command STOP / inject software fault。

### PASS

- PWM becomes externally active only after ARM；
- STOP causes deterministic safe output；
- state transition is correct；
- trip / buffer states match the state machine。

---

## P7 — CMPA shadow update

Change requested duty through：

```text
10%
25%
50%
75%
```

### Measure

Observe PWM transitions across update boundaries。

### PASS

- update occurs on the configured shadow-load boundary；
- no runt pulse；
- no mid-cycle discontinuity caused by direct active-register update；
- duty remains bounded by software limits。

---

## P8 — Soak

Run the M1 loop without a real power stage for at least 30 minutes。

Track：

- ISR count；
- max execution time；
- max latency；
- max jitter；
- ADC overflow；
- spontaneous fault；
- state changes；
- unexpected PWM disable/enable events。

### PASS

```text
0 spontaneous unsafe state transition
0 ADC overflow
0 known missed event
0 unexplained trip
```

Any max-timing metric must remain within the declared timing budget。

---

## P9 — Closed-loop low-risk reference

Only after P0~P8 PASS。

Use a low-risk feedback setup / simulated plant before connecting a real switching stage。

Loop：

```text
ADC
  ↓
scale
  ↓
error
  ↓
PI
  ↓
limiter
  ↓
CMPA shadow
```

### PASS

- duty remains bounded；
- positive / negative error response direction is correct；
- saturation behavior is understood；
- integrator behavior is bounded；
- STOP / FAULT still overrides control output；
- control execution remains within timing budget。

This PASS proves control plumbing, not Buck/PFC/LLC stability。

---

## Timing evidence table

Each retained run should include：

| Metric | Expected | Measured | PASS/FAIL |
|---|---:|---:|---|
| PWM frequency | configured | | |
| deadtime | configured | | |
| ISR rate | configured | | |
| SOCA→ISR latency | budgeted | | |
| ISR execution max | budgeted | | |
| ISR jitter max | budgeted | | |
| ADC overflow | 0 | | |
| missed event | 0 | | |
| trip-to-safe output | budgeted | | |

No timing result is valid without exact commit + tool configuration + probe point。

---

## Rollback rule

On any failure：

```text
1. stop / force safe output
2. disconnect external actuation if relevant
3. return to previous passing commit/configuration
4. change exactly one suspected cause
5. repeat the failed stage
```

Do not fix multiple unrelated layers in one patch during bring-up。

---

## M1 Definition of Done

```text
[ ] P0 PASS
[ ] P1 PASS
[ ] P2 PASS
[ ] P3 PASS
[ ] P4 PASS
[ ] P5 PASS
[ ] P6 PASS
[ ] P7 PASS
[ ] P8 PASS
[ ] P9 PASS or explicitly deferred

[ ] exact commit retained
[ ] tool versions retained
[ ] scope captures retained
[ ] measured timing table retained
[ ] no real power MOSFET required for M1
```

Only after this point should the project claim a deterministic safe power-control reference baseline。

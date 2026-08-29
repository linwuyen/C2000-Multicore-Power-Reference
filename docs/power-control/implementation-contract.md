# M1 Implementation Contract — Deterministic Safe Power-Control Loop

## Status

This document is the implementation contract for the first F28388D digital-power code milestone in this repository。

If code, comments, examples or future task prompts conflict with this contract, stop and resolve the conflict before implementation。

---

## M1 objective

Prove a safe and measurable CPU1 control pipeline without connecting a real power MOSFET stage：

```text
safe boot
  ↓
ePWM1A/B
  ↓
SOCA
  ↓
ADCA SOC0 / ADCINA0
  ↓
ADCINT1
  ↓
thin control ISR
  ↓
optional pure control calculation
  ↓
CMPA shadow
  ↓
next PWM cycle
  ↓
Trip Zone capable safe shutdown
```

---

## Non-negotiable safety requirements

1. External PWM path must default to disabled during boot / initialization。
2. ePWM must be forced into a safe Trip Zone state before external actuation is permitted。
3. GPIO99 / board buffer authority must be explicit and measurable。
4. STOP / FAULT must override controller output。
5. Reset / debugger interaction must not silently energize the external PWM path。
6. ADC input used for M1 must stay inside the reviewed 0~3.0 V board range。
7. Real power MOSFET / high-energy stage is out of scope for M1。
8. No claim of hardware fault latency is allowed until measured。

---

## Timing requirements

M1 initial bring-up target：

```text
PWM                  20 kHz
counter               up/down
initial duty          10%
deadtime              about 500 ns
sample source         EPWM1 SOCA
sample channel        ADCA SOC0 / ADCINA0
update                CMPA shadow
CPU                   CPU1
```

These values are bring-up parameters, not topology specifications。

Required diagnostics：

- ISR count；
- ADC interrupt overflow count；
- missed-event count if detectable；
- SOCA→ISR latency；
- ISR execution current / max；
- jitter metric；
- state / fault latch reason。

---

## Architecture requirements

### Register ownership

Only the F2838x BSP / low-level power HAL may contain direct peripheral-specific access needed for power control。

Pure control code must not directly reference：

```text
EPwm*
AdcaRegs / ADC result registers
GPIO mux registers
EPWM*_BASE
ADC*_BASE
```

### Control core

PI / limiter / slew must be host-testable numerical modules with no TI-library dependency。

### ISR

Fast ISR must remain bounded and non-blocking。

Forbidden in fast ISR：

- console print；
- SCI/SPI protocol parsing；
- dynamic allocation；
- NVM access；
- wait loops；
- retry loops without a fixed compile-time bound。

### Authority

Communication / host code may request state or references；it may not directly own PWM enable, direct duty authority or Trip Zone clear。

---

## Required state behavior

Minimum state set：

```text
BOOT
SAFE_INIT
READY
RUNNING
FAULT
```

Required invariant：

```text
state != RUNNING
=> external PWM path is non-energizing
```

`READY → RUNNING` requires an explicit reviewed ARM condition。

`RUNNING → FAULT` must latch the fault reason before recovery is considered。

---

## ADC contract

Logical code should consume a signal identity, not a physical channel number。

M1 logical binding：

```text
POWER_SIGNAL_VOUT
  ↓
J7-17
  ↓
ADCINA0
```

M1 static calibration points：

```text
0.5 V
1.5 V
2.5 V
```

The implementation must expose raw ADC and scaled value for bench comparison。

---

## PWM contract

M1 must support：

- initialize；
- set bounded duty；
- configure / retain shadow update semantics；
- force all outputs safe；
- controlled release after ARM；
- diagnostic visibility of requested vs applied duty where practical。

M1 does not require generic frequency / phase APIs for every future topology。

Do not design unused LLC/PSFB abstractions before the first reference needs them。

---

## Protection contract

M1 must establish：

- software force OST behavior；
- PWM A/B safe trip action；
- STOP / software fault → trip + state latch；
- external buffer-disable behavior。

Later milestone must add real hardware fault input through comparator / XBAR / Trip Zone before claiming high-speed OCP/OVP protection。

---

## Test requirements

Before merge of M1 code, expected verification layers are：

### Host / unit

- PI；
- limiter；
- slew；
- state transition helpers where portable。

### Static

- no direct power-register access outside allowed layer；
- no blocking API in fast ISR；
- no dynamic allocation in runtime power path；
- no direct communication→PWM enable path。

### Target bench

Follow `validation-plan.md` P0→P9 sequence。

---

## Out of scope for M1

Explicitly excluded：

- real Buck switching；
- PFC；
- LLC；
- PSFB；
- inverter；
- CLA optimization；
- DMA optimization；
- CPU2 control split；
- CM / Ethernet integration；
- production thresholds；
- product protocol；
- production qualification。

---

## Change rule

Every M1 patch must answer：

```text
What signal path changed?
What authority changed?
What timing budget changed?
What new failure mode was introduced?
How is it measured?
What is the rollback point?
```

If those answers are unclear, the patch is too large or insufficiently specified。

---

## M1 merge gate

M1 is ready for merge only when：

```text
[ ] implementation matches this contract
[ ] host tests pass
[ ] static ownership checks pass
[ ] exact target build is recorded
[ ] P0-P8 bench evidence is retained
[ ] P9 is PASS or explicitly deferred with reason
[ ] no unexplained PWM output exists during unsafe states
[ ] known limitations are documented
```

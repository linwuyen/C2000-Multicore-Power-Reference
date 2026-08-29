# M1A Safe PWM Bring-Up — Implementation and Bench Evidence Contract

## Status

**IMPLEMENTED IN SOURCE, NOT YET TARGET-BUILD OR BENCH VERIFIED.**

This document records the first executable vertical slice of M1. It must not be used as evidence that the YXDSP-F28388D board has passed the test until the target-build and bench sections are completed with retained evidence.

Implementation branch:

```text
feat/f28388d-power-control-minimal
```

Design/document baseline:

```text
docs/f28388d-power-control-source
99afaa8b5cf39c50d6968353d986e8bba0676fee
```

Code snapshot before this evidence document:

```text
4dbb91b379970b6b3bd2fd1b4bdd6b01c130ff6e
```

Rollback point for the implementation slice is the design/document baseline above.

---

## Scope of this slice

Implemented now:

```text
CPU1
  ↓
GPIO99 early safe state
  ↓
EPWM1A/B configuration
  ↓
20 kHz up/down time base
  ↓
CMPA shadow @ CTR=0
  ↓
complementary dead-band path
  ↓
software OST
  ↓
READY
  ↓ explicit debugger ARM only
RUNNING
  ↓ STOP / invalid duty
READY / latched FAULT
```

Explicitly not implemented in this slice:

- ADC;
- SOCA;
- ADC ISR;
- PI / limiter / slew;
- CMPSS / XBAR hardware fault input;
- CPU2 control split;
- CLA / DMA;
- real Buck or other power stage.

No real gate driver or power MOSFET is allowed during this slice.

---

## Changed files

```text
sysconfig_cpu1/power_bringup.h
sysconfig_cpu1/power_bringup.c
sysconfig_cpu1/cpu1_main.c
sysconfig_cpu1/led_ex2_blinky_sysconfig_cpu1.syscfg
```

No CPU2 source, linker command file, target configuration, SCI driver, or production firmware is modified.

---

## Ownership changes

### GPIO0 / GPIO1

Previous CPU1 SysConfig assigned GPIO0/GPIO1 as LED GPIOs. That ownership conflicts with the board power mapping:

```text
GPIO0 → EPWM1A
GPIO1 → EPWM1B
```

The M1 branch removes GPIO0/GPIO1 ownership from the CPU1 SysConfig file. The power bring-up layer configures the EPWM mux only after OST behavior has been configured.

### GPIO99

GPIO99 is owned by the power bring-up layer as the board PWM-buffer control.

Reviewed board documentation says the enable path is active-low:

```text
GPIO99 = 1 → intended buffer disable
GPIO99 = 0 → intended buffer enable
```

This is still a **board fact that requires actual-board measurement** before being trusted as a safety layer.

### ARM / STOP authority

SCI remains an echo path and does not receive direct PWM authority.

The first slice uses debugger-visible command variables:

```text
g_sPowerBringupCommand.armRequest
g_sPowerBringupCommand.stopRequest
g_sPowerBringupCommand.dutyPermille
```

Only `PowerBringup_Service()` translates those requests into trip/buffer/PWM actions.

---

## State invariant

Intended invariant after `PowerBringup_InitEarlySafeState()` has executed:

```text
state != POWER_STATE_RUNNING
=> OST asserted or retained
=> GPIO99 requests buffer disable
```

The implementation states are:

```text
BOOT
SAFE_INIT
READY
RUNNING
FAULT
```

`FAULT` is latched in this slice. STOP does not clear FAULT. Recovery requires reset/reload until a reviewed fault-reset policy is added later.

Important evidence boundary: firmware cannot prove the electrical state from reset assertion until GPIO99 has been configured. JP4/JP5 population and board pull behavior must therefore be checked at P0/P1.

---

## Timing model

The current project configuration targets 200 MHz CPU operation and the CPU1 SysConfig requests:

```text
EPWMCLKDIV = 1
```

The power module additionally configures EPWM1 TBCLK prescalers to `/1`.

With the intended 200 MHz EPWMCLK/TBCLK:

```text
PWM mode      = up/down
PWM frequency = 20 kHz
TBPRD         = 200 MHz / (2 × 20 kHz)
              = 5000
```

Dead-band counter is configured for full-cycle TBCLK counting:

```text
100 counts × 5 ns/count = 500 ns
```

Current diagnostic expectations:

```text
g_sPowerBringupDiag.tbprd         = 5000
g_sPowerBringupDiag.deadbandCount = 100
```

These values must be checked against the actual generated SysConfig output and scope measurements. They are not topology specifications.

---

## Duty semantics

`dutyPermille` is the **pre-dead-band AQ command**, not a claim about final connector high-time after dead-band insertion.

The base waveform uses:

```text
CAU → HIGH
CAD → LOW
```

so for up/down mode:

```text
base duty = (TBPRD - CMPA) / TBPRD
CMPA      = TBPRD × (1 - duty)
```

Expected compare values:

| `dutyPermille` | base duty | expected CMPA |
|---:|---:|---:|
| 100 | 10% | 4500 |
| 250 | 25% | 3750 |
| 500 | 50% | 2500 |
| 750 | 75% | 1250 |

Allowed software range in this slice is 5% to 90%. An out-of-range request latches `POWER_FAULT_DUTY_OUT_OF_RANGE`, forces OST, and requests buffer disable.

---

## Target-configuration discrepancy to resolve

The repository currently contains two different forms of target metadata:

- `.cproject` identifies `TMS320F28388D`;
- the CPU1 `.syscfg` generic CLI arguments use `F2838x / F2838x_176pin`, while its `@v2CliArgs` line still names `TMS320F28384D`.

This patch intentionally does **not** silently rewrite target metadata.

Before target evidence is accepted, CCS/SysConfig must prove which device description actually generates the build and the mismatch must either be corrected or explicitly justified.

Until then:

```text
SOURCE IMPLEMENTED != TARGET BUILD VERIFIED
```

---

## Build verification

### Required configuration

Start with CPU1 RAM build. Do not flash first.

Record:

```text
branch
exact commit SHA
CCS version
C2000Ware version
SysConfig version
C2000 compiler version
RAM/FLASH configuration
output artifact path
artifact SHA-256
build timestamp
```

PASS requires:

```text
[ ] SysConfig generation succeeds
[ ] power_bringup.c is compiled
[ ] no duplicate Device_init/code_start ownership
[ ] no undefined DriverLib symbol
[ ] no linker error
[ ] exact .out artifact retained
```

Do not infer build PASS from project metadata or source review.

---

## Bench sequence

### P0 — power-off inspection

```text
[ ] real power stage disconnected
[ ] JP4 state recorded
[ ] JP5 state recorded
[ ] J5 orientation confirmed
[ ] scope ground/reference safe
```

Preferred reviewed bring-up population is JP4 fitted / JP5 removed, but actual board state is the authority.

### P1 — boot-safe observation

After load/reset and before ARM, inspect:

```text
g_sPowerBringupDiag.state
    expected: POWER_STATE_READY

g_sPowerBringupDiag.fault
    expected: POWER_FAULT_NONE

g_sPowerBringupDiag.bufferEnableLevel
    expected firmware request: 1

g_sPowerBringupDiag.tripStatus
    expected: OST asserted
```

Measure simultaneously:

```text
CH1 = J5 EPWM1A path
CH2 = J5 EPWM1B path
CH3 = GPIO99
```

PASS:

- GPIO99 electrical level matches intended disable state;
- J5 is non-energizing before ARM;
- no unexplained pulse during reset/load/init;
- actual buffer-disabled electrical state is recorded.

Do not equate high-impedance with LOW.

### P2 — explicit ARM

From CCS debugger:

```text
g_sPowerBringupCommand.armRequest = 1
```

Expected after foreground service:

```text
state             = POWER_STATE_RUNNING
buffer level      = 0
OST status        = clear
```

Scope PASS:

```text
frequency         ≈ 20 kHz
mode              center-aligned / up-down behavior
A/B relationship  complementary as intended
dead time         ≈ 500 ns
no overlap        observed at both transitions
```

If polarity or dead-band behavior differs from the intended model, STOP immediately and treat the configuration as FAIL rather than compensating in the measurement interpretation.

### P3 — shadow duty update

With the board still disconnected from a power stage, test sequentially:

```text
g_sPowerBringupCommand.dutyPermille = 100
g_sPowerBringupCommand.dutyPermille = 250
g_sPowerBringupCommand.dutyPermille = 500
g_sPowerBringupCommand.dutyPermille = 750
```

For each point retain:

- requested duty;
- `cmpa` diagnostic;
- measured A/B waveform;
- dead time;
- evidence of no runt pulse during transition.

### P4 — STOP authority

From RUNNING:

```text
g_sPowerBringupCommand.stopRequest = 1
```

Expected:

```text
state        = POWER_STATE_READY
OST          = asserted
GPIO99       = disable level
J5           = non-energizing
```

Record STOP-to-J5-safe latency as an observation only. This path is foreground software and is **not** a high-speed protection claim.

### P5 — software fault injection

From READY or RUNNING:

```text
g_sPowerBringupCommand.dutyPermille = 950
```

Expected:

```text
state  = POWER_STATE_FAULT
fault  = POWER_FAULT_DUTY_OUT_OF_RANGE
OST    = asserted
GPIO99 = disable level
```

Then attempt:

```text
stopRequest = 1
armRequest  = 1
```

PASS requires FAULT to remain latched and output to remain non-energizing. Recovery for this slice is reset/reload only.

---

## Known limitations

1. No target build has yet been executed for this branch.
2. No board measurement has yet verified GPIO99 polarity, JP4/JP5 population, or J5 disabled-state behavior.
3. The SysConfig device metadata discrepancy remains open.
4. Software OST is the only trip source in this slice; no CMPSS/XBAR hardware path exists yet.
5. STOP is foreground software, so no deterministic fast shutdown latency is claimed.
6. There is no ADC/control ISR yet.
7. There is no explicit fault-reset state transition yet.
8. Requested/applied duty diagnostics describe the pre-dead-band command, not final pin duty.

---

## Next allowed change after PASS

Do not add PI or a real Buck immediately.

After target build plus P0-P5 safe-PWM evidence passes, the next vertical slice should be:

```text
J7-17 / ADCINA0
  ↓
static 0.5 / 1.5 / 2.5 V validation
  ↓
EPWM1 SOCA
  ↓
ADCA SOC0
  ↓
ADCINT1
  ↓
thin ISR + timing GPIO
```

That change must preserve this PWM safety baseline as the rollback point.

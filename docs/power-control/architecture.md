# F28388D Power-Control Architecture

## Design objective

先建立一條最小、可測量、可回退的 vertical slice，再讓 abstraction 隨被證明的需求成長。

第一版不追求 universal framework；第一版追求：

```text
safe boot
  ↓
PWM timing
  ↓
synchronous sampling
  ↓
deterministic ISR
  ↓
control update
  ↓
hardware-capable shutdown
```

---

## M1 signal path

```text
CPU1

EPWM1 time base
  ↓
AQ / deadband
  ↓
SOCA
  ↓
ADCA SOC0
  ↓
S/H + conversion
  ↓
ADCINT1
  ↓
PowerControlISR
  ├─ sensing update
  ├─ fast software protection
  ├─ control law
  ├─ output limit
  └─ timing diagnostics
  ↓
CMPA shadow write
  ↓
load at defined PWM boundary
  ↓
next PWM cycle
```

External actuation path:

```text
EPWM1A/B
  ↓
74LVXC3245 buffer
  ↓
J5
```

GPIO99 / JP4 / JP5 are part of the actuator-enable authority and therefore belong in the safety design, not in board-setup trivia。

---

## Dependency direction

Target direction:

```text
application / system policy
          ↓
       topology
          ↓
      control core
          ↓
       power HAL
          ↓
      BSP / drivers
          ↓
       hardware
```

Forbidden examples:

```text
control/ writes EPwm registers directly
system/ clears Trip Zone directly
communication/ writes duty directly
host command bypasses state machine to enable PWM
```

Allowed direction:

```text
Control_SetOutput()
  ↓
PowerPWM_SetDuty()
  ↓
BSP_F2838x_PWM...
  ↓
DriverLib / registers
```

---

## Minimal module set

M1 only needs:

```text
power/
  bsp/f2838x/
    power_board.c
    power_pwm.c
    power_adc.c
    power_trip.c
    power_debug_gpio.c

  sensing/
    power_sensing.c

  control/
    pi.c
    limiter.c
    slew.c

  protection/
    power_protection.c

  diag/
    timing_diag.c
```

Do not create empty PFC / LLC / PSFB / inverter modules before one real vertical slice requires them。

---

## ISR rule

Fast ISR must stay thin and deterministic。

Target form:

```c
__interrupt void PowerControlISR(void)
{
    TimingDiag_Begin();

    PowerSensing_UpdateFast();
    PowerProtection_RunFast();

    if (System_IsControlEnabled())
    {
        PowerControl_RunFast();
    }

    TimingDiag_End();

    /* clear ADC interrupt / PIE ACK */
}
```

Fast ISR must not contain：

- blocking I/O；
- printf / console output；
- dynamic allocation；
- unbounded loops；
- flash/NVM writes；
- protocol parsing；
- command retries；
- state-machine policy with unpredictable execution cost。

---

## Control authority

Control law computes a requested actuator value；it does not own permission to energize hardware。

```text
reference
  ↓
controller
  ↓
limiter
  ↓
requested duty / phase / frequency
```

Whether that value may reach PWM output is decided by system/protection authority。

This separation prevents a mathematically valid controller output from becoming an unsafe actuator command during boot, fault, reset, calibration or invalid-state conditions。

---

## Actuator authority

Target request path:

```text
Host / local request
  ↓
validation
  ↓
system state machine
  ↓
topology permission
  ↓
power HAL
  ↓
PWM
```

Host or communication code must never own：

- direct PWM enable；
- direct hardware-trip clear；
- unrestricted duty write；
- direct transition into RUN without local safety checks。

---

## Protection authority

Protection is split by required latency and failure tolerance。

### L0 — Hardware protection

```text
fault source
  ↓
comparator / digital fault
  ↓
XBAR / trip mux
  ↓
ePWM Trip Zone
  ↓
PWM forced safe
```

CPU is not required for first shutdown action。

### L1 — Fast software protection

```text
ADC sample
  ↓
fast ISR
  ↓
Protection_RunFast()
  ↓
software trip / latch
```

Use for conditions where ISR latency is acceptable and values need sampled-domain interpretation。

### L2 — Supervisory protection

Slow policy such as：

- temperature；
- fan failure；
- communication timeout；
- sensor plausibility；
- startup timeout；
- recovery / lockout policy。

Do not merge all three layers into one `Protection_CheckEverything()` function。

---

## State model for M1

Start minimal：

```text
BOOT
  ↓
SAFE_INIT
  ↓
READY
  ↓ arm
RUNNING
  ↓ stop / fault
FAULT
  ↓ reviewed reset
READY
```

Key invariant：

```text
BOOT / SAFE_INIT / READY / FAULT
=> external PWM must remain non-energizing
```

Only `RUNNING` may release all required safety gates。

Later real-power-stage reference can expand to：

```text
SELF_TEST
PRECHARGE
SOFT_START
STOPPING
RECOVERY
LATCHED
```

---

## Sampling architecture

The control loop sees the sampled value, not an ideal continuous signal。

Therefore sampling instant is part of control design：

```text
switching edge / ringing region  → avoid when practical
quiet current / voltage window   → preferred sample region
```

M1 may begin with SOCA at a simple deterministic event for timing validation；real topology integration must re-evaluate sample position based on switching noise, current ripple, sensor delay and plant timing。

---

## Multicore extension boundary

Do not split the M1 fast loop across CPU1/CPU2 before single-core timing and ownership are stable。

Recommended sequence：

```text
CPU1-only deterministic reference
  ↓
retained timing evidence
  ↓
identify workload / ownership reason to split
  ↓
CPU1 ↔ CPU2 semantic contract
  ↓
validate sequence / freshness / timeout / reset behavior
  ↓
board IPC timing evidence
```

Existing `examples/multicore_reference/` can provide the semantic ownership model, but it does not by itself prove target IPC timing or power-control correctness。

---

## Static architecture checks

Future CI should be able to reject at least：

```text
control/ contains EPwm or GPIO register access
communication/ calls PWM enable directly
fast ISR contains blocking calls
malloc/free exists in power runtime
trip clear is reachable from unvalidated host command
```

These checks turn architecture from documentation into an enforceable contract。

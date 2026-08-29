# YXDSP-F28388D Board Mapping for Digital Power

## Scope

本文件只整理對 M1 digital-power bring-up 有直接影響的 board-level path。

完整 GPIO mux 能力仍應回查 F2838x datasheet / TRM；這裡只回答：**這塊板上，哪些訊號實際經過哪些 board net / connector / buffer。**

---

## Board-level signal model

```text
TMS320F28388D GPIO / peripheral
          ↓
       pin mux
          ↓
board peripheral / level shift / buffer
          ↓
      J1 / J2 / J5 / J7 / J8
```

Important rule：MCU GPIO number ≠ board connector pin number。

第一次接線前必須交叉確認：

- schematic net；
- manual connector orientation；
- board silkscreen；
- continuity measurement。

不要以「左上一定是 Pin 1」作為接線依據。

---

## PWM path

Board documentation identifies：

```text
GPIO0  → ePWM1A
GPIO1  → ePWM1B
...
GPIO11 → ePWM6B
```

J5 exposes：

```text
J5-1 ... J5-12
XPWM1 ... XPWM12
ePWM1A/B ... ePWM6A/B
```

The board output path includes a 74LVXC3245 buffer / level-shift stage。

Therefore software-visible EPWM output and connector-visible J5 output are not the same authority boundary。

---

## PWM buffer authority

Board documentation indicates GPIO99 / JP4 / JP5 participate in the PWM buffer-enable path。

Reference bring-up policy：

```text
JP4 fitted
JP5 removed
GPIO99 controlled by firmware
```

GPIO99 enable semantics are active-low according to the reviewed board documentation。

Safe intention：

```text
GPIO99 = disable
  ↓
initialize clocks / GPIO / ePWM / ADC
  ↓
force ePWM OST trip
  ↓
verify internal timing
  ↓
only after reviewed ARM condition:
clear allowed trip + release buffer
```

### Required actual-board verification

Before relying on this path as a safety layer, measure：

- actual JP4 / JP5 population；
- GPIO99 logic level vs J5 enable behavior；
- buffer-disabled J5 electrical state；
- downstream gate-driver input state if connected；
- whether an external pull-down exists and is strong enough。

**High-Z is not equivalent to logic-low or gate-off.**

---

## ADC path for M1

M1 uses：

```text
J7-17
  ↓
ADCINA0
  ↓
board RC network
  ↓
ADCA SOC0
```

The board uses REF3030 for approximately 3.0 V ADC reference。

First static calibration points：

```text
Vin = 0.5 V  → ideal 12-bit code ≈ 683
Vin = 1.5 V  → ideal 12-bit code ≈ 2048
Vin = 2.5 V  → ideal 12-bit code ≈ 3413
```

Ideal relation：

```text
ADCcode = Vin / 3.0 × 4095
```

PASS is not exact equality；PASS requires：

- monotonic response；
- approximately correct gain；
- bounded offset；
- stable reading；
- no clipping in the intended range。

---

## ADC electrical constraint

Do not directly connect：

```text
±5 V
±10 V
high-voltage sensor output
unknown bipolar signal
```

to J7 ADC input。

Signals outside 0~3.0 V require reviewed：

- attenuation；
- level shift / bias；
- clamp / protection；
- source impedance；
- anti-alias / RC design；
- common-mode range。

The acquisition window must be validated against source impedance and the board RC network；do not assume one generic `ACQPS` is correct for every sensor。

---

## Sampling point

For M1, a deterministic SOCA event is sufficient to prove timing plumbing。

For a real power stage, sampling point must be chosen against actual switching behavior：

```text
MOSFET edge
  ↓
ringing / dv-dt / di-dt noise
  ↓
possible measurement corruption
```

A real topology test must compare sample positions and select a quieter window where practical。

---

## High-risk mux overlap

The F28388D has rich pin muxing, but the board routes many functions through shared pins / connectors。

Known conflict families include：

- SPI；
- SCI；
- QEP / CAP；
- EMIF；
- Ethernet / EtherCAT related signals。

Rule：

```text
MCU supports peripheral X
≠
board provides a conflict-free external path for X
```

Any new peripheral must first add a board binding record before code enables the mux。

---

## Minimal board binding table

M1 can begin with：

| Logical signal | Board path | Intended use |
|---|---|---|
| `POWER_PWM_MAIN_A` | ePWM1A → buffer → J5 | PWM output A |
| `POWER_PWM_MAIN_B` | ePWM1B → buffer → J5 | complementary PWM B |
| `POWER_PWM_BUFFER_EN` | GPIO99 / JP4 path | external PWM gate |
| `POWER_SIGNAL_VOUT` | J7-17 → ADCINA0 | static/synchronous ADC input |
| `POWER_DEBUG_ISR` | free verified GPIO | ISR profiling |

Exact debug GPIO is intentionally not fixed here until pin-mux conflict review is completed。

---

## Power-on checklist

Before M1 scope testing：

```text
[ ] correct board revision identified
[ ] JP4 / JP5 actual state recorded
[ ] 5 V rail measured
[ ] 3.3 V rail measured
[ ] 1.2 V rail measured
[ ] REF3.0 measured
[ ] RESET behavior observed
[ ] JTAG stable
[ ] GPIO99 safe level confirmed
[ ] power stage disconnected
[ ] J5 / J7 connector orientation confirmed
[ ] ADC source current-limited and <= 3.0 V
```

If any item is unknown, stop at board bring-up；do not compensate with software assumptions。

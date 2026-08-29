# F28388D Digital Power Control Reference

本目錄是 `C2000-Multicore-Power-Reference` 的數位電源文件入口。

目的不是建立另一套「萬能 framework」，而是先把一條可量測、可驗證、可回退的 power-control vertical slice 定義清楚，之後所有 firmware、bench evidence、host tests 與 multicore extension 都應回到這些文件核對。

## Authority boundary

本 repo 仍然是 public-safe reference / portfolio workspace，不是任何正式產品韌體的 production source of truth。

本目錄的文件只對本 repo 的 F28388D learning/reference scope 有效：

- board bring-up；
- ePWM / ADC / ISR deterministic timing；
- control-core ownership；
- protection authority；
- bench validation；
- later multicore integration。

任何產品專屬 threshold、customer protocol、internal measurement 或正式 qualification evidence 都不應被搬進來。

## Documents

1. [`source-of-truth.md`](source-of-truth.md)
   - 證據層級、目前已知事實、工程假設、待驗證項目。

2. [`architecture.md`](architecture.md)
   - 最小 power-control data path、dependency rule、control / protection / actuator authority。

3. [`board-f28388d-yxdsp.md`](board-f28388d-yxdsp.md)
   - YXDSP-F28388D 板級 PWM/ADC/enable path 與 bring-up safety constraints。

4. [`validation-plan.md`](validation-plan.md)
   - P0 → P9 bench validation、量測指標、PASS/rollback 規則。

5. [`roadmap.md`](roadmap.md)
   - 從 single-core deterministic loop 到 Buck、hardware protection、CLA/DMA、multicore 的學習順序。

## First implementation target

第一個 executable milestone 固定為：

```text
M1 — deterministic safe power-control loop

CPU1
  ePWM1A/B
    ↓ SOCA
  ADCA SOC0
    ↓ EOC / ADCINT1
  thin ISR
    ↓
  sensing → fast protection → control → limit
    ↓
  CMPA shadow
    ↓
  next PWM cycle
```

M1 不接真正 power MOSFET，不宣稱 plant stability，也不碰 PFC / LLC / PSFB compensation。

成功條件是能量測並保存：

- PWM frequency / duty / deadtime；
- ADC scale；
- SOCA → ISR latency；
- ISR execution time / jitter；
- ADC overflow / missed event count；
- CMPA shadow update boundary；
- trip / stop safe-state behavior。

## Reference baseline

Documentation branch creation baseline:

```text
main
1096701f918f00da7c0383829514c662f130561d
```

Current documented TI environment in the repository README:

```text
C2000Ware        6.00.01.00
SysConfig        1.24.0 or newer
SysConfig tested 1.27.1
C2000 compiler   ti-cgt-c2000_22.6.1.LTS
Device           F2838x / F28388D, 176-pin package
```

## Source set used to create these documents

- `28388 開發板手冊 V1.0`；
- `YSDSP-F28388D 一體板原理圖`；
- `YXDSP-F28388D 數位電源韌體實戰指南`；
- `POWER_CONTROL_MINIMAL F28388D 數位電源設計與驗證指南`；
- `C2000-Multicore-Power-Reference` current repository state at the baseline commit above。

TI register-level implementation、device limits、interrupt details 與 timing constraints 仍必須回到對應版本的 TI F2838x TRM / datasheet / C2000Ware 驗證。

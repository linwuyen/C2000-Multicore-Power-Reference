# Power-Control Source of Truth

## Purpose

這份文件定義本 repo 的 F28388D digital-power reference 到底「已知什麼、假設什麼、還沒證明什麼」。

核心規則：**不把合理推論寫成已驗證事實，不把 host test 當 board evidence，不把 board behavior 當 production qualification。**

---

## Evidence hierarchy

### L1 — Primary hardware evidence

優先度最高：

- YSDSP-F28388D schematic net；
- board BOM / actual IC marking；
- actual board continuity；
- scope / logic-analyzer measurement；
- measured rail / reset / reference behavior。

若 schematic 與說明文字衝突，先以 schematic net 為主要文件證據；若實板 BOM / marking 又與 schematic 不同，實板優先。

### L2 — Board documentation

- YXDSP / YSDSP development-board manual；
- connector tables；
- example wiring diagrams；
- documented jumper behavior。

### L3 — TI first-party device evidence

Register、timing、peripheral behavior 與 electrical limit 必須以當下實作版本對應的：

- F2838x datasheet；
- F2838x Technical Reference Manual；
- C2000Ware DriverLib documentation / example；
- compiler / SysConfig version documentation。

### L4 — Repository executable evidence

可包含：

- exact-commit CCS build；
- host unit / contract / numerical tests；
- static ownership checks；
- HIL / bench scripts；
- retained artifacts / logs / hashes。

每個 test 必須說明它證明什麼，以及它不能證明什麼。

### L5 — Engineering inference

由 topology physics、signal path、software ownership 或 timing budget 推導出的設計選擇。

這類內容必須標成「engineering assumption / inference」，直到被測試或 first-party evidence 支持。

---

## Known facts — board level

目前文件集直接支持以下板級事實：

- J5 exposes XPWM1~12，對應 ePWM1A/B ~ ePWM6A/B。
- PWM output path 經過 74LVXC3245 level-shift / buffer。
- GPIO99 / JP4 / JP5 參與 PWM buffer enable path。
- bring-up 文件建議優先採 `JP4 fitted + JP5 removed`，由 firmware 掌控 buffer enable；JP4 / JP5 不應同時插入。
- GPIO99 的 buffer-enable control 為 active-low semantics；safe boot 應先保持 external PWM path disabled。
- J7-17 對應 ADCINA0。
- ADC reference network 使用 REF3030，board-level reference 約 3.0 V。
- 直接送入 ADC header 的訊號必須留在 0~3.0 V 範圍；雙極性或更高電壓訊號需要先縮放、偏壓與保護。
- SPI / SCI / QEP / EMIF / Ethernet 存在大量 GPIO mux overlap；MCU peripheral capability 不代表 board connector 上存在獨立乾淨路徑。

---

## Known facts — repository level

At documentation baseline:

```text
repository: linwuyen/C2000-Multicore-Power-Reference
main:       1096701f918f00da7c0383829514c662f130561d
```

Repo 已存在：

- F2838x CPU1 / CPU2 / CM CCS/SysConfig workspace；
- RAM / FLASH build boundaries；
- public CPU1→CPU2 command/applied-state reference；
- deterministic host tests for portable multicore frame semantics；
- explicit README confidentiality / evidence boundary。

Portable multicore host tests **不證明**：

- CCS CPU1/CPU2 build；
- IPC latency；
- Message RAM atomic publication；
- board pin behavior；
- power-control timing；
- production qualification。

---

## Engineering assumptions for M1

以下是 first bring-up choices，不是 topology-final constants：

```text
PWM frequency      20 kHz
counter mode       center-aligned / up-down
initial duty       10%
deadtime           about 500 ns
ADC source         ePWM1 SOCA
ADC signal         ADCINA0 / J7-17
ADC ISR            ADCA1 ISR
CMPA update        shadow, boundary load
CPU                CPU1 only
power stage        disconnected
```

選 20 kHz 的目的只是讓 scope timing 清楚、降低 first bring-up failure density；未來 real Buck / PFC / PSFB / LLC 的 switching frequency 必須由 plant、magnetics、loss、sampling 與 control-bandwidth requirements 重新決定。

---

## Items that must be verified on the actual board

在任何 real power-stage connection 前，至少要取得以下 bench evidence：

- GPIO99 實際 enable/disable polarity；
- JP4 / JP5 實板 fitted state；
- buffer disabled 時 J5 output 與下游 gate-driver input 的實際 electrical state；
- High-Z path 是否有可靠 pull-down；
- ePWM1A/B pin mapping 與 physical connector orientation；
- deadtime polarity；
- PWM trip action；
- ADCINA0 gain / offset / monotonicity；
- actual REF3.0 value；
- ADC acquisition window 是否足以支援 board RC + source impedance；
- SOCA event position；
- ADC interrupt rate；
- interrupt overflow / missed event；
- CMPA shadow load timing；
- reset / debugger halt / reconnect 時 external PWM behavior。

---

## Evidence naming rule

Bench evidence 建議固定：

```text
evidence/
  <date>/
    <commit>/
      build/
      scope/
      measurements.md
      tool_versions.txt
      artifact_sha256.txt
```

`measurements.md` 至少要記錄：

- exact commit；
- CCS / compiler / C2000Ware / SysConfig versions；
- build configuration；
- board revision；
- jumper state；
- probe / instrument setup；
- expected value；
- measured value；
- PASS / FAIL；
- unresolved discrepancy。

---

## Promotion rule

只有在一個敘述取得 higher-level evidence 後，才能從：

```text
assumption
  ↓
measured observation
  ↓
repeatable validation
  ↓
retained evidence
```

升級成 reference fact。

任何未量測的 timing、polarity、fault latency、sample quality 都不得用「已驗證」描述。

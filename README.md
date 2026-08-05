# C2000 Multicore Power Reference

Public-safe F2838x multicore workspace and reference material for portfolio use.

This repository is intentionally separated from formal product firmware and internal project governance. **It is not a production source of truth.** Product source, design authority, retained evidence, and operational state remain in separate private systems and are not copied here.

## What this repository demonstrates

- TI C2000 F2838x CPU1/CPU2/CM project structure;
- SysConfig and generated-device-support ownership;
- RAM and FLASH build boundaries;
- duplicate-symbol and tool-version troubleshooting;
- public-safe multicore command/applied-state design;
- fail-safe frame validation and exact-evidence thinking.

## Public multicore reference

[`examples/multicore_reference/`](examples/multicore_reference/) contains a portable CPU1 → CPU2 reference:

```text
CPU1 command owner
  sequence + command + reference + flags
                 ↓ checksum / transport
CPU2 applied-state owner
  validate → apply reference or force safe zero
```

It includes:

- an explicit frame ABI;
- monotonic sequence ownership;
- checksum validation;
- stale/out-of-order rejection;
- safe-zero behavior for corrupt or unsupported frames;
- deterministic GCC host tests;
- GitHub Actions validation.

Run:

```text
cd examples/multicore_reference
make test
```

The host reference does not claim CCS build, IPC timing, board behavior, or production qualification.

## Sanitized engineering case study

[`docs/case-studies/multicore-power-firmware-recovery.md`](docs/case-studies/multicore-power-firmware-recovery.md) explains a public-safe command/applied-state recovery method for multicore power firmware. Proprietary product identity, source, thresholds, pins, protocol values, and internal measurements are omitted.

## CCS workspace layout

```text
C2000-Multicore-Power-Reference/
  .gitignore
  README.md
  sysconfig_cm/
  sysconfig_cpu1/
  sysconfig_cpu2/
  sysconfig_multi/
  examples/multicore_reference/
  docs/case-studies/
```

CPU-specific source and SysConfig files remain inside their CCS project directories. Application source is not placed loosely at repository root.

## Required TI components

Current documented environment:

```text
C2000Ware        6.00.01.00
SysConfig        1.24.0 or newer
SysConfig tested 1.27.1
C2000 compiler   ti-cgt-c2000_22.6.1.LTS
Device           F2838x / F28388D, 176-pin package
```

Confirm product discovery in:

```text
Window > Preferences > Code Composer Studio > Products
```

## Build verification

CPU1 and CPU2 RAM configurations can be invoked from the documented CCS workspace:

```powershell
cd .\sysconfig_cpu1\RAM
& 'C:\ti\ccs1281\ccs\utils\bin\gmake' -k -j 16 all -O

cd ..\..\sysconfig_cpu2\RAM
& 'C:\ti\ccs1281\ccs\utils\bin\gmake' -k -j 16 all -O
```

Expected outputs:

```text
sysconfig_cpu1/RAM/empty_sysconfig_cpu1.out
sysconfig_cpu2/RAM/empty_sysconfig_cpu2.out
```

A retained build result must also record exact commit, tool versions, configuration, artifact path, timestamp, and SHA-256. README commands are not evidence that the current head has been built.

## Device-support ownership

SysConfig owns generated device support. Legacy local files such as:

```text
device/device.c
device/F2838x_CodeStartBranch.asm
```

must not be compiled together with generated equivalents. Duplicate `Device_init` or `code_start` is an ownership error, not a warning to suppress.

## Common failures

### SysConfig version mismatch

```text
C2000 SysConfig version 6.00.01.00 requires at least version 1.24.0 of SysConfig.
```

Install/discover a compatible SysConfig version; do not patch generated output to bypass the requirement.

### Duplicate symbols

```text
symbol "Device_init" redefined
symbol "code_start" redefined
```

Check generated vs legacy source ownership for every build configuration.

### DriverLib path failure

Confirm the project points to the installed C2000Ware version and does not rely on a stale/deprecated package variable.

## Public and confidentiality boundary

1. Do not copy formal product firmware into this repository.
2. Do not publish customer/product-specific schematics, thresholds, pin maps, protocols, logs, or measurements.
3. Do not imply production qualification from host tests or empty-workspace builds.
4. Keep generated build output out of Git.
5. Every example must state what it proves and what remains unverified.
6. Formal product source and internal project governance remain separate private authorities.

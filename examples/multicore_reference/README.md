# Public multicore reference

A portable CPU1 → CPU2 ownership example for a safety-oriented C2000 portfolio.

```text
CPU1 command owner
  sequence + command + reference + flags
                 ↓ checksum / IPC transport
CPU2 applied-state owner
  validate magic, checksum, sequence, command
                 ↓
  apply reference or force safe zero
```

## Demonstrated properties

- one producer owns command sequencing;
- one consumer owns the applied reference;
- frame corruption is detected before application;
- repeated or out-of-order frames are rejected;
- unsupported commands fail to safe zero;
- an explicit safe-zero command cannot carry a nonzero reference;
- host tests are deterministic and independent of TI libraries.

Run:

```text
cd examples/multicore_reference
make test
```

## Mapping to F2838x

A target integration can transport the same semantic frame through IPC flags, Message RAM, FSI, SPI, or another reviewed channel. The target adapter must separately define and validate:

- ABI width, alignment, endianness, and atomic publication;
- producer/consumer ownership;
- memory barriers and notification ordering;
- timeout and missing-pulse behavior;
- boot synchronization and stale Message RAM contents;
- reset behavior of either CPU;
- safe output state when validation fails;
- telemetry acknowledging command vs applied state.

The portable checksum is for accidental corruption and deterministic tests; it is not cryptographic authentication.

## Evidence boundary

This example proves only its portable host behavior on the exact tested commit. It does not claim a CCS CPU1/CPU2 build, IPC timing, board behavior, or production qualification.

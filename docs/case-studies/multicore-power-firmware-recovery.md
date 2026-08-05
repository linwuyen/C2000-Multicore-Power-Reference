# Sanitized case study: restoring multicore command coherence

> Public portfolio summary. Product identity, schematics, pin assignments, protocol values, thresholds, source code, and internal measurements are intentionally omitted.

## Context

A multicore power-control firmware path separated responsibilities across a command/parser CPU and a real-time waveform/control CPU. The design also had to preserve a deterministic safe output while commands were rejected, repeated, or handed over between built-in and externally supplied waveform sources.

## Failure pattern

The difficult bug was not a single arithmetic error. It was a state-coherence problem:

- the requested selection and the actually applied source could diverge;
- an unsupported selection could be rejected but still disturb later recovery;
- repeating the same valid selection could be ignored because the command value had not changed;
- a pending handover could complete after a newer request and overwrite the intended state;
- a neutral output value existed as a literal rather than an explicit safety contract;
- ownership of a cross-CPU status signal was not represented by one canonical configuration source.

These conditions are dangerous in power firmware because a command parser can report one state while the real-time output path is executing another.

## Engineering approach

### 1. Separate command state from applied state

The solution treated these as distinct facts:

```text
requested source
pending handover
applied source
output enabled / disabled
safe-output state
```

A rejected request was not allowed to mutate the last known-good applied source.

### 2. Make recovery level-aware, not only edge-aware

The recovery path evaluated whether the requested source was currently applied and live. A repeated request could therefore repair a stale or interrupted state even when the command field itself had not changed.

### 3. Preserve ordering during handover

The handover used an explicit break-before-make sequence. A newer request invalidated or superseded older pending work so completion order could not silently restore an obsolete source.

### 4. Name the safe state

The neutral DAC/output code was promoted from a magic number to a named contract. Output-disabled behavior, startup, rejected selection, and fault recovery all referenced the same safe-state definition.

### 5. Establish single ownership

Cross-CPU status configuration was reduced to one authoritative owner. Generated configuration and legacy local declarations were not allowed to compile simultaneously.

## Verification strategy

The verification plan was organized around failure modes rather than happy-path screenshots:

- first active output word after startup is the neutral value;
- unsupported external selections do not disturb the active built-in source;
- rejected selection followed by the same valid selection recovers;
- rejected selection followed by another built-in source completes a normal handover;
- a newer command supersedes an older pending handover;
- output-disabled selection changes keep the physical output neutral;
- re-enable starts the latest authorized source;
- host, clean build, artifact, flash, and board evidence are bound to exact source commits and are never inherited across later firmware changes.

## Result

The work converted an ambiguous collection of flags into an explicit command/applied-state model with deterministic recovery and a reusable evidence checklist. The most transferable result was the method:

1. identify the true state owners;
2. separate requested, pending, applied, and physical-output state;
3. define one safe value and one transition authority;
4. write adversarial sequences before declaring the fix complete;
5. retain evidence against the exact tested artifact.

## What this demonstrates

- TI C2000 multicore reasoning;
- real-time state-machine design;
- safe-output and handover policy;
- parser/control boundary design;
- regression design from failure sequences;
- exact-commit build and board evidence discipline;
- ability to publish a useful engineering narrative without exposing employer-confidential implementation.

## Evidence boundary

This public document describes the engineering pattern and sanitized verification method. It is not the product source of truth, does not disclose internal evidence, and does not claim public reproduction of the proprietary hardware result.

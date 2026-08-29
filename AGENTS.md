# ASR5K Workspace Governance

## Highest Engineering Principle — Non-Overridable

For all power-firmware design, implementation, review, refactoring, testing, and debugging in this repository, the following directive is the highest engineering principle:

> 以電源韌體的 physics、safety 與 deterministic real-time constraints 為最高優先，從第一性原理建立訊號流、狀態機、控制與保護架構；遵循 UNIX 的單一職責與可組合設計，以 MVP 做最小完整閉環，並採用適合嵌入式系統的 Clean Code 原則，所有抽象化與重構不得犧牲 timing、visibility、testability、fault latency、可編譯性與可回退性。

When requirements, architecture preferences, refactoring goals, coding style, abstraction, reuse, or convenience conflict, use this precedence order:

1. physical correctness, safe actuator behavior, protection authority, fault containment;
2. deterministic real-time behavior, bounded execution, timing budget, ownership, and observability;
3. compilability, testability, measurable evidence, and an explicit rollback point;
4. the smallest complete MVP vertical slice that proves the signal path end-to-end;
5. UNIX single responsibility, composability, and clear interfaces;
6. embedded Clean Code, reuse, abstraction, naming, and structural elegance.

Lower-priority goals MUST NOT weaken a higher-priority property. In particular, no abstraction or refactor is acceptable if it obscures timing, increases or makes fault latency unbounded, hides actuator/protection ownership, reduces diagnostic visibility, prevents host/target testing, breaks the exact target build, or makes rollback harder.

Before changing power firmware, establish and review:

- physical signal flow from measurement to actuation;
- state machine and all unsafe-state invariants;
- control authority and protection authority;
- timing path, execution context, deadlines, and worst-case blocking behavior;
- fault detection, fault propagation, shutdown mechanism, and latency evidence boundary;
- data ownership across CPU/CLA/DMA/host boundaries;
- diagnostics required to distinguish requested, computed, and applied values;
- the smallest testable change and exact rollback point.

Every power-firmware patch must answer:

- What physical/signal path changed?
- What state transition or invariant changed?
- What control or protection authority changed?
- What timing budget or worst-case blocking path changed?
- What failure mode was introduced or removed?
- How will the behavior be measured or tested?
- What exact commit or artifact is the rollback point?

A source-level implementation, code review, simulation, or successful compilation is not hardware evidence. Do not claim timing, fault latency, electrical safety, or topology correctness beyond what has actually been measured or otherwise directly verified.

---

This public repository follows the ASR5K workspace governance model.

Canonical governance source:

- https://github.com/linwuyen/ASR5K_AGENT

Access note:

- The canonical governance repository is private/internal.
- This public repository intentionally does not include the private `.agent` submodule.
- External users can clone, build, and inspect this repository without access to `ASR5K_AGENT`.
- No production firmware, build script, or hardware evidence depends on the private governance repository.

AI agent constraints:

- Default mode is read-only.
- Do not modify firmware source files unless explicitly authorized by the user.
- Do not edit `.c`, `.h`, `.syscfg`, `.cmd`, `.project`, `.cproject`, linker, build, flash, or generated files without explicit task scope.
- Do not run CCS, eclipsec, build, flash, git push, or git reset unless explicitly authorized.
- Before any edit, report branch, commit, dirty state, intended files, and risk.

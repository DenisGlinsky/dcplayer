# All task-specific AGENTS templates


---

# AGENTS.md — T00a Repository bootstrap
## 1. Task goal

Set up the repository skeleton, build system, presets, scripts, and placeholder targets so later branches can work without re-laying the foundation.

## 2. Out of scope

- No DCP parsing logic.
- No security or KDM logic.
- No playback features.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `docs/CODE_STYLE.md`
- `docs/STATUS.md`

## 4. Files allowed to modify

- `CMakeLists.txt`
- `cmake/**`
- `scripts/**`
- `apps/**/CMakeLists.txt`
- `src/**/CMakeLists.txt`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`

## 5. Deliverables

- CMake project root and presets.
- Bootstrap and smoke scripts.
- Directory skeleton and placeholder targets.
- Initial build/test instructions.

## 6. Definition of Done

- Repository configures on the primary Linux target.
- Repository builds without ad-hoc manual fixes.
- Scaffold-level tests or smoke checks pass.

## 7. Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
./scripts/bootstrap.sh
./scripts/smoke.sh
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T00b Formal companion specs freeze
## 1. Task goal

Create or refresh the baseline companion specs that define the domain objects, contracts, and invariants used by later branches.

## 2. Out of scope

- No parser or runtime implementation.
- No UI or playback code.
- No transport integration.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `docs/CODE_STYLE.md`
- `docs/IMPLEMENT.md`
- `docs/STATUS.md`

## 4. Files allowed to modify

- `specs/objects/**`
- `docs/IMPLEMENT.md`
- `docs/STATUS.md`
- `docs/ARCHITECTURE.md`

## 5. Deliverables

- Object spec files listed in `docs/PLAN.md` section 5.
- Consistent fields, invariants, validation rules, and versioning notes.
- Spec cross-links where needed.

## 6. Definition of Done

- All baseline specs exist.
- Spec style is consistent.
- Later tasks can cite specs instead of inventing contracts in code.

## 7. Verification

```bash
grep -R "^# " specs/objects
markdownlint specs/objects || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T00c Project memory discipline
## 1. Task goal

Lock down the rules for durable project memory, handoff format, artifact naming, and task-local readsets.

## 2. Out of scope

- No product features.
- No parser or playback work.
- No infrastructure changes beyond documentation and templates.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/IMPLEMENT.md`
- `docs/STATUS.md`
- `docs/CODE_STYLE.md`

## 4. Files allowed to modify

- `AGENTS.md`
- `tasks/**`
- `docs/IMPLEMENT.md`
- `docs/STATUS.md`
- `docs/PLAN.md`
- `docs/CODE_STYLE.md`

## 5. Deliverables

- Documented handoff format.
- Artifact naming and storage rules.
- Task template rules for required reading sets and stop conditions.

## 6. Definition of Done

- A future branch can end with a clean reproducible handoff.
- Rules are short, explicit, and enforceable.

## 7. Verification

```bash
test -f AGENTS.md
find tasks -name AGENTS.md | sort | head
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T01a ASSETMAP + PKL parser
## 1. Task goal

Implement parse and validation for ASSETMAP and PKL into normalized domain objects.

## 2. Out of scope

- No CPL parsing.
- No resolver logic.
- No playback.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `docs/CODE_STYLE.md`
- `specs/objects/AssetMap.md`
- `specs/objects/PKL.md`
- `docs/STATUS.md`

## 4. Files allowed to modify

- `src/dcp/assetmap/**`
- `src/dcp/pkl/**`
- `tests/fixtures/**`
- `tests/unit/**`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`

## 5. Deliverables

- ASSETMAP parser.
- PKL parser.
- Validation diagnostics.
- Valid and invalid fixtures.

## 6. Definition of Done

- Valid fixtures parse into normalized models.
- Invalid fixtures fail deterministically.
- Unit tests cover the main validation paths.

## 7. Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build --output-on-failure -R T01a || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T01b CPL parser
## 1. Task goal

Implement parse and validation for CPL, reels, track references, edit rate, and composition metadata.

## 2. Out of scope

- No OV/VF resolution.
- No timeline generation.
- No playback.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `docs/CODE_STYLE.md`
- `specs/objects/CPL.md`
- `specs/objects/Reel.md`
- `specs/objects/TrackFile.md`

## 4. Files allowed to modify

- `src/dcp/cpl/**`
- `tests/fixtures/**`
- `tests/unit/**`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`

## 5. Deliverables

- CPL parser.
- Reel and track reference model integration.
- Negative fixtures and diagnostics.

## 6. Definition of Done

- CPL parse output is deterministic.
- Invalid structures fail clearly.
- Track references are represented in the domain model.

## 7. Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build --output-on-failure -R T01b || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T01c dcp_probe and composition graph
## 1. Task goal

Build `dcp_probe` so it reads a DCP directory and emits a deterministic normalized composition graph.

## 2. Out of scope

- No decode or playback.
- No security logic.
- No TMS/UI.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `specs/objects/CompositionGraph.md`
- `docs/CODE_STYLE.md`
- `docs/STATUS.md`

## 4. Files allowed to modify

- `apps/dcp_probe/**`
- `src/dcp/**`
- `tests/integration/**`
- `tests/fixtures/**`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`

## 5. Deliverables

- `apps/dcp_probe` CLI.
- Stable JSON output format.
- Interop and SMPTE integration fixtures/tests.

## 6. Definition of Done

- `dcp_probe <path>` returns deterministic JSON.
- CLI exit codes are stable.
- Integration tests cover at least one Interop and one SMPTE sample.

## 7. Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build --output-on-failure -R T01c || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T02a OV/VF resolver
## 1. Task goal

Resolve OV/VF dependencies and detect missing assets, broken references, and graph conflicts.

## 2. Out of scope

- No supplemental merge rules beyond what is needed to keep scope isolated.
- No timeline rendering.
- No playback.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `specs/objects/CompositionGraph.md`
- `specs/objects/CPL.md`
- `docs/CODE_STYLE.md`
- `docs/STATUS.md`

## 4. Files allowed to modify

- `src/dcp/ov_vf_resolver/**`
- `tests/fixtures/**`
- `tests/unit/**`
- `tests/integration/**`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`

## 5. Deliverables

- OV/VF resolver.
- Missing/conflict diagnostics.
- Effective composition graph output.

## 6. Definition of Done

- OV and VF relationships resolve on fixtures.
- Broken references produce deterministic diagnostics.

## 7. Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build --output-on-failure -R T02a || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T02b Supplemental semantics
## 1. Task goal

Define and implement supplemental package merge and override rules without expanding into playback.

## 2. Out of scope

- No decode or playout.
- No security work.
- No UI.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `specs/objects/SupplementalMergePolicy.md`
- `specs/objects/CompositionGraph.md`
- `docs/STATUS.md`

## 4. Files allowed to modify

- `src/dcp/supplemental/**`
- `src/dcp/ov_vf_resolver/**`
- `specs/objects/SupplementalMergePolicy.md`
- `tests/fixtures/**`
- `tests/unit/**`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`

## 5. Deliverables

- Supplemental merge policy.
- Implementation support in resolver path.
- OV+VF+supplemental fixtures.

## 6. Definition of Done

- Supplemental merge behavior is documented and deterministic.
- Resolver output is reproducible on fixtures.

## 7. Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build --output-on-failure -R T02b || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T02c Dry-run playback timeline
## 1. Task goal

Produce a playback timeline by reel without decode or playout.

## 2. Out of scope

- No decoder implementation.
- No subtitles or audio rendering.
- No security integration.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `specs/objects/PlaybackTimeline.md`
- `specs/objects/CompositionGraph.md`
- `docs/STATUS.md`

## 4. Files allowed to modify

- `src/playout/timeline/**`
- `src/dcp/**`
- `tests/unit/**`
- `tests/integration/**`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`

## 5. Deliverables

- Timeline builder.
- Machine-readable timeline dump.
- Validation diagnostics.

## 6. Definition of Done

- Timeline output is deterministic.
- Timeline covers picture, audio, and text tracks at the planning level.

## 7. Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build --output-on-failure -R T02c || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T03a PKI and trust store baseline
## 1. Task goal

Define and implement certificate store, trust anchors, chain validation, and revocation handling.

## 2. Out of scope

- No mutual-TLS transport implementation.
- No decrypt/sign APIs.
- No KDM logic.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `specs/objects/CertificateStore.md`
- `specs/objects/TrustChain.md`
- `docs/CODE_STYLE.md`
- `docs/STATUS.md`

## 4. Files allowed to modify

- `src/security_api/certs/**`
- `specs/objects/CertificateStore.md`
- `specs/objects/TrustChain.md`
- `tests/unit/**`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`

## 5. Deliverables

- Certificate store implementation or formalized skeleton.
- Trust chain validation rules.
- CRL/OCSP contract notes and tests.

## 6. Definition of Done

- Trust store import and validation are explicit.
- Revocation behavior is defined and testable.

## 7. Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build --output-on-failure -R T03a || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T03b Pi/ZymKey server ↔ Ubuntu/TPM client contract
## 1. Task goal

Formalize the mutual-TLS secure channel contract between the Pi+ZymKey service and the Ubuntu+TPM media host.

## 2. Out of scope

- No full decrypt service implementation.
- No KDM ingest.
- No playback orchestration.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `specs/objects/SecureChannelContract.md`
- `specs/objects/CertificateStore.md`
- `specs/objects/TrustChain.md`
- `docs/STATUS.md`

## 4. Files allowed to modify

- `src/security_api/host_spb1_contract/**`
- `spb1/api/**`
- `specs/objects/SecureChannelContract.md`
- `docs/ARCHITECTURE.md`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`
- `tests/integration/security/**`

## 5. Deliverables

- Mutual-TLS contract.
- Identity and ACL assumptions.
- Handshake sequence diagrams or equivalent.
- Acceptance criteria for allowed/denied handshake paths.

## 6. Definition of Done

- Handshake rules are explicit and testable.
- The contract captures server key in ZymKey, client key in TPM, cert validation, and device identity checks.

## 7. Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build --output-on-failure -R T03b || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T03c ACL + protected API envelope
## 1. Task goal

Define the protected request/response envelope and authorization model for secure operations such as sign, unwrap, and decrypt.

## 2. Out of scope

- No full device firmware.
- No playback code.
- No UI.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `specs/objects/ProtectedApiEnvelope.md`
- `specs/objects/SecureChannelContract.md`
- `docs/STATUS.md`

## 4. Files allowed to modify

- `src/security_api/host_spb1_contract/**`
- `spb1/api/**`
- `specs/objects/ProtectedApiEnvelope.md`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`
- `tests/integration/security/**`

## 5. Deliverables

- Protected API envelope spec.
- ACL and identity rules.
- Error codes and idempotency notes.

## 6. Definition of Done

- Secure operations can be added under one envelope without redefining the transport model.
- ACL checks and audit fields are explicit.

## 7. Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build --output-on-failure -R T03c || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T03d Audit, tamper, escrow, recovery
## 1. Task goal

Define and implement signed/encrypted audit semantics, tamper reactions, escrow handling, and recovery procedure support.

## 2. Out of scope

- No playout orchestration.
- No GPU work.
- No TMS features.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `specs/objects/SecurityEvent.md`
- `specs/objects/ProtectedApiEnvelope.md`
- `docs/STATUS.md`

## 4. Files allowed to modify

- `src/security_api/log_client/**`
- `spb1/diag/**`
- `specs/objects/SecurityEvent.md`
- `docs/ARCHITECTURE.md`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`
- `tests/integration/security/**`

## 5. Deliverables

- Security event model.
- Tamper state transitions.
- Escrow/recovery procedure notes or simulator support.
- Protected audit export rules.

## 6. Definition of Done

- Tamper and recovery flows are explicit.
- Audit fields are stable and machine-readable.
- The design reflects local/encrypted logging, tamper, recovery, and escrow requirements.

## 7. Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build --output-on-failure -R T03d || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T03e Secure time semantics
## 1. Task goal

Define secure UTC time, skew policy, timestamp rules, and show-window dependencies for the secure module.

## 2. Out of scope

- No KDM ingest implementation beyond what is required to test time semantics.
- No playback orchestration.
- No UI.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `specs/objects/SecurityModuleContract.md`
- `specs/objects/KDMValidationResult.md`
- `docs/STATUS.md`

## 4. Files allowed to modify

- `src/security_api/**/time*`
- `spb1/api/**`
- `specs/objects/SecurityModuleContract.md`
- `specs/objects/KDMValidationResult.md`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`
- `tests/unit/**`
- `tests/integration/security/**`

## 5. Deliverables

- Secure time contract.
- Skew/drift policy.
- Negative tests for inconsistent time and show-window conditions.

## 6. Definition of Done

- Time semantics are explicit and reusable by KDM validation and audit logging.

## 7. Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build --output-on-failure -R T03e || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T04a ISecurityModule + simulator
## 1. Task goal

Define the host-side `ISecurityModule` interface and provide a non-production simulator implementation.

## 2. Out of scope

- No real hardware crypto.
- No production secret handling.
- No playback beyond contract testing.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `specs/objects/SecurityModuleContract.md`
- `docs/CODE_STYLE.md`
- `docs/STATUS.md`

## 4. Files allowed to modify

- `src/security_api/**`
- `spb1/simulator/**`
- `specs/objects/SecurityModuleContract.md`
- `tests/unit/**`
- `tests/integration/security/**`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`

## 5. Deliverables

- `ISecurityModule` interface.
- Simulator backend.
- Contract tests for time, cert, KDM, log, and FM-state methods.

## 6. Definition of Done

- Host-side code can compile against the interface only.
- Simulator supports contract testing without real hardware.

## 7. Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build --output-on-failure -R T04a || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T04b KDM ingest and binding
## 1. Task goal

Implement KDM ingest, validation windows, and binding to CPL/composition identity.

## 2. Out of scope

- No secure playout orchestration.
- No GPU decode.
- No TMS UI.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `specs/objects/KDMEnvelope.md`
- `specs/objects/KDMValidationResult.md`
- `specs/objects/CPL.md`
- `docs/STATUS.md`

## 4. Files allowed to modify

- `src/security_api/kdm/**`
- `src/dcp/cpl/**`
- `specs/objects/KDMEnvelope.md`
- `specs/objects/KDMValidationResult.md`
- `tests/unit/**`
- `tests/integration/**`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`

## 5. Deliverables

- KDM ingest pipeline.
- Binding rules to compositions/CPLs.
- Valid/invalid/expired/future test cases.

## 6. Definition of Done

- KDM scenarios are deterministic and fail closed.
- Binding behavior is explicit and covered by tests.

## 7. Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build --output-on-failure -R T04b || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T04c Fail-closed playback state machine
## 1. Task goal

Define and implement the security-critical playback state machine with explicit denial paths and no silent fallback.

## 2. Out of scope

- No full orchestrator.
- No UI beyond operator-visible state reasons.
- No unrelated refactors.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `specs/objects/PlaybackSession.md`
- `specs/objects/SecurityModuleContract.md`
- `docs/STATUS.md`

## 4. Files allowed to modify

- `src/playout/**`
- `src/security_api/**`
- `specs/objects/PlaybackSession.md`
- `docs/ARCHITECTURE.md`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`
- `tests/unit/**`
- `tests/integration/**`

## 5. Deliverables

- State machine definition.
- Explicit transition and denial rules.
- Operator-visible failure reasons.

## 6. Definition of Done

- No hidden degraded mode remains.
- Each denial path is logged and testable.

## 7. Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build --output-on-failure -R T04c || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T04d Security event export
## 1. Task goal

Export security events to the host plane in an operationally useful but secret-safe format.

## 2. Out of scope

- No web UI.
- No schedule features.
- No unrelated logging systems.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `specs/objects/SecurityEvent.md`
- `specs/objects/OperatorAlert.md`
- `docs/STATUS.md`

## 4. Files allowed to modify

- `src/security_api/log_client/**`
- `src/core/alerts/**`
- `specs/objects/SecurityEvent.md`
- `specs/objects/OperatorAlert.md`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`
- `tests/unit/**`
- `tests/integration/**`

## 5. Deliverables

- Security event export schema.
- Filtering rules for host visibility.
- Operator alert mapping.

## 6. Definition of Done

- Host receives only minimal operational data.
- Event export is deterministic and documented.

## 7. Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build --output-on-failure -R T04d || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T05a J2K backend abstraction
## 1. Task goal

Define the common backend interface for CPU and GPU JPEG 2000 decode, including surfaces, queueing, and error reporting.

## 2. Out of scope

- No production decoder implementation beyond scaffolding.
- No watermark insertion.
- No playout orchestration.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `docs/CODE_STYLE.md`
- `docs/STATUS.md`
- `specs/objects/PlaybackSession.md`

## 4. Files allowed to modify

- `src/video/**`
- `src/playout/frame_queue/**`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`
- `tests/unit/**`

## 5. Deliverables

- Backend interface.
- Frame surface model.
- Queueing and error model.
- Perf counter hooks.

## 6. Definition of Done

- Playout code can target CPU and GPU backends through one stable interface.

## 7. Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build --output-on-failure -R T05a || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T05b CPU reference decoder
## 1. Task goal

Create the deterministic CPU baseline decoder used for validation, regression, and fallback.

## 2. Out of scope

- No production GPU implementation.
- No secure HSM integration.
- No FM insertion.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `docs/LIBRARIES.md`
- `docs/STATUS.md`
- `specs/objects/PlaybackSession.md`

## 4. Files allowed to modify

- `src/video/j2k_cpu/**`
- `src/video/**`
- `tests/unit/**`
- `tests/integration/**`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`

## 5. Deliverables

- CPU decode path.
- Reference outputs or comparison hooks.
- Regression fixtures.

## 6. Definition of Done

- CPU decoder is deterministic enough to serve as an oracle for later GPU work.

## 7. Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build --output-on-failure -R T05b || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T05c GPU decode contract
## 1. Task goal

Define the GPU memory ownership, handoff boundary, queueing, and synchronization model for `decrypt -> decode`.

## 2. Out of scope

- No vendor-specific backend implementation.
- No playout orchestrator.
- No watermark insertion.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `docs/IMPLEMENT.md`
- `docs/STATUS.md`
- `specs/objects/PlaybackSession.md`

## 4. Files allowed to modify

- `src/video/j2k_gpu_open/**`
- `src/video/**`
- `src/playout/frame_queue/**`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`
- `tests/unit/**`

## 5. Deliverables

- GPU decode contract.
- Queueing and synchronization model.
- Perf counters and failure surface.

## 6. Definition of Done

- Secure integration and vendor backends can target one stable GPU contract.

## 7. Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build --output-on-failure -R T05c || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T05d NVIDIA backend
## 1. Task goal

Implement the production NVIDIA GPU decode backend against the common J2K backend contract.

## 2. Out of scope

- No Intel/AMD/Apple backends.
- No FM insertion.
- No secure orchestration beyond what is needed for backend tests.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `docs/LIBRARIES.md`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`

## 4. Files allowed to modify

- `src/video/j2k_gpu_open/**`
- `src/video/nvidia/**`
- `src/video/**`
- `tests/integration/**`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`

## 5. Deliverables

- NVIDIA backend.
- Metrics and diagnostics hooks.
- Smoke tests on reference samples.

## 6. Definition of Done

- Reference GPU hardware can run the backend with reproducible smoke results.

## 7. Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build --output-on-failure -R T05d || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T06a Watermark object model
## 1. Task goal

Define watermark payload, control flags, evidence format, attack matrix, and state semantics for picture and sound FM work.

## 2. Out of scope

- No GPU insertion code.
- No detector implementation beyond interfaces.
- No secure orchestration.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `specs/objects/WatermarkPayload.md`
- `specs/objects/WatermarkEvidence.md`
- `docs/STATUS.md`

## 4. Files allowed to modify

- `specs/objects/WatermarkPayload.md`
- `specs/objects/WatermarkEvidence.md`
- `src/watermark/fm_backend_api/**`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`

## 5. Deliverables

- Payload spec.
- Evidence spec.
- Attack matrix notes.
- Control/state model.

## 6. Definition of Done

- Detector and inserter branches can rely on one stable object model and one attack-matrix vocabulary.

## 7. Verification

```bash
grep -R "Watermark" specs/objects src/watermark/fm_backend_api || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T06b Detector harness
## 1. Task goal

Build the offline detector harness and scoring/report format for approved degradation scenarios.

## 2. Out of scope

- No production FM insertion in playback.
- No UI.
- No secure orchestration.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `specs/objects/WatermarkPayload.md`
- `specs/objects/WatermarkEvidence.md`
- `docs/STATUS.md`

## 4. Files allowed to modify

- `src/watermark/detector_harness/**`
- `tests/compliance/**`
- `tests/fixtures/**`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`

## 5. Deliverables

- Detector harness.
- Attack corpus policy.
- Scoring/report format.

## 6. Definition of Done

- The project can express what counts as successful identification for the approved attack matrix.

## 7. Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build --output-on-failure -R T06b || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T06c GPU picture FM prototype
## 1. Task goal

Implement picture watermark insertion on the GPU path and wire it to the detector harness.

## 2. Out of scope

- No sound FM.
- No final secure orchestration.
- No UI.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `specs/objects/WatermarkPayload.md`
- `specs/objects/WatermarkEvidence.md`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`

## 4. Files allowed to modify

- `src/watermark/fm_backend_api/**`
- `src/video/**`
- `src/playout/**`
- `tests/integration/**`
- `tests/compliance/**`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`

## 5. Deliverables

- GPU picture FM insertion path.
- State/log hooks.
- Detector harness integration tests.

## 6. Definition of Done

- Picture FM is injected on the GPU path and measurable through detector output and logs.

## 7. Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build --output-on-failure -R T06c || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T06d Secure playback integration
## 1. Task goal

Integrate GPU picture FM into the secure encrypted show chain between decrypt handoff and playout.

## 2. Out of scope

- No sound FM.
- No TMS features.
- No open-mode orchestration changes unless required for shared interfaces.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `specs/objects/PlaybackSession.md`
- `specs/objects/WatermarkPayload.md`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`

## 4. Files allowed to modify

- `src/playout/**`
- `src/video/**`
- `src/watermark/**`
- `src/security_api/**`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`
- `tests/integration/**`
- `tests/compliance/**`

## 5. Deliverables

- Integrated secure chain from decrypt handoff to playout.
- Explicit FM state propagation.
- Failure handling for unavailable or denied FM state.

## 6. Definition of Done

- Secure chain uses `HSM decrypt -> GPU decode -> GPU picture FM -> playout` with explicit state transitions.

## 7. Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build --output-on-failure -R T06d || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T06e Sound FM reference path
## 1. Task goal

Create the separate sound forensic watermark reference path and connect it to the detector harness.

## 2. Out of scope

- No picture FM changes unless needed for shared evidence interfaces.
- No TMS work.
- No AES3 output.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `specs/objects/WatermarkPayload.md`
- `specs/objects/WatermarkEvidence.md`
- `docs/STATUS.md`

## 4. Files allowed to modify

- `src/watermark/**`
- `src/audio/**`
- `tests/compliance/**`
- `tests/integration/**`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`

## 5. Deliverables

- Sound FM reference insertion.
- Detector integration.
- Sound attack-matrix coverage.

## 6. Definition of Done

- Sound FM exists as a separate tested track and does not piggyback on picture FM assumptions.

## 7. Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build --output-on-failure -R T06e || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T07a PCM engine + channel routing
## 1. Task goal

Implement the multichannel PCM engine and label-driven channel routing.

## 2. Out of scope

- No AES3 hardware adapter yet.
- No TMS/UI.
- No secure orchestration.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `docs/STATUS.md`
- `specs/objects/PlaybackTimeline.md`

## 4. Files allowed to modify

- `src/audio/pcm_engine/**`
- `src/audio/channel_router/**`
- `tests/unit/**`
- `tests/integration/**`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`

## 5. Deliverables

- PCM engine.
- Label-driven router.
- Routing fixtures and validation tests.

## 6. Definition of Done

- Routing no longer depends on hardcoded channel assumptions.

## 7. Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build --output-on-failure -R T07a || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T07b Audio clock and sync stabilization
## 1. Task goal

Stabilize synchronization between audio output and the playback timeline.

## 2. Out of scope

- No AES3 adapter.
- No subtitle work.
- No UI.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `docs/STATUS.md`
- `specs/objects/PlaybackTimeline.md`
- `specs/objects/PlaybackSession.md`

## 4. Files allowed to modify

- `src/audio/**`
- `src/playout/sync/**`
- `tests/unit/**`
- `tests/integration/**`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`

## 5. Deliverables

- Audio clock model.
- Drift handling.
- Synchronization tests.

## 6. Definition of Done

- Audio remains synchronized under the expected load envelope.

## 7. Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build --output-on-failure -R T07b || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T07c AES3 adapter
## 1. Task goal

Implement the 24-bit AES3 output adapter as a dedicated isolated subsystem.

## 2. Out of scope

- No broader audio engine redesign.
- No TMS/UI.
- No sound FM.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `docs/STATUS.md`
- `specs/objects/PlaybackSession.md`

## 4. Files allowed to modify

- `src/audio/aes3_adapter/**`
- `src/audio/**`
- `tests/integration/**`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`

## 5. Deliverables

- AES3 device abstraction.
- Output contract.
- Loopback or integration tests.

## 6. Definition of Done

- AES3 output path is isolated, documented, and testable.

## 7. Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build --output-on-failure -R T07c || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T07d Interop subtitles
## 1. Task goal

Implement Interop subtitle/subpicture support with parse, timing, and render integration.

## 2. Out of scope

- No SMPTE timed text in this branch.
- No audio work.
- No secure orchestration changes unless required by shared render hooks.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `docs/STATUS.md`
- `specs/objects/PlaybackTimeline.md`

## 4. Files allowed to modify

- `src/subtitles/interop/**`
- `src/playout/**`
- `tests/fixtures/**`
- `tests/integration/**`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`

## 5. Deliverables

- Interop subtitle parser.
- Timing integration.
- Render integration tests.

## 6. Definition of Done

- Interop subtitles render with correct timing on reference fixtures.

## 7. Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build --output-on-failure -R T07d || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T07e SMPTE timed text
## 1. Task goal

Implement SMPTE Timed Text with font handling and timing/render integration.

## 2. Out of scope

- No Interop subtitle changes except shared hooks.
- No audio work.
- No TMS/UI.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `docs/STATUS.md`
- `specs/objects/PlaybackTimeline.md`

## 4. Files allowed to modify

- `src/subtitles/smpte_tt/**`
- `src/playout/**`
- `tests/fixtures/**`
- `tests/integration/**`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`

## 5. Deliverables

- SMPTE timed text parser.
- Font handling.
- Timing and render integration tests.

## 6. Definition of Done

- SMPTE timed text behaves deterministically on reference fixtures.

## 7. Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build --output-on-failure -R T07e || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T08a Open playout orchestrator
## 1. Task goal

Build the open-content playout orchestrator used for baseline testing and development.

## 2. Out of scope

- No secure HSM path.
- No encrypted show control.
- No TMS UI.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `specs/objects/PlaybackSession.md`
- `specs/objects/PlaybackTimeline.md`
- `docs/STATUS.md`

## 4. Files allowed to modify

- `src/playout/**`
- `apps/playout_service/**`
- `tests/integration/**`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`

## 5. Deliverables

- Open playout orchestrator.
- Timeline-driven session control.
- Metrics and logging hooks.

## 6. Definition of Done

- Open playback is reproducible and observable on reference fixtures.

## 7. Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build --output-on-failure -R T08a || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T08b Secure playout orchestrator
## 1. Task goal

Assemble the full secure show chain from local storage through HSM decrypt, GPU decode, GPU picture FM, and playout.

## 2. Out of scope

- No SMS/TMS work.
- No sound FM beyond already defined interfaces.
- No unrelated open-path refactors.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `specs/objects/PlaybackSession.md`
- `specs/objects/SecurityModuleContract.md`
- `specs/objects/WatermarkPayload.md`
- `docs/STATUS.md`

## 4. Files allowed to modify

- `src/playout/**`
- `src/security_api/**`
- `src/video/**`
- `src/watermark/**`
- `apps/playout_service/**`
- `tests/integration/**`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`

## 5. Deliverables

- Secure session orchestrator.
- Coordination between host, security module, GPU decode, and FM.
- Explicit failure handling.

## 6. Definition of Done

- Secure playback follows the fixed chain with explicit state transitions and no silent fallback.

## 7. Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build --output-on-failure -R T08b || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T08c Fault matrix
## 1. Task goal

Define and implement explicit behavior for expected failures such as network loss, HSM denial, KDM invalidation, GPU failure, and FM unavailability.

## 2. Out of scope

- No new playback features outside failure handling.
- No UI beyond operator-facing failure reasons.
- No unrelated refactors.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `specs/objects/FaultMatrix.md`
- `specs/objects/OperatorAlert.md`
- `docs/STATUS.md`

## 4. Files allowed to modify

- `specs/objects/FaultMatrix.md`
- `src/playout/**`
- `src/core/alerts/**`
- `tests/integration/**`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`

## 5. Deliverables

- Fault matrix spec.
- Implementation of expected responses.
- Negative tests and operator alert mapping.

## 6. Definition of Done

- Each expected fault has a defined response, log entry, and operator signal.

## 7. Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build --output-on-failure -R T08c || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T08d Encrypted show rehearsal
## 1. Task goal

Run a full encrypted playback rehearsal and produce a reproducible runbook and evidence pack.

## 2. Out of scope

- No new product features unless they block the rehearsal.
- No UI expansion.
- No opportunistic refactors.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `docs/IMPLEMENT.md`
- `docs/STATUS.md`
- `specs/objects/PlaybackSession.md`

## 4. Files allowed to modify

- `docs/IMPLEMENT.md`
- `docs/STATUS.md`
- `tests/compliance/**`
- `tests/integration/**`
- `scripts/**`
- `artifacts/**`

## 5. Deliverables

- Rehearsal procedure.
- Evidence pack.
- Acceptance checklist and known-gaps note.

## 6. Definition of Done

- An encrypted show can be rehearsed reproducibly and audited.

## 7. Verification

```bash
./scripts/rehearse_encrypted_show.sh || true
ctest --test-dir build --output-on-failure -R T08d || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T09a Local SMS API
## 1. Task goal

Implement the local control API for one auditorium, including state, transport-safe commands, inventory, and alerts.

## 2. Out of scope

- No full TMS web.
- No remote multi-site features.
- No unrelated UI framework work.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `specs/objects/OperatorAlert.md`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`

## 4. Files allowed to modify

- `apps/sms_ui/**`
- `src/core/**`
- `src/playout/**`
- `tests/integration/**`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`

## 5. Deliverables

- Local SMS API.
- Play/pause/stop and state control.
- Inventory and alerts endpoints.

## 6. Definition of Done

- One auditorium can be controlled through a stable local API.

## 7. Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build --output-on-failure -R T09a || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T09b Minimal TMS web
## 1. Task goal

Implement the minimal TMS web UI for inventory, CPL/KDM status, scheduling, alerts, and remote launch.

## 2. Out of scope

- No broad enterprise TMS scope.
- No unrelated design system work.
- No security refactors outside what is needed for status display.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `specs/objects/OperatorAlert.md`
- `specs/objects/ScheduleEntry.md`
- `docs/STATUS.md`

## 4. Files allowed to modify

- `apps/tms_web/**`
- `src/core/**`
- `src/playout/**`
- `tests/integration/**`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`

## 5. Deliverables

- TMS web UI.
- Status dashboard.
- Schedule and alert views.
- Remote launch hook.

## 6. Definition of Done

- Operator can see missing CPL, invalid KDM, and expiring show-window alerts.

## 7. Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build --output-on-failure -R T09b || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


---

# AGENTS.md — T09c Monitoring and operator audit
## 1. Task goal

Unify operator logs, role model, observability, and incident export across playback, security, and scheduling.

## 2. Out of scope

- No new playback features.
- No broad analytics platform.
- No unrelated infrastructure refactor.

## 3. Required reading set

- `AGENTS.md`
- `docs/PLAN.md`
- `docs/ARCHITECTURE.md`
- `specs/objects/SecurityEvent.md`
- `specs/objects/OperatorAlert.md`
- `docs/STATUS.md`

## 4. Files allowed to modify

- `src/core/**`
- `apps/sms_ui/**`
- `apps/tms_web/**`
- `tests/integration/**`
- `docs/STATUS.md`
- `docs/IMPLEMENT.md`

## 5. Deliverables

- Unified operator audit model.
- Observability view.
- Incident/export support.

## 6. Definition of Done

- Operator-facing monitoring is coherent across playback, security, and scheduling.

## 7. Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build --output-on-failure -R T09c || true
```

## 8. Handoff updates

- Update `docs/STATUS.md` with what changed, what was verified, and what remains open.
- Update `docs/IMPLEMENT.md` if implementation details, file layout, or command flow changed.
- Update `docs/ARCHITECTURE.md` only if architectural decisions changed.
- Update `docs/PLAN.md` only for status, dependency, or sequencing notes relevant to this task.

## 9. Stop conditions

- Stop if completing this task requires pulling in a neighboring task from the plan.
- Stop if the task would exceed the allowed file surface without a clear justification.
- Stop if object contracts must change and the companion spec was not updated first.
- Do not perform opportunistic refactors outside the task scope.

## 10. Reporting format

At the end of the branch, report in this order:

- What was done.
- Which files changed.
- How it was verified.
- What is intentionally not covered.
- Which risks or follow-ups remain.


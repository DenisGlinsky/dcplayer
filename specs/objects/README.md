# Companion object specifications

Эта директория хранит канонические companion-specs доменных, security и operational объектов проекта.
Спецификация — это объектный контракт, а не комментарий к будущему коду.

## Общие правила

1. Для каждого объекта есть отдельный *.md файл.
2. Каноническая JSON-сериализация использует snake_case.
3. Время хранится в UTC ISO-8601 с суффиксом Z.
4. Идентификаторы передаются строками; бинарные blob не используются как transport contract.
5. Если объект меняется семантически, сначала обновляется spec, затем код и docs/STATUS.md.

## Канонический baseline

- `AssetMap.md`
- `PKL.md`
- `CPL.md`
- `Reel.md`
- `TrackFile.md`
- `CompositionGraph.md`
- `PlaybackTimeline.md`
- `SupplementalMergePolicy.md`
- `CertificateStore.md`
- `TrustChain.md`
- `SecureChannelContract.md`
- `ProtectedApiEnvelope.md`
- `SecurityModuleContract.md`
- `SecureClockPolicy.md`
- `HsmDecryptSession.md`
- `KDMEnvelope.md`
- `KDMValidationResult.md`
- `SecurityEvent.md`
- `AuditExport.md`
- `J2KBackendContract.md`
- `FrameSurface.md`
- `WatermarkPayload.md`
- `WatermarkState.md`
- `WatermarkEvidence.md`
- `DetectorReport.md`
- `ChannelRouteMap.md`
- `AES3DeviceProfile.md`
- `SubtitleCue.md`
- `TimedTextDocument.md`
- `PlaybackSession.md`
- `FaultMatrix.md`
- `OperatorAlert.md`
- `ScheduleEntry.md`
- `ShowControlCommand.md`

# R4 — Hardwarevalidierung und SD-Speichertest

**Datum:** 2026-10-28 (Europe/Vienna) · **Testgerät:** Daniels Erica Synths LXR-02 Desktop · **Stand:** erster erfolgreicher eigener Hardware-/Speichertest.

> [!CAUTION]
> **Experimentelle Firmware — keine allgemeine Sicherheitsfreigabe.** R4 wurde bislang nur auf **einem** LXR-02 und nicht über einen langen Zeitraum oder auf unterschiedlichen Geräten getestet. Der bestandene Kurztest ist **kein Nachweis**, dass Fehler, Datenverlust, Ausfälle oder Schäden am Gerät, an angeschlossener Audiotechnik oder am Gehör ausgeschlossen sind. Risiken wurden nicht vollständig untersucht. Wer die Firmware eigenverantwortlich testet, sollte vorhandene Projekte extern sichern, die offizielle Herstellerfirmware als Rückweg aufbewahren und Audio zunächst mit niedrigem Pegel prüfen. Besonders das noch **ungeklärte laute FX-Bus-`DRV/CLP`-Pfeifen** erfordert Vorsicht.

## Reproduzierbare Firmware-Identität

| Gegenstand | Referenz |
|---|---|
| Eigenes R4-Image | `LXR_V2_V1.7.img`, **288.676 Bytes**, SHA-256 `1886a5484aedf7e53ed4aaf21b08c56073a1527a9ec0cc634c089db769281877` |
| R4-Quellbranch | [`ui/kit-load-confirmation`](https://github.com/DocLXR-02/lxr-desktop-app/tree/ui/kit-load-confirmation), Commit `c870f7249d7ee0d65423bc8b7bfdb422dcc74482` |
| Lieferung | [`delivery/lxr-lab-r4-kit-load-2026-10-28`](https://github.com/DocLXR-02/lxr-desktop-app/tree/delivery/lxr-lab-r4-kit-load-2026-10-28), Commit `eea1949f0641e375f8cc6082971242549aeed1d1` |
| R4-Liefer-ZIP | `delivery/LXR_LAB_R4_KIT_LOAD_CONFIRMATION.zip`, SHA-256 `c83290aad78ffa469cd65cec009273c6c29ba029877274d3f59ee44373411a73` laut `delivery/PACKAGE_SHA256SUMS.txt` |
| Offizielles Original | v1.71 `LXRV2_update_v1.71(1).img`, **273.544 Bytes**, SHA-256 `188695ea78cd3da0fb6a3e825732bd7fd365d4491a37126003ec513e163e1576` |

Das interne R4-Dateinamensfragment `V1.7` ist **keine Behauptung**, R4 sei binär identisch zur offiziellen Hersteller-v1.71. Zuvor wurde die offizielle v1.71 erfolgreich geflasht, der LXR startete und beide damaligen Projekte waren auswählbar. Das ist **kein** nach R4 separat erprobter Rollback.

Der historische [R4-Paketbericht](https://github.com/DocLXR-02/lxr-desktop-app/blob/codex-reports/2026-10-28-r4-firmware-package/docs/codex-berichte/2026-10-28_r4-firmware-package.md) entstand **vor** dem Gerätetest und ist deshalb mit „not hardware validated“ gekennzeichnet. Dieser neue datierte Bericht ergänzt ihn, ohne die frühere Aussage rückwirkend zu überschreiben.

## Gerätebeobachtungen und Testergebnisse

| Funktion/Test | Ergebnis und Grenze |
|---|---|
| Flash/Start | **BESTANDEN (GERÄT):** Eigene Firmware installiert, Gerät startet, Klangerzeugung und Projektwahl funktionieren. |
| **Bug 001: Filter-Drive-Morph** | **BESTANDEN (GERÄT):** Ursprünglich hörbares Knacksen/Drive-Rückspringen beim Kit-Morph nicht mehr vorhanden. |
| **Bug 002: Filter Drive nach Patternwechsel** | **NICHT REPRODUZIERBAR (GERÄT):** Test mit AutoSave an und aus; kein eigener unabhängiger Ursachen-/Fixnachweis. |
| Morph-Regelbereich **127** | **BESTANDEN (GERÄT):** Kürzerer Drehweg an echter Hardware angenehmer. |
| `Load Kit` und `Load Morph Kit` | **BESTANDEN (GERÄT):** Jeweils kurzes `success!`-Feedback sichtbar. Keine zusätzliche UI-Veränderung für Pattern/Save/Song verlangt. |
| Save Kit/Pattern/Project | **BESTANDEN IM SMOKETEST (GERÄT):** Mehrfach gespeichert, anschließend mehrfach neu gestartet; Inhalte verfügbar. |
| R4-SD-Archiv | **BESTANDEN (DATEIPRÜFUNG):** `Archive_resetR4.zip` inhaltlich und strukturell plausibel; Details unten. |

## SD-Artefaktprüfung: `Archive_resetR4.zip`

Der Nutzer richtete die zuvor problematische **32-GB-FAT32-microSDHC** für R4 neu ein und erzeugte **neue** Projektordner statt einer Migration alter Projektordner. Verwendet wurde ein anderer SD-Adapter; AutoSave war aus. Das Archiv enthält:

- `PROJ00` (Name `R4`) und `PROJ01` (`X`), je **194** Datendateien: **64 SND** à 255 Bytes, **64 PAT** à 3.663 Bytes mit `PatV03`, **64 SNG** à 72 Bytes, `GLO.CFG` 30 Bytes und `PRJ.NFO` 8 Bytes.
- In beiden Projektordnern **keine 0-Byte-Projektdateien**. Projekte enthalten plausible bearbeitete Kit-/Pattern-Daten.
- `PROJ.IDX` enthält `0` und verweist auf ein vorhandenes `PROJ00`.
- Keine offenen Transaktionsordner. `TMP/TMP.SND` (248 Bytes) und `TMP/TMP.PAT` (3.650 Bytes) sind vorhanden und für sich kein Fehlerbefund.
- ZIP-Datenstruktur ist lesbar; das enthaltene R4-Image entspricht dem oben genannten SHA-256.

**Resultat:** Neuanlage, gewöhnliche Save-/Load-Nutzung und Neustarts funktionierten unter diesen Bedingungen. Der Test beweist keine allgemeine Legacy-Kompatibilität oder Resistenz gegen Stromverlust in jedem Schreibfenster.

## Vorheriger SD-Vorfall (R3) — Befunde, nicht vermutete Ursache

Im früheren Archiv `Archive.zip` wurden folgende konkrete Punkte beobachtet:

1. `PROJ.IDX` = `1`, aber **kein `PROJ01`**, weshalb beim Booten `SD error / Path? Retry save` auftrat.
2. In `PROJ00`/`PROJ14` zusammen **21 0-Byte-Projektdateien**; andere Daten hatten normale Kennungen/Dateigrößen.
3. `.img` auf SD nur **16.384 Bytes** (offensichtlich kein vollständiges eigenes R4-Image; frühere R3-Situation).
4. Die iPhone-Dateien-App zeigte den Karteninhalt zeitweise nicht, während er am iPad sichtbar war; später kehrte die Anzeige am iPhone zurück.

Die R4-Erprobung wechselte **zugleich** Dateisystemzustand (Neuformatierung), Adapter, Projektordner und AutoSave-Zustand. **Es ist daher nicht geklärt**, ob die R3-Probleme durch Firmwarelogik, unterbrochene Kopierprozesse, SD/FAT32, Adapter oder ungültige Altbestände entstanden. Keine Aussage „alte Projekte inkompatibel“ und keine absolute „R4 kann niemals korrumpieren“-Garantie.

## Bewusst offener Testumfang

- Altprojektordner von vor R4 wurden **nicht** separat auf Kompatibilität getestet.
- Keine systematische Stromausfall-in-Write-Phase-Serie am Gerät; Recovery-/Full-Card-/Fremddateischutzprüfungen aus Hosttests nicht als Gerätetests ausgeben.
- Keine umfassende Langzeit-Stresstestkampagne für Audio-DMA, CPU-Spitzen, Stack oder beliebige Projektdaten.
- Original-v1.71 war ein funktionierender *Vor*-R4-Flash; ein Rück-Flash von R4 auf v1.71 ist nicht dokumentiert.

## Abschluss und weitere Arbeit

**Freigegebene Arbeits-Baseline ist R4.** Die Änderungen `LXR-001` und `LXR-003` (historisch Bug 001 und Bug 011) sind anhand des Nutzer-Gerätetests bestätigt; der ehemalige Fall Bug 002 ist derzeit nicht reproduzierbar, ohne getrennten kausalen Fixnachweis. Weitere Änderungen nur auf separaten Branches und mit neuen nachvollziehbaren Code-/Build-/Gerätetests. Das nächste primäre Klangthema ist das anhaltende **FX-Bus `DRV/CLP`-Pfeifen**, siehe [`CHANGELOG.md`](CHANGELOG.md); zunächst GitHub-Community-Lösungen und reproduzierbare Hardwarebedingungen prüfen. Die unabhängige Firmwareanpassung erfolgte unter Einsatz von ChatGPT und Codex; Details zur Rollenverteilung im README des Dokumentationsbranches.

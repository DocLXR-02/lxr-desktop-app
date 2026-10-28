# LXR Firmware Lab — Changelog

**Stand:** 28.10.2026 · **R4:** experimentelle, eigene LXR-02-Firmware · **Umfang:** nur unsere vorgenommenen Änderungen und das unmittelbar nächste Vorhaben. Dies ist **keine** vollständige öffentliche Sammlung aller diskutierten oder vermuteten Bugs.

> [!CAUTION]
> **Experimentelle Firmware — keine allgemeine Sicherheitsfreigabe.** Die R4-Version wurde nur kurzzeitig auf **einem** Gerät getestet. Weder Langzeitbetrieb noch verschiedene LXR-02-Hardwareexemplare noch sämtliche Risiken für Gerät, Audiotechnik und Gehör wurden unabhängig geprüft. Fehler, Datenverlust und Schäden können nicht ausgeschlossen werden. Nutzung auf eigene Verantwortung, Projekte vorher extern sichern, offizielle Firmware als Rückweg behalten und die Abhörlautstärke beim Testen niedrig halten. Siehe [`R4_HARDWARE_VALIDATION.md`](R4_HARDWARE_VALIDATION.md).

## Bestätigter R4-Stand

`LXR-###` ist die **einzige fortlaufende öffentliche Änderungsnummer**. Der separate Typ kennzeichnet, *warum* etwas geändert wurde: `FIX` = Bugfix, `UX` = Verbesserung der Bedienbarkeit, `ENH` = technische Erweiterung/Stabilisierung, `FEAT` = neue Funktion. Diese Typangaben sind keine Behauptung einer unveränderten Herstellerfunktion. Historische interne Fallnummern bleiben als Querverweis erhalten.

| ID | Typ | Änderung | Nachweis |
|---|---|---|---|
| **LXR-001** | **FIX** | **Kit-Morph / Filter Drive:** Der ursprünglich hörbare Knackser/Drive-Rücksprung wurde mit einer Korrektur im NRPN-/CC0-Pfad behoben (interner Fall `BUG001`, [Quellfix](https://github.com/DocLXR-02/lxr-desktop-app/tree/bug001/filter-drive-fix), Commit `7d9fe99ea0b1ddc2f4d8e875d2787614df15a2b0`). | **GERÄT:** auf eigener R3/R4 für den reproduzierten Fall bestätigt. |
| **LXR-002** | **UX** | **Morph-Drehweg:** Wertebereich von 255 auf **127** verkürzt, um schneller zum Ziel-Kit zu gelangen. Bewusste Designentscheidung, **kein Bugfix**. | **GERÄT:** wesentlich angenehmere Bedienung bestätigt. |
| **LXR-003** | **UX** | **Ladefeedback:** `Load Kit` und `Load Morph Kit` zeigen jeweils kurz `success!` (interner Fall `BUG011`). Sonstige Save-/Load-Anzeigen wurden nicht ungefragt neu gestaltet. | **GERÄT:** beide Rückmeldungen bestätigt. |
| **LXR-004** | **ENH** | **Projektspeicherung/Recovery (S4):** Ausgewählte Save-/Recovery-Pfade gehärtet, u. a. Hosttests für volle Karten, fremde Dateien, Erst-Save-Stufe und eindeutige Transaktionszuordnung. | **HOSTTEST** laut Codex; **GERÄT** für normales Kit-/Pattern-/Projekt-Speichern und wiederholte Neustarts. Keine umfassende Power-Loss-Garantie. |

**Begleitbefund ohne eigene Änderungsnummer:** Der interne Fall `BUG002` (Filter Drive nach Patternwechsel teils erst nach Potibewegung klanglich aktualisiert) ist auf eigener Firmware mit AutoSave an/aus **nicht mehr reproduzierbar**. Weil kein unabhängiger kausaler Einzelfix nachgewiesen ist, führen wir ihn **nicht** als abgeschlossene Codekorrektur auf.

## Vorschau — nächster Untersuchungsfall

**FX-Bus `DRV/CLP`:** Bei bestimmten Parameterkombinationen kann ein lautes, anhaltendes Pfeifen auftreten — sogar nach Voice-1-Fader-Mute und bei gestopptem Sequencer. `D/W` ändert den Pegel; ein Absenken bestimmter Effektparameter unter einen Schwellenwert beendet es. **Symptom am Gerät beobachtet, Ursache noch offen, hier noch keine Korrektur.**

Vor der Untersuchung Community-Issues/PRs/Fixes auf GitHub prüfen und den kleinsten reproduzierbaren Test aufbauen. Wegen des ungewöhnlich hohen Pegels bei Audiotests vorsichtig sein. Eine neue `LXR-`-Nummer wird erst bei einer tatsächlich umgesetzten Änderung vergeben.

## Verweise

- **R4-Quellstand:** [`ui/kit-load-confirmation`](https://github.com/DocLXR-02/lxr-desktop-app/tree/ui/kit-load-confirmation), Commit `c870f7249d7ee0d65423bc8b7bfdb422dcc74482`.
- **R4-Lieferstand:** [`delivery/lxr-lab-r4-kit-load-2026-10-28`](https://github.com/DocLXR-02/lxr-desktop-app/tree/delivery/lxr-lab-r4-kit-load-2026-10-28), Commit `eea1949f0641e375f8cc6082971242549aeed1d1`.
- **Firmware-Image:** `LXR_V2_V1.7.img`, **288.676 Byte**, SHA-256 `1886a5484aedf7e53ed4aaf21b08c56073a1527a9ec0cc634c089db769281877` (**kein** Beweis für Identität mit Hersteller-v1.71).
- **Gerätetest und Grenzen:** [`R4_HARDWARE_VALIDATION.md`](R4_HARDWARE_VALIDATION.md).
- **Transparenz:** Die Anpassungen dieses experimentellen Forks wurden mit Unterstützung von ChatGPT und Codex erarbeitet; Hardwaretests und musikalische Entscheidungen verantwortet der Maintainer. Siehe `README.md` auf diesem Dokumentationsbranch.

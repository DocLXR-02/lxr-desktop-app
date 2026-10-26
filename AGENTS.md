# LXR Firmware Lab — Codex instructions

These rules apply to Codex tasks in `DocLXR-02/lxr-desktop-app`. Treat this repository as STM32F765 real-time audio firmware.

## Project safety

- Never modify, commit to, force-push, or merge into `main`. For authorized changes use `lab-setup` or a dedicated branch based on `lab-setup`; keep the exact base commit in the report.
- Do not flash firmware or direct hardware tests without Daniel's explicit approval, a validated backup, and a recovery plan. A successful build does not prove correct audio behavior.
- Diagnose before changing code. Do not broaden a bug fix into unrelated DSP, UI, timing, or preset modifications. Distinguish hardware observations, code evidence, hypotheses, and tests.
- Respect explicit task restrictions. In particular, a task that expressly forbids *all* file changes, commits, or pushes is not authorization to make a report commit.

## Mandatory handoff to ChatGPT

At the end of **every** task, prepare a self-contained Markdown handoff report sufficient for another assistant to continue without access to this Codex conversation. Include, as applicable:

1. Task name/ID, date, repository, branch, base/head commit, and precise objective and scope.
2. Observations, reproduction steps, source paths and line references, decisive code excerpts, confirmed findings versus hypotheses, and the reasoning linking them.
3. Changes made (or a clear statement that none were made), files affected, meaningful diff summary, and decisions/tradeoffs.
4. Commands and tests actually run, their exact results, firmware version/build artifact metadata, errors and limitations; mark proposed or unrun tests explicitly.
5. Remaining risks, open questions, recommended next action, and whether anything was pushed or published.

Store the report at `docs/codex-berichte/YYYY-MM-DD_<short-task-description>.md`. Use a unique descriptive slug; if a filename exists, choose a new name rather than overwriting it. Keep the report Markdown-only and do not include passwords, tokens, API keys, personal data, or other secrets.

**Publishing:** If the task permits documentation writes and Git commits/pushes, commit and push the report to `lab-setup` or a **report-only branch based on `lab-setup`**. When source-code changes have not yet been approved for push, do not accidentally publish them with the report: use a separate documentation-only branch. Do not force-push or merge branches. Do not make unrelated project changes. For an expressly read-only/no-commit task, still produce the full report in the final response, clearly marking it **not uploaded**, and ask for permission before publishing.

Verify the remote branch and exact file URL after pushing (not merely a local file or an attempted push). End your response with the verified GitHub report URL and branch. If GitHub write access or verification fails, say so plainly, explain where the report remains available, and provide the full report text as a fallback. Never claim that a report was saved without verification.

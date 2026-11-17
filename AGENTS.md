# LXR Firmware Lab — Codex instructions

These rules apply to authorized contributors and Codex tasks in `DocLXR-02/lxr-desktop-app`. Treat this repository as STM32F765 real-time audio firmware. The current canonical governance is on `docs/lab-project-state` in `docs/WORKFLOW.md`, `docs/PROJECT_STATUS.md`, and `docs/EVIDENCE_INDEX.md`; read those documents explicitly at task start because historical source commits do not inherit later documentation. A historical `AGENTS.md` remains historical evidence, not an instruction to rewrite accepted source history.

## Project safety

- Never modify, commit to, force-push, or merge into `main`. For authorized firmware, test, or CI changes, create a separate numbered release branch from the **exact owner-approved source commit SHA** identified in the task. `lab-setup` is a historical baseline, not a mandatory parent for new releases. Never use a documentation branch as a firmware source. Record and independently verify the source base and resulting head SHAs.
- Do not flash firmware or direct hardware tests without Daniel's explicit approval, a validated backup, and a recovery plan. A successful build does not prove correct audio behavior.
- Diagnose before changing code. Do not broaden a bug fix into unrelated DSP, UI, timing, or preset modifications. Distinguish hardware observations, code evidence, hypotheses, and tests.
- Respect explicit task restrictions. In particular, a task that expressly forbids *all* file changes, commits, or pushes is not authorization to make a report commit.

## Mandatory handoff to ChatGPT

**Execution selection:** CHAT/GitHub/CI is preferred for bounded tasks when tools and source-SHA instructions permit. Codex is conditional on an owner-approved efficiency gate for work that needs extensive multi-file editing, iterative debugging or a complex local environment. Every code edit, build, hardware test, flash, release and sound change needs explicit owner approval. Preserve `main`, R4, R11, R18, R20 and accepted R22. Never merge, force-push, publish a release or flash without separate authorization. A documentation-only instruction change does not create or validate a new firmware build.

At the end of **every** task, prepare a self-contained Markdown handoff report sufficient for another assistant to continue without access to this Codex conversation. Include, as applicable:

1. Task name/ID, date, repository, branch, base/head commit, and precise objective and scope.
2. Observations, reproduction steps, source paths and line references, decisive code excerpts, confirmed findings versus hypotheses, and the reasoning linking them.
3. Changes made (or a clear statement that none were made), files affected, meaningful diff summary, and decisions/tradeoffs.
4. Commands and tests actually run, their exact results, firmware version/build artifact metadata, errors and limitations; mark proposed or unrun tests explicitly.
5. Remaining risks, open questions, recommended next action, and whether anything was pushed or published.

Store the report at `docs/codex-berichte/YYYY-MM-DD_<short-task-description>.md`. Use a unique descriptive slug; if a filename exists, choose a new name rather than overwriting it. Keep the report Markdown-only and do not include passwords, tokens, API keys, personal data, or other secrets.

**Publishing:** If the task permits documentation writes and Git commits/pushes, commit and push the report to a **separate documentation/report branch** based on the explicitly approved documentation base SHA. Do not publish a report by modifying `lab-setup`, a stable firmware source branch, or a firmware candidate unless separately authorized. When source-code changes have not yet been approved for push, do not accidentally publish them with the report: use a separate documentation-only branch. Do not force-push or merge branches. Do not make unrelated project changes. For an expressly read-only/no-commit task, still produce the full report in the final response, clearly marking it **not uploaded**, and ask for permission before publishing.

Verify the remote branch and exact file URL after pushing (not merely a local file or an attempted push). End your response with the verified GitHub report URL and branch. If GitHub write access or verification fails, say so plainly, explain where the report remains available, and provide the full report text as a fallback. Never claim that a report was saved without verification.

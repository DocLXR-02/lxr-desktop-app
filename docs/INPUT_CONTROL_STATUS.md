# DATA input investigation — current status

Snapshot: 2026-11-01. This supplements the CLP/resource handoff, without beginning another audit.

The owner reports intermittent missed DATA presses and skipped rotary states. The two symptoms remain separate. Audible clicks and tactile detents do not establish clean electrical contacts; hardware failure is neither established nor excluded.

Prior research on 2026-10-31 inspected the upstream/R4/corrected-R10 input files. It identified a nominal10ms sampling/four-stable-sample switch filter (roughly30–40ms clean-edge acceptance with the assumed1ms tick), followed by a level-only handoff to the foreground menu. A complete accepted press/release during a long foreground gap can be missed by that interface. This is a source-level mechanism, not proof that it caused the owner's particular failures.

The inspected rotary decoder returns only -1/0/+1, has no acceleration multiplier, and bounds net pending movement. Sampling, backlog, reversals and menu translation need targeted replay before assigning a cause. The DATA switch is not the separate panel shift-register button path, and the four analog pots have another path.

The kit-load success notice inspected at R4/R10 lasts750ms and uses a rendering timeout, not a750ms blocking delay. Earlier approximate one-second descriptions should not be interpreted as an input lock. A queued/latching press fix would need ownership and expiry rules so a stale confirmation cannot act on a later Save/Clear page.

The separate encoder Codex audit is **NOT STARTED** in the available evidence. Its previously prepared task used R10 and an earlier blocked-R11 status. That context is now superseded: current firmware is the identified R11, whose final report exists. The refreshed private handoff retains R10/R4 as historical comparisons and requires a current-state check. Do not start that task automatically or repeat the unrelated resource audit.

No input firmware fix, decoder test, diagnostic image, build or device trace was produced by this synchronization. Source findings remain from the earlier inspected versions, not a claimed new R11 input audit. See [PROJECT_STATUS.md](PROJECT_STATUS.md) and [WORKFLOW.md](WORKFLOW.md).

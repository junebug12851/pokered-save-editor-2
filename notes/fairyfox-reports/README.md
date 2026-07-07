# fairyfox-reports/ {#pse_fairyfox_reports_about}

Process reports for this node — one file per run of a **fairyfox system procedure**
(setup, onboarding, adopting updates, a check-for-updates pass). Each report is a
full, honest account of how the run went: what was done, what went well, what was
rough, and concrete suggestions to improve the procedure.

These are this repo's **own** record (committed here, unlike the git-ignored
`assets/references/` clones). The hub reads them on request through its read-only
inbound clone and uses the feedback to improve the shared standards — the node never
pushes anything to the hub.

- One file per run: `YYYY-MM-DD-<procedure>.md` (e.g. `2026-06-26-adopting-updates.md`;
  suffix `-2` if two runs of the same procedure land in one day).
- Start from the template: `assets/references/fairyfox.io/hub/templates/fairyfox-report.md`.
- Full standard: `hub/standards/process-reports.md` (the model is summarized in
  `notes/reference/cross-project-sync.md` → "Process reports").

Write reports in the notes voice — direct, matter-of-fact, honest over flattering. A
report that hides the rough parts defeats its only purpose.

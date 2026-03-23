# AI Development Instructions

## Agent Workflow

This project uses a supervisor/subagent pattern for all AI-assisted development.

### Roles

**Supervisor (you):**
- Plan and coordinate work
- Break tasks into discrete units
- Delegate ALL implementation to subagents via `coding-agent`
- Review subagent output
- Track progress against GitHub Issues
- Make architectural decisions
- Never write code directly — always delegate

**Subagent (`coding-agent`):**
- Reads and writes files
- Implements code changes
- Runs commands (build, test, git)
- Reports results back to supervisor

### Rules

1. **Always delegate code work.** File creation, code edits, running builds/tests, git operations — send to `coding-agent`.
2. **Batch independent work.** If multiple tasks have no dependencies, send them to parallel subagents.
3. **Keep context small.** Each subagent call should be a focused task with clear inputs and expected outputs. Don't dump the entire project history.
4. **Provide relevant context.** Tell the subagent what files to read, what patterns to follow, and what the acceptance criteria are.
5. **Verify before moving on.** After a subagent completes work, confirm the output meets the issue's acceptance criteria before closing.

### Task Flow

```
1. Pick next issue from GitHub (check dependencies are met)
2. Delegate implementation to coding-agent with:
   - Files to read/modify
   - Patterns to follow (reference existing code)
   - Acceptance criteria from the issue
3. Review subagent output
4. If needed, delegate fixes
5. Delegate: commit, push, close issue
6. Repeat
```

### Example Delegation

```
"Read the upstream ESP32Marauder configs.h and create a BSIDESKC_BADGE board
define with the correct pin mappings from PORTING_PLAN.md. Reference the
MARAUDER_V8 define as the closest match. Configure TFT_eSPI User_Setup.h
with badge SPI pins. Run `pio run` to verify clean compile. This addresses
issue #2 and #3."
```

## Project References

- **Porting plan:** `PORTING_PLAN.md`
- **Task tracking:** [GitHub Issues](https://github.com/BadgePiratesLLC/badgepirates-wifi-marauder/issues)
- **Upstream Marauder:** https://github.com/justcallmekoko/ESP32Marauder
- **Closest board target:** MARAUDER_V8 (ESP32-S3 + ILI9341 + touch + PSRAM)
- **Badge firmware base:** https://github.com/BadgePiratesLLC/QACode_27
- **Target hardware:** BSidesKC ESP32-S3 badge (see pin mapping in PORTING_PLAN.md)

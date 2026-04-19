# PUBLIC STACK

`OpenClaw Zombie Director` is **experimental**.

This release intentionally has one primary public stack.

## Canonical Public Stack

- `OpenClaw Zombie Director`
- `OpenClaw Zombie Director Dead Everon`
- `Dead Everon`
- `Bacon Zombies`

## Why this matters

This project can be extended to other maps and zombie packs, but the current public release should be understood as:

- one validated experimental stack
- one documented install path
- one documented OpenClaw opt-in path

That keeps the release simpler for server admins and reduces support confusion.

## Recommended admin rollout

1. Install the canonical stack.
2. Start the server without OpenClaw.
3. Verify the scenario and dependencies load correctly.
4. Add OpenClaw later by uploading:
   `<server profile>/openclaw-zombie-director/rest-bridge.override.json`

# SUPPORTED

`OpenClaw Zombie Director` is **experimental**.

This file defines the intended support boundary for the current public release.

## Supported

- local hosted servers
- self-managed dedicated servers
- cloud/dedicated servers
- hosted panels that allow normal mod/scenario install plus profile/config file upload
- running the mod with no OpenClaw at all
- running the mod with OpenClaw through the REST bridge override file
- running the mod with another external AI that uses the same REST bridge contract

## Supported Public Stack

The current public release is centered on this experimental stack:

- Dead Everon
- Bacon Zombies
- OpenClaw Zombie Director
- OpenClaw Zombie Director Dead Everon overlay

Treat that as the canonical supported stack for this release.

## Not Supported

- OpenClaw desktop vision or UI clicking on true headless servers
- assuming every zombie pack or map pack will work the same way
- one-click automatic setup on every hosting panel
- guaranteed clean logs from every third-party dependency
- polished production stability expectations

## Practical Meaning

If you want the smoothest install path right now:

- use the published experimental stack above
- run without OpenClaw first
- then add OpenClaw through the profile override file if you want AI direction

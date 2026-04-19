# PUBLISH CHECKLIST

`OpenClaw Zombie Director` is **experimental**.

This checklist is the publish packet for the current release candidate.

## Recommended Publish Shape

- Channel: `beta`
- Recommended tag: `v2026.4.19-beta.1`
- Recommended release title: `OpenClaw Zombie Director (Experimental) 2026.4.19-beta.1`
- Recommended workshop title: `OpenClaw Zombie Director (Experimental)`

## Why beta is the right choice

- the project is still experimental
- the hosted-panel flow is usable but still evolving
- third-party dependency noise may still exist outside the director-owned path
- the public stack is intentionally narrow and should not be presented as a polished general release

## Publish Inputs

Use these files directly:

- [START HERE.md](X:/OpenClaw/workspace/projects/reforger-zombie-director/deploy/public-release/START%20HERE.md:1)
- [SUPPORTED.md](X:/OpenClaw/workspace/projects/reforger-zombie-director/deploy/public-release/SUPPORTED.md:1)
- [PUBLIC STACK.md](X:/OpenClaw/workspace/projects/reforger-zombie-director/deploy/public-release/PUBLIC%20STACK.md:1)
- [RELEASE CHECKLIST.md](X:/OpenClaw/workspace/projects/reforger-zombie-director/deploy/public-release/RELEASE%20CHECKLIST.md:1)
- [RELEASE-VALIDATION.md](X:/OpenClaw/workspace/projects/reforger-zombie-director/deploy/public-release/RELEASE-VALIDATION.md:1)
- [workshop-release-description.md](X:/OpenClaw/workspace/projects/reforger-zombie-director/deploy/public-release/workshop-release-description.md:1)
- [PUBLISH MANIFEST.json](X:/OpenClaw/workspace/projects/reforger-zombie-director/deploy/public-release/PUBLISH%20MANIFEST.json:1)

## Assets To Publish

- [openclaw-zombie-director-public-release.zip](X:/OpenClaw/workspace/projects/reforger-zombie-director/deploy/public-release/dist/openclaw-zombie-director-public-release.zip)
- [openclaw-zombie-director-cloud-windows-dedicated-remote-openclaw.zip](X:/OpenClaw/workspace/projects/reforger-zombie-director/deploy/cloud/dist/openclaw-zombie-director-cloud-windows-dedicated-remote-openclaw.zip)
- [openclaw-zombie-director-cloud-linux-dedicated-remote-openclaw.zip](X:/OpenClaw/workspace/projects/reforger-zombie-director/deploy/cloud/dist/openclaw-zombie-director-cloud-linux-dedicated-remote-openclaw.zip)
- [openclaw-zombie-director-workshop-cover.png](X:/OpenClaw/workspace/projects/reforger-zombie-director/deploy/public-release/assets/openclaw-zombie-director-workshop-cover.png)

## Credential-Gated Steps Remaining

1. Approve the exact publish tag/version.
2. Create the Git tag from the intended commit.
3. Create the GitHub release or release page entry.
4. Paste the copy from `workshop-release-description.md`.
5. Upload the release zip assets.
6. Publish or update the workshop item(s).

## Recommended Publish Order

1. Publish the GitHub/release-page assets first.
2. Confirm the bundle downloads are correct.
3. Publish the workshop item copy with the same experimental wording.
4. Keep the workshop item and release page aligned on:
   - title
   - experimental status
   - canonical public stack
   - without/OpenClaw install split

## Do Not Claim

Do not claim:

- polished production stability
- one-click setup on every host
- universal compatibility with every zombie pack/map pack
- desktop-vision support on headless servers

## Ready/Not Ready

- `Ready for experimental beta-style publish`: yes
- `Ready for polished stable release`: no

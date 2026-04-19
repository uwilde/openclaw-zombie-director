# OpenClaw Zombie Director Public Release Pack

This folder assembles the user-facing distribution artifacts for `OpenClaw Zombie Director`.

`OpenClaw Zombie Director` is **experimental**.

## Included

- quick-start docs for server operators
- quick-start docs for hosted panels
- a `START HERE` doc for end users
- a `SUPPORTED` boundary doc
- a canonical `PUBLIC STACK` doc
- a release checklist
- a release-validation report
- a publish manifest
- a publish checklist
- copy-ready workshop/release description text
- Windows control-host bundle
- Linux dedicated-server bundle

## Build

```powershell
powershell -ExecutionPolicy Bypass -File ".\package-public-release.ps1"
```

The final zip is written to:

- `deploy/public-release/dist/openclaw-zombie-director-public-release.zip`

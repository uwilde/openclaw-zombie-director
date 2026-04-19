# RELEASE CHECKLIST

`OpenClaw Zombie Director` is **experimental**.

Use this checklist before calling a build release-ready.

## Scenario Defaults

- Published scenario uses `m_sConcreteProfileId = dead_everon_bacon_concrete`
- Published scenario uses `m_RestBridge.m_bEnabled = false`
- Published scenario uses `m_RestBridge.m_sProfileOverridePath = openclaw-zombie-director/rest-bridge.override.json`
- Published scenario uses `m_bRunSmokeBootstrap = false`
- Published scenario uses `m_bRelocateFirstPlayer = false`

## Public Docs

- `START HERE.md` is present
- `SUPPORTED.md` is present
- `PUBLIC STACK.md` is present
- `docs/server-operator-quick-start.md` is present
- `docs/hosting-panel-quick-start.md` is present
- `workshop-release-description.md` is present
- every public-facing doc clearly says the mod is experimental

## Bundles

- Windows control-host bundle zip exists
- Linux dedicated-server bundle zip exists
- each bundle includes `install-control-host.ps1`
- each bundle includes `server/rest-bridge.profile-override.example.json`
- generated secret-bearing files are excluded from shipped zips

## Runtime Path

- no-OpenClaw server path is documented
- OpenClaw control-host install path is documented
- profile override file path is documented as:
  `<server profile>/openclaw-zombie-director/rest-bridge.override.json`
- OpenClaw remains optional

## Validation

- bundle configurator generates a BOM-safe override file
- profile override loads successfully at runtime
- the canonical public world opens
- the scenario starts without smoke-only publish blockers

## Release Copy

- Workshop title includes `Experimental`
- workshop short description includes `Experimental`
- release description includes `Experimental`
- admin install steps match the current bundles

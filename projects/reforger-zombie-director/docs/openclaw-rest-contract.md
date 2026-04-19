# OpenClaw REST Contract

The director uses a very small REST surface so external systems can stay high-level and stateless.

## Base URL

Default:

```text
http://127.0.0.1:18890/
```

The addon appends route strings configured on `OCD_ZombieDirectorRestBridgeConfig`.

Remote dedicated-server example:

```text
https://director.example.com/
```

## Health

```http
GET /reforger/zombie-director/health
```

Response:

```json
{
  "ok": true,
  "service": "openclaw-zombie-director-bridge"
}
```

## Pull hints

```http
GET /reforger/zombie-director/hints?afterSeq=12
```

Response:

```json
{
  "serverTime": "2026-04-17T20:14:21Z",
  "hints": [
    {
      "seq": 13,
      "type": "reinforce_zone",
      "targetZoneId": "zone_farmhouse",
      "templateId": "z_rushers",
      "requestedBudget": 10,
      "weight": 2.0,
      "ttlSeconds": 45,
      "reason": "players are overrunning the outer trench",
      "correlationId": "gm-2026-04-17-001",
      "anchor": "0 0 0"
    }
  ]
}
```

Supported `type` values in this scaffold:

- `reinforce_zone`
- `stage_ambush`
- `sweep_corridor`
- `force_spike`
- `quiet_down`

Unknown values are ignored safely.

## Push hints

```http
POST /reforger/zombie-director/hints
Content-Type: application/json
```

Request body:

```json
{
  "hints": [
    {
      "type": "stage_ambush",
      "targetZoneId": "zone_wheatfield",
      "templateId": "z_walkers",
      "requestedBudget": 8,
      "weight": 1.5,
      "ttlSeconds": 30,
      "reason": "players entering open terrain",
      "correlationId": "analysis-pass-42",
      "anchor": "0 0 0"
    }
  ]
}
```

Response:

```json
{
  "ok": true,
  "lastSeq": 14
}
```

## Push snapshot from Reforger

```http
POST /reforger/zombie-director/snapshot
Content-Type: application/json
```

Body shape:

```json
{
  "budgetCurrent": 31.5,
  "budgetCap": 60,
  "activeWaveCount": 7,
  "players": [
    {
      "playerId": 1,
      "name": "PlayerOne",
      "position": "1000 0 2000",
      "moveDir": "1 0 0"
    }
  ],
  "zones": [
    {
      "zoneId": "zone_farmhouse",
      "score": 4.7,
      "cooldownRemaining": 18,
      "activeBudget": 12,
      "reason": "base pressure + reinforce hint"
    }
  ]
}
```

## Notes

- The bridge is intentionally local-first and does not require the Game Master UI.
- A scenario can ignore OpenClaw entirely and still use the same director logic via the local script API.
- If you want remote control across machines or from a cloud server, move the bridge off loopback and secure it before exposing it anywhere else.
- Headless servers should use this REST path, not desktop-vision or GUI-click automation.

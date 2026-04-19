[ComponentEditorProps(category: "OpenClaw/Zombie Director", description: "Server-authoritative zombie director with optional OpenClaw REST bridge", color: "220 70 70 255", visible: false)]
class OCD_ZombieDirectorGameModeComponentClass : SCR_BaseGameModeComponentClass
{
}

class OCD_ZombieDirectorGameModeComponent : SCR_BaseGameModeComponent
{
	protected static const ResourceName WAYPOINT_PREFAB_MOVE = "{750A8D1695BD6998}Prefabs/AI/Waypoints/AIWaypoint_Move.et";
	protected static const ResourceName WAYPOINT_PREFAB_CYCLE = "{35BD6541CBB8AC08}Prefabs/AI/Waypoints/AIWaypoint_Cycle.et";
	protected static const float NO_PLAYER_WAVE_GRACE_SECONDS = 90.0;
	protected static const int TERRAIN_POINT_ATTEMPTS = 8;
	protected static const float TERRAIN_POINT_MIN_RADIUS = 6.0;
	protected static const float TERRAIN_POINT_MAX_RADIUS = 24.0;
	protected static const int NAVMESH_FOOTPRINT_SAMPLE_COUNT = 8;
	protected static const float INDIVIDUAL_SPAWN_NAVMESH_CLEARANCE = 5.0;
	protected static const float GROUP_SPAWN_NAVMESH_CLEARANCE = 14.0;
	protected static const float ROUTE_POINT_NAVMESH_CLEARANCE = 8.0;
	protected static const float MAX_ROUTE_POINT_RESOLVE_DRIFT = 2.0;

	protected static OCD_ZombieDirectorGameModeComponent s_Instance;

	[Attribute(defvalue: "1", desc: "Enable the director")]
	protected bool m_bEnabled;

	[Attribute(defvalue: "1.0", desc: "Director think period in seconds")]
	protected float m_fThinkPeriodSeconds;

	[Attribute(defvalue: "2.5", desc: "Snapshot publish period in seconds")]
	protected float m_fSnapshotPeriodSeconds;

	[Attribute(defvalue: "60", desc: "Global budget cap")]
	protected int m_iGlobalBudgetCap;

	[Attribute(defvalue: "2.5", desc: "Budget regenerated per second")]
	protected float m_fBudgetRegenPerSecond;

	[Attribute(defvalue: "12", desc: "Maximum active wave records")]
	protected int m_iMaxActiveWaves;

	[Attribute(defvalue: "45", desc: "How much quiet time before ambient pressure ramps")]
	protected float m_fQuietSecondsBeforePressure;

	[Attribute(defvalue: "28", desc: "Minimum seconds between dynamic zone events in the same zone")]
	protected float m_fZoneEventCooldownSeconds;

	[Attribute(defvalue: "10", desc: "Suppress duplicate hints with the same signature for this many seconds")]
	protected float m_fHintDedupSeconds;

	[Attribute(uiwidget: UIWidgets.Object, desc: "External bridge settings")]
	protected ref OCD_ZombieDirectorRestBridgeConfig m_RestBridge = new OCD_ZombieDirectorRestBridgeConfig();

	[Attribute(uiwidget: UIWidgets.Object, desc: "Zombie template list")]
	protected ref array<ref OCD_ZombieUnitTemplateConfig> m_aTemplates = {};

	[Attribute(defvalue: "", desc: "Optional built-in runtime preset. Supported: dead_everon_bacon_concrete")]
	protected string m_sConcreteProfileId;

	[Attribute(defvalue: "0", desc: "Queue a small startup smoke pass so editor validation can exercise spawn, hint, and sweep without a live player")]
	protected bool m_bRunSmokeBootstrap;

	[Attribute(defvalue: "6.0", desc: "Delay before the startup smoke pass begins")]
	protected float m_fSmokeBootstrapInitialDelaySeconds;

	[Attribute(defvalue: "9.0", desc: "Seconds between smoke pass stages")]
	protected float m_fSmokeBootstrapStageGapSeconds;

	[Attribute(defvalue: "0", desc: "Relocate the first player to a deterministic smoke anchor after spawn")]
	protected bool m_bRelocateFirstPlayer;

	[Attribute(defvalue: "0 0 0", desc: "Preferred smoke player start (world coordinates)")]
	protected vector m_vPreferredPlayerStart;

	[Attribute(defvalue: "80", desc: "Search radius used when relocating the first player to terrain")]
	protected float m_fPlayerRelocationRadius;

	[Attribute(defvalue: "24.0", desc: "Hold hint-driven spawns until the world has had this many seconds to settle after game-mode start")]
	protected float m_fSpawnWarmupSeconds;

	protected ref array<ref OCD_ZombieDirectorZoneDefinition> m_aZones = {};
	protected ref map<string, ref OCD_ZombieDirectorZoneRuntimeState> m_mZoneState = new map<string, ref OCD_ZombieDirectorZoneRuntimeState>();
	protected ref map<int, ref OCD_ZombieDirectorPlayerTrack> m_mPlayers = new map<int, ref OCD_ZombieDirectorPlayerTrack>();
	protected ref array<ref OCD_ZombieDirectorActiveWave> m_aActiveWaves = {};
	protected ref array<ref OCD_ZombieDirectorHintJson> m_aLocalHints = {};
	protected ref OCD_ZombieDirectorRestBridge m_Bridge;
	protected ref OCD_ZombieDirectorDeadEveronConcreteRuntime m_ConcreteRuntime;
	protected float m_fBudgetCurrent;
	protected float m_fBridgePollAccumulator;
	protected float m_fSnapshotAccumulator;
	protected float m_fQuietAccumulator;
	protected float m_fRuntimeSeconds;
	protected bool m_bPlayerRelocationCompleted;

	static OCD_ZombieDirectorGameModeComponent GetInstance()
	{
		return s_Instance;
	}

	override void OnGameModeStart()
	{
		if (!m_bEnabled || !GetGameMode() || !GetGameMode().IsMaster())
			return;

		s_Instance = this;
		m_fBudgetCurrent = m_iGlobalBudgetCap;
		m_fRuntimeSeconds = 0.0;
		m_bPlayerRelocationCompleted = !m_bRelocateFirstPlayer;
		ApplyProfileBridgeOverrideIfPresent();
		m_Bridge = new OCD_ZombieDirectorRestBridge(m_RestBridge);
		if (m_sConcreteProfileId == "dead_everon_bacon_concrete")
			m_ConcreteRuntime = new OCD_ZombieDirectorDeadEveronConcreteRuntime();
		EnsureConcreteTemplates();

		int thinkMs = 1000;
		if (m_fThinkPeriodSeconds > 0)
			thinkMs = m_fThinkPeriodSeconds * 1000;

		int snapshotMs = 2500;
		if (m_fSnapshotPeriodSeconds > 0)
			snapshotMs = m_fSnapshotPeriodSeconds * 1000;

		GetGame().GetCallqueue().CallLater(DirectorTick, thinkMs, true);
		GetGame().GetCallqueue().CallLater(SnapshotTick, snapshotMs, true);

		if (m_bRunSmokeBootstrap)
			ScheduleSmokeBootstrap();
	}

	override void OnGameModeEnd(SCR_GameModeEndData data)
	{
		GetGame().GetCallqueue().Remove(DirectorTick);
		GetGame().GetCallqueue().Remove(SnapshotTick);
		GetGame().GetCallqueue().Remove(SmokeBootstrapStageOne);
		GetGame().GetCallqueue().Remove(SmokeBootstrapStageTwo);
		if (m_ConcreteRuntime)
			m_ConcreteRuntime.Shutdown();

		if (s_Instance == this)
			s_Instance = null;
	}

	void EnqueueLocalHint(notnull OCD_ZombieDirectorHintJson hint)
	{
		m_aLocalHints.Insert(hint);
	}

	protected void DirectorTick()
	{
		if (!m_bEnabled || !GetGameMode() || !GetGameMode().IsMaster())
			return;

		m_fRuntimeSeconds += m_fThinkPeriodSeconds;

		RefreshZones();
		RefreshPlayers();
		UpdateZoneSignals();
		UpdateBudget();
		UpdateZoneTimers();
		UpdateActiveWaves();
		UpdateConcreteRuntime();
		PollBridgeIfDue();

		if (!IsSpawnWarmupComplete())
			return;

		ProcessIncomingHints();
		TriggerZoneEvents();
		TrySpawnBestWave();
	}

	protected bool IsSpawnWarmupComplete()
	{
		if (m_fSpawnWarmupSeconds <= 0.0)
			return true;

		return m_fRuntimeSeconds >= m_fSpawnWarmupSeconds;
	}

	protected void SnapshotTick()
	{
		if (!m_bEnabled || !GetGameMode() || !GetGameMode().IsMaster() || !m_Bridge)
			return;

		OCD_ZombieDirectorSnapshotJson snapshot = BuildSnapshot();
		m_Bridge.PublishSnapshot(snapshot);
	}

	protected void RefreshZones()
	{
		m_aZones.Clear();

		array<OCD_ZombieDirectorZoneComponent> componentZones = {};
		OCD_ZombieDirectorZoneComponent.GetRegistered(componentZones);
		foreach (OCD_ZombieDirectorZoneComponent componentZone : componentZones)
		{
			if (!componentZone)
				continue;

			OCD_ZombieDirectorZoneDefinition definition = componentZone.BuildDefinition();
			if (!definition)
				continue;

			m_aZones.Insert(definition);
		}

		if (m_ConcreteRuntime)
		{
			array<ref OCD_ZombieDirectorZoneDefinition> generatedZones = {};
			m_ConcreteRuntime.RefreshZones(generatedZones);
			foreach (OCD_ZombieDirectorZoneDefinition generatedZone : generatedZones)
			{
				if (!generatedZone)
					continue;

				m_aZones.Insert(generatedZone);
			}
		}

		foreach (OCD_ZombieDirectorZoneDefinition zone : m_aZones)
		{
			if (!zone)
				continue;

			GetOrCreateZoneState(zone.GetZoneId());
		}
	}

	protected void UpdateConcreteRuntime()
	{
		if (!m_ConcreteRuntime)
			return;

		m_ConcreteRuntime.UpdateCapturePressure();
	}

	protected OCD_ZombieDirectorZoneRuntimeState GetOrCreateZoneState(string zoneId)
	{
		OCD_ZombieDirectorZoneRuntimeState state = m_mZoneState.Get(zoneId);
		if (state)
			return state;

		state = new OCD_ZombieDirectorZoneRuntimeState();
		state.m_sZoneId = zoneId;
		m_mZoneState.Set(zoneId, state);
		return state;
	}

	protected void RefreshPlayers()
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return;

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);

		bool anyPlayerNearActiveWave = false;

		foreach (int playerId : playerIds)
		{
			IEntity controlled = playerManager.GetPlayerControlledEntity(playerId);
			if (!controlled)
				continue;

			OCD_ZombieDirectorPlayerTrack track = m_mPlayers.Get(playerId);
			if (!track)
			{
				track = new OCD_ZombieDirectorPlayerTrack();
				track.m_iPlayerId = playerId;
				m_mPlayers.Set(playerId, track);
			}

			track.UpdateFrom(controlled, playerManager.GetPlayerName(playerId));
			TryRelocatePlayer(controlled, track);

			foreach (OCD_ZombieDirectorActiveWave wave : m_aActiveWaves)
			{
				if (!wave || !wave.IsAlive())
					continue;

				if (OCD_ZombieDirectorMath.Distance2D(track.m_vPosition, wave.GetOrigin()) < 80)
				{
					anyPlayerNearActiveWave = true;
					break;
				}
			}
		}

		if (anyPlayerNearActiveWave)
			m_fQuietAccumulator = 0;
		else
			m_fQuietAccumulator += m_fThinkPeriodSeconds;
	}

	protected void TryRelocatePlayer(notnull IEntity controlled, notnull OCD_ZombieDirectorPlayerTrack track)
	{
		if (m_bPlayerRelocationCompleted || !m_bRelocateFirstPlayer)
			return;

		vector relocationTarget = m_vPreferredPlayerStart;
		if (relocationTarget.LengthSq() <= 0.001)
			return;

		float searchRadius = 80.0;
		if (m_fPlayerRelocationRadius > 1.0)
			searchRadius = m_fPlayerRelocationRadius;

		vector terrainPosition;
		if (!SCR_WorldTools.FindEmptyTerrainPosition(terrainPosition, relocationTarget, searchRadius))
			terrainPosition = relocationTarget;

		terrainPosition[1] = terrainPosition[1] + 1.2;
		controlled.SetOrigin(terrainPosition);
		track.UpdateFrom(controlled, track.m_sName);
		m_bPlayerRelocationCompleted = true;
		Log("Relocated first player to smoke anchor at " + terrainPosition + ".");
	}

	protected void UpdateBudget()
	{
		m_fBudgetCurrent += m_fBudgetRegenPerSecond * m_fThinkPeriodSeconds;
		if (m_fBudgetCurrent > m_iGlobalBudgetCap)
			m_fBudgetCurrent = m_iGlobalBudgetCap;
	}

	protected void UpdateZoneSignals()
	{
		foreach (OCD_ZombieDirectorZoneDefinition zone : m_aZones)
		{
			if (!zone)
				continue;

			OCD_ZombieDirectorZoneRuntimeState state = GetOrCreateZoneState(zone.GetZoneId());
			bool hasPlayerInside = false;
			bool hasMovingPlayer = false;
			float bestApproach = -1.0;
			float presenceDistance = zone.GetRadius() + (zone.GetMinSpawnDistance() * 0.5);
			float movementDistance = zone.GetRadius() + zone.GetMaxSpawnDistance();
			float priorAbsenceSeconds = state.m_fAbsenceSeconds;

			state.m_bWasPlayerInside = state.m_bPlayerInside;

			foreach (int playerId, OCD_ZombieDirectorPlayerTrack track : m_mPlayers)
			{
				if (!track || !track.m_bHasPosition)
					continue;

				float distance = OCD_ZombieDirectorMath.Distance2D(track.m_vPosition, zone.GetZoneOrigin());
				if (distance <= presenceDistance)
					hasPlayerInside = true;

				if (distance > movementDistance)
					continue;

				if (track.m_vMoveDirection.LengthSq() <= 0.1)
					continue;

				vector towardZone = OCD_ZombieDirectorMath.NormalizeOr(zone.GetZoneOrigin() - track.m_vPosition);
				float approach = vector.Dot(track.m_vMoveDirection, towardZone);
				if (approach > bestApproach)
					bestApproach = approach;

				if (approach > -0.2)
					hasMovingPlayer = true;
			}

			state.m_bPlayerInside = hasPlayerInside;
			state.m_fBestApproach = bestApproach;

			if (hasPlayerInside)
			{
				if (!state.m_bWasPlayerInside)
					state.m_fLastEntryAbsenceSeconds = priorAbsenceSeconds;

				state.m_fPresenceSeconds += m_fThinkPeriodSeconds;
				state.m_fAbsenceSeconds = 0.0;
			}
			else
			{
				state.m_fPresenceSeconds = 0.0;
				state.m_fAbsenceSeconds += m_fThinkPeriodSeconds;
			}

			if (hasMovingPlayer)
			{
				state.m_fMovementSeconds += m_fThinkPeriodSeconds;
			}
			else if (state.m_fMovementSeconds > 0.0)
			{
				state.m_fMovementSeconds -= m_fThinkPeriodSeconds * 0.5;
				if (state.m_fMovementSeconds < 0.0)
					state.m_fMovementSeconds = 0.0;
			}
		}
	}

	protected void UpdateZoneTimers()
	{
		foreach (string zoneId, OCD_ZombieDirectorZoneRuntimeState state : m_mZoneState)
		{
			if (!state)
				continue;

			if (state.m_fCooldownRemaining > 0)
				state.m_fCooldownRemaining -= m_fThinkPeriodSeconds;

			if (state.m_fExternalTtlSeconds > 0)
				state.m_fExternalTtlSeconds -= m_fThinkPeriodSeconds;

			if (state.m_fEventCooldownRemaining > 0)
				state.m_fEventCooldownRemaining -= m_fThinkPeriodSeconds;

			if (state.m_fHintSuppressRemaining > 0)
				state.m_fHintSuppressRemaining -= m_fThinkPeriodSeconds;

			if (state.m_fExternalWeight > 0 && state.m_fExternalTtlSeconds > 0)
			{
				state.m_fExternalWeight -= m_fThinkPeriodSeconds * 0.08;
				if (state.m_fExternalWeight < 0)
					state.m_fExternalWeight = 0;
			}

			if (state.m_fExternalTtlSeconds <= 0)
			{
				state.m_fExternalWeight = 0;
				state.m_sForcedTemplateId = string.Empty;
				state.m_iRequestedBudget = 0;
			}
		}
	}

	protected void UpdateActiveWaves()
	{
		for (int i = m_aActiveWaves.Count() - 1; i >= 0; i--)
		{
			OCD_ZombieDirectorActiveWave wave = m_aActiveWaves[i];
			if (!wave)
			{
				m_aActiveWaves.RemoveOrdered(i);
				continue;
			}

			wave.m_fAgeSeconds += m_fThinkPeriodSeconds;

			if (!wave.IsAlive() || ShouldDespawnWave(wave))
			{
				ReleaseWaveBudget(wave);
				wave.DeleteDynamicWaypointEntities();

				if (wave.m_Entity && !wave.m_Entity.IsDeleted())
					SCR_EntityHelper.DeleteEntityAndChildren(wave.m_Entity);

				m_aActiveWaves.RemoveOrdered(i);
			}
		}
	}

	protected bool ShouldDespawnWave(notnull OCD_ZombieDirectorActiveWave wave)
	{
		OCD_ZombieDirectorZoneDefinition zone = FindZoneById(wave.m_sZoneId);
		if (!zone)
			return true;

		if (wave.m_fAgeSeconds > 900)
			return true;

		bool hasTrackedPlayer = false;

		foreach (int playerId, OCD_ZombieDirectorPlayerTrack track : m_mPlayers)
		{
			if (!track || !track.m_bHasPosition)
				continue;

			hasTrackedPlayer = true;
			if (OCD_ZombieDirectorMath.Distance2D(track.m_vPosition, wave.GetOrigin()) <= zone.GetDespawnDistance())
				return false;
		}

		if (!hasTrackedPlayer)
			return wave.m_fAgeSeconds > NO_PLAYER_WAVE_GRACE_SECONDS;

		return true;
	}

	protected void PollBridgeIfDue()
	{
		if (!m_Bridge || !m_Bridge.IsEnabled())
			return;

		m_fBridgePollAccumulator += m_fThinkPeriodSeconds;
		if (m_fBridgePollAccumulator < m_RestBridge.m_fPollIntervalSeconds)
			return;

		m_fBridgePollAccumulator = 0;
		m_Bridge.PollHints();
	}

	protected void ProcessIncomingHints()
	{
		array<ref OCD_ZombieDirectorHintJson> bridgeHints = {};
		if (m_Bridge)
			m_Bridge.ConsumeHints(bridgeHints);

		foreach (OCD_ZombieDirectorHintJson hintA : bridgeHints)
			ApplyHint(hintA);

		foreach (OCD_ZombieDirectorHintJson hintB : m_aLocalHints)
			ApplyHint(hintB);

		m_aLocalHints.Clear();
	}

	protected void ApplyHint(notnull OCD_ZombieDirectorHintJson hint)
	{
		string zoneId = hint.targetZoneId;
		if (zoneId.IsEmpty())
			return;

		OCD_ZombieDirectorZoneDefinition zone = FindZoneById(zoneId);
		if (!zone || !zone.AllowsExternalHints())
			return;

		OCD_ZombieDirectorZoneRuntimeState state = GetOrCreateZoneState(zoneId);
		string signature = BuildHintSignature(hint);
		if (!signature.IsEmpty() && state.m_sLastHintSignature == signature && state.m_fHintSuppressRemaining > 0)
			return;

		state.m_sLastHintSignature = signature;
		state.m_fHintSuppressRemaining = m_fHintDedupSeconds;
		state.m_fExternalWeight = OCD_ZombieDirectorMath.ClampFloat(state.m_fExternalWeight + hint.weight, 0.0, 4.0);
		if (hint.ttlSeconds > state.m_fExternalTtlSeconds)
			state.m_fExternalTtlSeconds = hint.ttlSeconds;

		if (!hint.templateId.IsEmpty())
			state.m_sForcedTemplateId = hint.templateId;

		if (hint.requestedBudget > 0)
			state.m_iRequestedBudget = hint.requestedBudget;

		state.m_sLastReason = hint.reason;
		Log("Applied hint zone=" + zoneId + " template=" + hint.templateId + " weight=" + hint.weight + " reason=" + hint.reason + ".");
	}

	protected string BuildHintSignature(notnull OCD_ZombieDirectorHintJson hint)
	{
		if (!hint.correlationId.IsEmpty())
			return hint.correlationId;

		return hint.targetZoneId + "|" + hint.type + "|" + hint.templateId;
	}

	protected void TriggerZoneEvents()
	{
		foreach (OCD_ZombieDirectorZoneDefinition zone : m_aZones)
		{
			if (!zone)
				continue;

			OCD_ZombieDirectorZoneRuntimeState state = GetOrCreateZoneState(zone.GetZoneId());
			if (state.m_fEventCooldownRemaining > 0.0)
				continue;

			if (state.m_iActiveBudget >= zone.GetLocalBudgetCap())
				continue;

			// Keep broad base umbrella zones from competing with the tighter encounter lanes.
			if (zone.HasTag("base") && !zone.HasTag("lab") && !zone.HasTag("village") && !zone.HasTag("woods") && zone.GetRadius() > 60.0)
				continue;

			if (HasNearbyActiveWave(zone, zone.GetRadius() + 36.0))
				continue;

			if (TryTriggerLabOrQuarantineEvent(zone, state))
				return;

			if (TryTriggerRoadblockEvent(zone, state))
				return;

			if (TryTriggerVillageEdgeEvent(zone, state))
				return;

			if (TryTriggerForestTrailEvent(zone, state))
				return;
		}
	}

	protected bool TryTriggerLabOrQuarantineEvent(notnull OCD_ZombieDirectorZoneDefinition zone, notnull OCD_ZombieDirectorZoneRuntimeState state)
	{
		if (!state.m_bPlayerInside || state.m_fPresenceSeconds < 4.0)
			return false;

		if (!zone.HasTag("lab") && !zone.HasTag("quarantine"))
			return false;

		string templateId = "dead_everon_quarantine_spike";
		if (zone.HasTag("lab") && state.m_fPresenceSeconds >= 8.0)
			templateId = ResolveEventTemplate(zone, "dead_everon_horde_push", "dead_everon_quarantine_spike", "dead_everon_checkpoint_mix");
		else
			templateId = ResolveEventTemplate(zone, "dead_everon_quarantine_spike", "dead_everon_checkpoint_mix", "dead_everon_horde_push");

		if (templateId.IsEmpty())
			return false;

		string reason = "quarantine corridor breach";
		if (zone.HasTag("lab"))
			reason = "lab corridor breach";

		QueueZoneEventHint(zone, state, "force_spike", "breach", templateId, 10, 1.55, reason, 26.0);
		return true;
	}

	protected bool TryTriggerRoadblockEvent(notnull OCD_ZombieDirectorZoneDefinition zone, notnull OCD_ZombieDirectorZoneRuntimeState state)
	{
		if (!state.m_bPlayerInside || state.m_fMovementSeconds < 2.0 || state.m_fBestApproach <= 0.05)
			return false;

		if (!zone.HasTag("checkpoint") && !zone.HasTag("roadblock"))
			return false;

		string templateId = ResolveEventTemplate(zone, "dead_everon_checkpoint_mix", "dead_everon_tier1_sweep", "dead_everon_quarantine_spike");
		if (templateId.IsEmpty())
			return false;

		QueueZoneEventHint(zone, state, "force_spike", "road_choke", templateId, 10, 1.35, "roadblock choke pressure", 24.0);
		return true;
	}

	protected bool TryTriggerVillageEdgeEvent(notnull OCD_ZombieDirectorZoneDefinition zone, notnull OCD_ZombieDirectorZoneRuntimeState state)
	{
		if (!state.m_bPlayerInside || state.m_bWasPlayerInside || state.m_fLastEntryAbsenceSeconds < 12.0)
			return false;

		if (!zone.HasTag("village") && !zone.HasTag("ruin"))
			return false;

		string templateId = ResolveEventTemplate(zone, "dead_everon_tier1_sweep", "dead_everon_civilian_drifters", "dead_everon_checkpoint_mix");
		if (templateId.IsEmpty())
			return false;

		QueueZoneEventHint(zone, state, "stage_ambush", "village_edge", templateId, 8, 1.2, "village-edge breach", 20.0);
		return true;
	}

	protected bool TryTriggerForestTrailEvent(notnull OCD_ZombieDirectorZoneDefinition zone, notnull OCD_ZombieDirectorZoneRuntimeState state)
	{
		if (!state.m_bPlayerInside || state.m_fMovementSeconds < 3.0)
			return false;

		if (!zone.HasTag("woods"))
			return false;

		string templateId = ResolveEventTemplate(zone, "dead_everon_tier1_sweep", "dead_everon_civilian_drifters");
		if (templateId.IsEmpty())
			return false;

		QueueZoneEventHint(zone, state, "stage_ambush", "forest_trail", templateId, 9, 1.25, "forest-trail pursuit", 22.0);
		return true;
	}

	protected void QueueZoneEventHint(notnull OCD_ZombieDirectorZoneDefinition zone, notnull OCD_ZombieDirectorZoneRuntimeState state, string type, string eventKey, string templateId, int requestedBudget, float weight, string reason, float ttlSeconds)
	{
		string correlationId = zone.GetZoneId() + "|" + eventKey + "|" + templateId;
		OCD_ZombieDirectorAPI.QueueLocalHint(type, zone.GetZoneId(), templateId, requestedBudget, weight, reason, ttlSeconds, zone.GetZoneOrigin(), correlationId);
		state.m_fEventCooldownRemaining = m_fZoneEventCooldownSeconds;
	}

	protected string ResolveEventTemplate(notnull OCD_ZombieDirectorZoneDefinition zone, string primaryTemplateId, string secondaryTemplateId = string.Empty, string tertiaryTemplateId = string.Empty)
	{
		if (!primaryTemplateId.IsEmpty() && zone.AllowsTemplate(primaryTemplateId) && HasTemplate(primaryTemplateId))
			return primaryTemplateId;

		if (!secondaryTemplateId.IsEmpty() && zone.AllowsTemplate(secondaryTemplateId) && HasTemplate(secondaryTemplateId))
			return secondaryTemplateId;

		if (!tertiaryTemplateId.IsEmpty() && zone.AllowsTemplate(tertiaryTemplateId) && HasTemplate(tertiaryTemplateId))
			return tertiaryTemplateId;

		return string.Empty;
	}

	protected bool HasTemplate(string templateId)
	{
		foreach (OCD_ZombieUnitTemplateConfig template : m_aTemplates)
		{
			if (!template || !template.IsValid() || template.m_sId != templateId)
				continue;

			return true;
		}

		return false;
	}

	protected bool HasNearbyActiveWave(notnull OCD_ZombieDirectorZoneDefinition zone, float radius)
	{
		foreach (OCD_ZombieDirectorActiveWave wave : m_aActiveWaves)
		{
			if (!wave || !wave.IsAlive())
				continue;

			if (OCD_ZombieDirectorMath.Distance2D(wave.GetOrigin(), zone.GetZoneOrigin()) <= radius)
				return true;
		}

		return false;
	}

	protected void TrySpawnBestWave()
	{
		if (m_aActiveWaves.Count() >= m_iMaxActiveWaves)
			return;

		float bestScore = 0;
		OCD_ZombieDirectorZoneDefinition bestZone = null;
		OCD_ZombieUnitTemplateConfig bestTemplate = null;
		OCD_ZombieDirectorZoneRuntimeState bestState = null;
		string bestReason;

		foreach (OCD_ZombieDirectorZoneDefinition zone : m_aZones)
		{
			if (!zone)
				continue;

			OCD_ZombieDirectorZoneRuntimeState state = GetOrCreateZoneState(zone.GetZoneId());
			if (state.m_fCooldownRemaining > 0)
				continue;

			if (state.m_iActiveBudget >= zone.GetLocalBudgetCap())
				continue;

			string reason;
			float rawZoneScore = ScoreZone(zone, state, reason);
			state.m_fSmoothedScore = OCD_ZombieDirectorMath.LerpFloat(state.m_fSmoothedScore, rawZoneScore, 0.45);
			state.m_fLastScore = state.m_fSmoothedScore;
			state.m_sLastReason = reason;

			if (state.m_fLastScore <= 0)
				continue;

			OCD_ZombieUnitTemplateConfig template = SelectTemplateForZone(zone, state);
			if (!template)
				continue;

			if (state.m_fLastScore > bestScore)
			{
				bestScore = state.m_fLastScore;
				bestZone = zone;
				bestTemplate = template;
				bestState = state;
				bestReason = reason;
			}
		}

		if (!bestZone || !bestTemplate || !bestState)
			return;

		TrySpawnWave(bestZone, bestState, bestTemplate, bestReason);
	}

	protected float ScoreZone(notnull OCD_ZombieDirectorZoneDefinition zone, notnull OCD_ZombieDirectorZoneRuntimeState state, out string reason)
	{
		float score = 0;
		reason = "idle";

		foreach (int playerId, OCD_ZombieDirectorPlayerTrack track : m_mPlayers)
		{
			if (!track || !track.m_bHasPosition)
				continue;

			float distance = OCD_ZombieDirectorMath.Distance2D(track.m_vPosition, zone.GetZoneOrigin());
			if (distance > zone.GetRadius() + zone.GetMaxSpawnDistance())
				continue;

			float proximity = 1.0 - OCD_ZombieDirectorMath.ClampFloat(distance / (zone.GetRadius() + zone.GetMaxSpawnDistance()), 0, 1);
			score += proximity * zone.GetAmbientWeight();
			reason = "player proximity";

			if (zone.HasTag("base"))
			{
				score += proximity * zone.GetBaseWeight() * 1.5;
				reason = "base pressure";
			}

			if (zone.HasTag("field") && track.m_vMoveDirection.LengthSq() > 0.1)
			{
				vector towardZone = OCD_ZombieDirectorMath.NormalizeOr(zone.GetZoneOrigin() - track.m_vPosition);
				float forwardDot = vector.Dot(track.m_vMoveDirection, towardZone);
				if (forwardDot > 0.25)
				{
					score += forwardDot * zone.GetFieldWeight();
					reason = "open-field approach";
				}
			}
		}

		if (state.m_bPlayerInside)
		{
			score += 0.35 * zone.GetAmbientWeight();
			if (reason == "idle")
				reason = "occupied zone";
		}

		if (m_fQuietAccumulator >= m_fQuietSecondsBeforePressure)
		{
			score += 0.75 * zone.GetAmbientWeight();
			reason = "quiet-session pressure";
		}

		if (state.m_fExternalWeight > 0)
		{
			score += state.m_fExternalWeight;
			if (!state.m_sLastReason.IsEmpty())
				reason = state.m_sLastReason;
			else
				reason = "external hint";
		}

		return score;
	}

	protected OCD_ZombieUnitTemplateConfig SelectTemplateForZone(notnull OCD_ZombieDirectorZoneDefinition zone, notnull OCD_ZombieDirectorZoneRuntimeState state)
	{
		if (!state.m_sForcedTemplateId.IsEmpty())
		{
			foreach (OCD_ZombieUnitTemplateConfig templateForced : m_aTemplates)
			{
				if (templateForced && templateForced.m_sId == state.m_sForcedTemplateId && zone.AllowsTemplate(templateForced.m_sId) && templateForced.IsValid())
					return templateForced;
			}
		}

		float bestWeight = -1;
		OCD_ZombieUnitTemplateConfig bestTemplate = null;
		array<string> zoneTags = {};
		zone.GetTags(zoneTags);

		foreach (OCD_ZombieUnitTemplateConfig template : m_aTemplates)
		{
			if (!template || !template.IsValid())
				continue;

			if (!zone.AllowsTemplate(template.m_sId))
				continue;

			array<string> templateTags = {};
			template.GetPreferredTags(templateTags);

			float desirability = template.m_fWeight + OCD_ZombieDirectorCsv.CountMatches(zoneTags, templateTags);
			if (template.m_bSpawnAsGroupPrefab)
				desirability += 0.5;

			if (desirability > bestWeight)
			{
				bestWeight = desirability;
				bestTemplate = template;
			}
		}

		return bestTemplate;
	}

	protected void TrySpawnWave(notnull OCD_ZombieDirectorZoneDefinition zone, notnull OCD_ZombieDirectorZoneRuntimeState state, notnull OCD_ZombieUnitTemplateConfig template, string reason)
	{
		if (ShouldPrimeZoneBeforeFirstSpawn(zone, state, template))
		{
			state.m_bSpawnPrimed = true;
			float primeCooldown = OCD_ZombieDirectorMath.ClampFloat(zone.GetBaseCooldownSeconds() * 0.35, 6.0, 12.0);
			if (state.m_fCooldownRemaining < primeCooldown)
				state.m_fCooldownRemaining = primeCooldown;

			Log("Primed zone " + zone.GetZoneId() + " for first-use pathing stability; delaying initial wave.");
			return;
		}

		if (template.m_bSpawnAsGroupPrefab)
		{
			if (template.m_iBudgetCost > m_fBudgetCurrent)
				return;

			vector spawnPointGroup = ChooseSpawnPoint(zone, GROUP_SPAWN_NAVMESH_CLEARANCE);
			vector focusPointGroup = ResolveFocusPoint(zone, spawnPointGroup);
			IEntity spawnedGroup = SpawnRuntimeEntity(template.m_sGroupPrefab, spawnPointGroup, focusPointGroup);
			if (!spawnedGroup)
				return;

			state.m_bSpawnPrimed = true;
			array<IEntity> dynamicWaypointEntities = null;
			ApplyGroupSweep(zone, spawnedGroup, dynamicWaypointEntities);
			TrackSpawn(zone, template, spawnedGroup, template.m_iBudgetCost, reason, dynamicWaypointEntities);
			return;
		}

		int memberCount = template.RollMemberCount();
		if (state.m_iRequestedBudget > 0)
		{
			int budgetDivisor = template.m_iBudgetCost;
			if (budgetDivisor < 1)
				budgetDivisor = 1;

			int requestedCount = state.m_iRequestedBudget / budgetDivisor;
			requestedCount = OCD_ZombieDirectorMath.ClampInt(requestedCount, template.m_iMinMembers, template.m_iMaxMembers);
			if (requestedCount > memberCount)
				memberCount = requestedCount;
		}

		int totalCost = memberCount * template.m_iBudgetCost;
		if (totalCost > m_fBudgetCurrent)
			return;

		for (int i = 0; i < memberCount; i++)
		{
			ResourceName memberPrefab = template.GetRandomMemberPrefab();
			if (memberPrefab.IsEmpty())
				continue;

			vector spawnPoint = ChooseSpawnPoint(zone, INDIVIDUAL_SPAWN_NAVMESH_CLEARANCE);
			vector focusPoint = ResolveFocusPoint(zone, spawnPoint);
			IEntity spawnedEntity = SpawnRuntimeEntity(memberPrefab, spawnPoint, focusPoint);
			if (!spawnedEntity)
				continue;

			state.m_bSpawnPrimed = true;
			TrackSpawn(zone, template, spawnedEntity, template.m_iBudgetCost, reason);
		}
	}

	protected bool ShouldPrimeZoneBeforeFirstSpawn(notnull OCD_ZombieDirectorZoneDefinition zone, notnull OCD_ZombieDirectorZoneRuntimeState state, notnull OCD_ZombieUnitTemplateConfig template)
	{
		if (state.m_bSpawnPrimed)
			return false;

		if (zone.HasSweepRoutePoints())
			return true;

		if (zone.HasTag("lab") || zone.HasTag("quarantine") || zone.HasTag("roadblock"))
			return true;

		return template.m_bSpawnAsGroupPrefab;
	}

	protected IEntity SpawnRuntimeEntity(ResourceName prefab, vector spawnPoint, vector focusPoint)
	{
		vector direction = OCD_ZombieDirectorMath.NormalizeOr(focusPoint - spawnPoint);
		vector transform[4];
		Math3D.DirectionAndUpMatrix(direction, "0 1 0", transform);
		transform[3] = spawnPoint;

		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return null;

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[0] = transform[0];
		spawnParams.Transform[1] = transform[1];
		spawnParams.Transform[2] = transform[2];
		spawnParams.Transform[3] = transform[3];

		return GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), spawnParams);
	}

	protected vector ChooseSpawnPoint(notnull OCD_ZombieDirectorZoneDefinition zone, float requiredNavmeshClearance = 0.0)
	{
		vector routedSpawnPoint;
		if (TryChooseRouteSpawnPoint(zone, routedSpawnPoint, requiredNavmeshClearance))
			return routedSpawnPoint;

		vector zoneOrigin = zone.GetZoneOrigin();
		OCD_ZombieDirectorPlayerTrack nearestPlayer = GetNearestPlayer(zoneOrigin);
		if (!nearestPlayer)
		{
			if (TryChooseSafeRingSpawnPoint(zoneOrigin, zone.GetMinSpawnDistance(), zone.GetMaxSpawnDistance(), routedSpawnPoint, requiredNavmeshClearance))
				return routedSpawnPoint;

			if (TryResolveTerrainPoint(zoneOrigin, zone.GetMinSpawnDistance(), routedSpawnPoint, requiredNavmeshClearance))
				return routedSpawnPoint;

			if (TryResolveZoneFallbackPoint(zone, routedSpawnPoint, requiredNavmeshClearance))
				return routedSpawnPoint;

			if (TryGetResolvedRouteFallbackPoint(zone, routedSpawnPoint))
				return routedSpawnPoint;

			if (TryGetRawRouteFallbackPoint(zone, routedSpawnPoint))
				return routedSpawnPoint;

			return zoneOrigin;
		}

		vector biasDirection = zoneOrigin - nearestPlayer.m_vPosition;
		if (zone.HasTag("field") && nearestPlayer.m_vMoveDirection.LengthSq() > 0.1)
			biasDirection = nearestPlayer.m_vMoveDirection;

		float lateral = zone.GetRadius() * 0.5;
		if (TryChooseSafeBiasedSpawnPoint(zoneOrigin, biasDirection, zone.GetMinSpawnDistance(), zone.GetMaxSpawnDistance(), lateral, routedSpawnPoint, requiredNavmeshClearance))
			return routedSpawnPoint;

		if (TryChooseSafeRingSpawnPoint(zoneOrigin, zone.GetMinSpawnDistance(), zone.GetMaxSpawnDistance(), routedSpawnPoint, requiredNavmeshClearance))
			return routedSpawnPoint;

		if (TryResolveTerrainPoint(zoneOrigin, zone.GetMinSpawnDistance(), routedSpawnPoint, requiredNavmeshClearance))
			return routedSpawnPoint;

		if (TryResolveZoneFallbackPoint(zone, routedSpawnPoint, requiredNavmeshClearance))
			return routedSpawnPoint;

		if (TryGetResolvedRouteFallbackPoint(zone, routedSpawnPoint))
			return routedSpawnPoint;

		if (TryGetRawRouteFallbackPoint(zone, routedSpawnPoint))
			return routedSpawnPoint;

		return zoneOrigin;
	}

	protected bool TryResolveZoneFallbackPoint(notnull OCD_ZombieDirectorZoneDefinition zone, out vector outPoint, float requiredNavmeshClearance = 0.0)
	{
		float fallbackSearchRadius = OCD_ZombieDirectorMath.MaxFloat(zone.GetMaxSpawnDistance(), zone.GetMinSpawnDistance());
		if (fallbackSearchRadius < zone.GetRadius() * 0.35)
			fallbackSearchRadius = zone.GetRadius() * 0.35;

		array<vector> rawRoutePoints = {};
		zone.GetSweepRoutePoints(rawRoutePoints);
		foreach (vector rawRoutePoint : rawRoutePoints)
		{
			if (TryResolveTerrainPoint(rawRoutePoint, fallbackSearchRadius, outPoint, requiredNavmeshClearance))
				return true;
		}

		if (TryChooseSafeRingSpawnPoint(zone.GetZoneOrigin(), 0.0, fallbackSearchRadius, outPoint, requiredNavmeshClearance))
			return true;

		return TryResolveTerrainPoint(zone.GetZoneOrigin(), fallbackSearchRadius, outPoint, requiredNavmeshClearance);
	}

	protected bool TryGetResolvedRouteFallbackPoint(notnull OCD_ZombieDirectorZoneDefinition zone, out vector outPoint)
	{
		array<vector> routePoints = {};
		GetResolvedRoutePoints(zone, routePoints);
		if (routePoints.IsEmpty())
			return false;

		return TryChooseBestFallbackRoutePoint(zone, routePoints, outPoint);
	}

	protected bool TryGetRawRouteFallbackPoint(notnull OCD_ZombieDirectorZoneDefinition zone, out vector outPoint)
	{
		array<vector> routePoints = {};
		zone.GetSweepRoutePoints(routePoints);
		if (routePoints.IsEmpty())
			return false;

		return TryChooseBestFallbackRoutePoint(zone, routePoints, outPoint);
	}

	protected bool TryChooseBestFallbackRoutePoint(notnull OCD_ZombieDirectorZoneDefinition zone, notnull array<vector> routePoints, out vector outPoint)
	{
		int chosenIndex = Math.RandomInt(0, routePoints.Count());
		OCD_ZombieDirectorPlayerTrack nearestPlayer = GetNearestPlayer(zone.GetZoneOrigin());
		if (nearestPlayer)
		{
			float bestDistance = -1.0;
			for (int i = 0; i < routePoints.Count(); i++)
			{
				float playerDistance = OCD_ZombieDirectorMath.Distance2D(nearestPlayer.m_vPosition, routePoints[i]);
				if (playerDistance <= bestDistance)
					continue;

				bestDistance = playerDistance;
				chosenIndex = i;
			}
		}

		outPoint = routePoints[chosenIndex];
		return true;
	}

	protected bool TryChooseRouteSpawnPoint(notnull OCD_ZombieDirectorZoneDefinition zone, out vector outSpawnPoint, float requiredNavmeshClearance = 0.0)
	{
		array<vector> routePoints = {};
		GetResolvedRoutePoints(zone, routePoints);
		if (routePoints.IsEmpty())
			return false;

		int chosenIndex = Math.RandomInt(0, routePoints.Count());
		OCD_ZombieDirectorPlayerTrack nearestPlayer = GetNearestPlayer(zone.GetZoneOrigin());
		if (nearestPlayer)
		{
			float bestDistance = -1.0;
			for (int i = 0; i < routePoints.Count(); i++)
			{
				float playerDistance = OCD_ZombieDirectorMath.Distance2D(nearestPlayer.m_vPosition, routePoints[i]);
				if (playerDistance <= bestDistance)
					continue;

				bestDistance = playerDistance;
				chosenIndex = i;
			}
		}

		float routeMinDistance = OCD_ZombieDirectorMath.ClampFloat(zone.GetMinSpawnDistance() * 0.2, 6.0, 18.0);
		float routeMaxDistance = OCD_ZombieDirectorMath.ClampFloat(zone.GetMinSpawnDistance() * 0.45, routeMinDistance + 2.0, 26.0);
		for (int routeOffset = 0; routeOffset < routePoints.Count(); routeOffset++)
		{
			int candidateIndex = (chosenIndex + routeOffset) % routePoints.Count();
			if (TryChooseSafeRingSpawnPoint(routePoints[candidateIndex], routeMinDistance, routeMaxDistance, outSpawnPoint, requiredNavmeshClearance))
				return true;

			if (TryResolveTerrainPoint(routePoints[candidateIndex], routeMaxDistance, outSpawnPoint, requiredNavmeshClearance))
				return true;
		}

		return false;
	}

	protected vector ResolveFocusPoint(notnull OCD_ZombieDirectorZoneDefinition zone, vector spawnPoint)
	{
		array<vector> routePoints = {};
		GetResolvedRoutePoints(zone, routePoints);
		if (routePoints.IsEmpty())
			return zone.GetZoneOrigin();

		float bestDistance = float.MAX;
		int closestIndex = 0;
		for (int i = 0; i < routePoints.Count(); i++)
		{
			float routeDistance = OCD_ZombieDirectorMath.Distance2D(routePoints[i], spawnPoint);
			if (routeDistance >= bestDistance)
				continue;

			bestDistance = routeDistance;
			closestIndex = i;
		}

		if (closestIndex < routePoints.Count() - 1)
			return routePoints[closestIndex + 1];

		if (closestIndex > 0)
			return routePoints[closestIndex - 1];

		return zone.GetZoneOrigin();
	}

	protected void ApplyGroupSweep(notnull OCD_ZombieDirectorZoneDefinition zone, notnull IEntity groupEntity, out array<IEntity> outDynamicWaypointEntities)
	{
		outDynamicWaypointEntities = null;

		SCR_AIGroup group = SCR_AIGroup.Cast(groupEntity);
		if (!group)
			return;

		array<string> waypointNames = {};
		zone.GetSweepWaypointNames(waypointNames);
		ClearGroupWaypoints(group);

		if (!waypointNames.IsEmpty())
		{
			group.AddWaypointsStatic(waypointNames);
			TryAppendCycleWaypoint(zone, group, outDynamicWaypointEntities);
			Log("Applied static sweep path to zone " + zone.GetZoneId() + " (" + waypointNames.Count() + " waypoints).");
			return;
		}

		array<vector> routePoints = {};
		GetResolvedRoutePoints(zone, routePoints);
		if (routePoints.IsEmpty())
			return;

		array<ref SCR_WaypointPrefabLocation> dynamicWaypoints = {};
		for (int i = 0; i < routePoints.Count(); i++)
		{
			SCR_WaypointPrefabLocation waypointLocation = new SCR_WaypointPrefabLocation();
			waypointLocation.m_WPPrefabName = WAYPOINT_PREFAB_MOVE;
			waypointLocation.m_WPInstanceName = zone.GetZoneId() + "_dynamic_" + i;
			waypointLocation.m_WPWorldLocation = routePoints[i];
			dynamicWaypoints.Insert(waypointLocation);
		}

		if (dynamicWaypoints.IsEmpty())
			return;

		outDynamicWaypointEntities = {};
		group.AddWaypointsDynamic(outDynamicWaypointEntities, dynamicWaypoints);
		TryAppendCycleWaypoint(zone, group, outDynamicWaypointEntities, routePoints[routePoints.Count() - 1]);
		Log("Applied dynamic sweep path to zone " + zone.GetZoneId() + " (" + dynamicWaypoints.Count() + " route points).");
	}

	protected void ClearGroupWaypoints(notnull SCR_AIGroup group)
	{
		array<AIWaypoint> existingWaypoints = {};
		group.GetWaypoints(existingWaypoints);
		foreach (AIWaypoint waypoint : existingWaypoints)
			group.RemoveWaypoint(waypoint);
	}

	protected void TryAppendCycleWaypoint(notnull OCD_ZombieDirectorZoneDefinition zone, notnull SCR_AIGroup group, out array<IEntity> dynamicWaypointEntities, vector anchorPoint = "0 0 0")
	{
		if (!zone.ShouldCycleWaypoints())
			return;

		array<AIWaypoint> cycleWaypoints = {};
		group.GetWaypoints(cycleWaypoints);
		if (cycleWaypoints.IsEmpty())
			return;

		vector cyclePoint = anchorPoint;
		if (cyclePoint.LengthSq() <= 0.001)
			cyclePoint = zone.GetZoneOrigin();

		IEntity cycleEntity = SpawnWaypointEntity(WAYPOINT_PREFAB_CYCLE, cyclePoint);
		if (!cycleEntity)
			return;

		AIWaypointCycle cycleWaypoint = AIWaypointCycle.Cast(cycleEntity);
		if (!cycleWaypoint)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(cycleEntity);
			return;
		}

		cycleWaypoint.SetWaypoints(cycleWaypoints);
		group.AddWaypoint(cycleWaypoint);

		if (!dynamicWaypointEntities)
			dynamicWaypointEntities = {};

		dynamicWaypointEntities.Insert(cycleEntity);
	}

	protected IEntity SpawnWaypointEntity(ResourceName prefab, vector worldPoint)
	{
		vector transform[4];
		Math3D.DirectionAndUpMatrix("1 0 0", "0 1 0", transform);
		transform[3] = worldPoint;

		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return null;

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[0] = transform[0];
		spawnParams.Transform[1] = transform[1];
		spawnParams.Transform[2] = transform[2];
		spawnParams.Transform[3] = transform[3];
		return GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), spawnParams);
	}

	protected void GetResolvedRoutePoints(notnull OCD_ZombieDirectorZoneDefinition zone, out notnull array<vector> outRoutePoints)
	{
		outRoutePoints.Clear();

		array<vector> rawRoutePoints = {};
		zone.GetSweepRoutePoints(rawRoutePoints);
		foreach (vector rawRoutePoint : rawRoutePoints)
		{
			vector resolvedRoutePoint;
			if (!TryResolveTerrainPoint(rawRoutePoint, 12.0, resolvedRoutePoint, ROUTE_POINT_NAVMESH_CLEARANCE))
				continue;

			if (OCD_ZombieDirectorMath.Distance2D(rawRoutePoint, resolvedRoutePoint) > MAX_ROUTE_POINT_RESOLVE_DRIFT)
				continue;

			outRoutePoints.Insert(resolvedRoutePoint);
		}
	}

	protected bool TryChooseSafeRingSpawnPoint(vector center, float minDistance, float maxDistance, out vector outPoint, float requiredNavmeshClearance = 0.0)
	{
		for (int attempt = 0; attempt < TERRAIN_POINT_ATTEMPTS; attempt++)
		{
			vector candidatePoint = OCD_ZombieDirectorMath.BuildRingPoint(center, minDistance, maxDistance);
			if (TryResolveTerrainPoint(candidatePoint, minDistance, outPoint, requiredNavmeshClearance))
				return true;
		}

		return false;
	}

	protected bool TryChooseSafeBiasedSpawnPoint(vector center, vector preferredDirection, float minDistance, float maxDistance, float lateralDistance, out vector outPoint, float requiredNavmeshClearance = 0.0)
	{
		for (int attempt = 0; attempt < TERRAIN_POINT_ATTEMPTS; attempt++)
		{
			vector candidatePoint = OCD_ZombieDirectorMath.BuildBiasedSpawnPoint(center, preferredDirection, minDistance, maxDistance, lateralDistance);
			if (TryResolveTerrainPoint(candidatePoint, minDistance, outPoint, requiredNavmeshClearance))
				return true;
		}

		return false;
	}

	protected bool TryResolveTerrainPoint(vector desiredPoint, float searchRadiusHint, out vector outPoint, float requiredNavmeshClearance = 0.0)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return false;

		NavmeshWorldComponent soldiersNavmesh = GetSoldiersNavmeshWorld();
		vector searchCenter = desiredPoint;
		searchCenter[1] = world.GetSurfaceY(searchCenter[0], searchCenter[2]);

		float baseRadius = OCD_ZombieDirectorMath.ClampFloat(searchRadiusHint * 0.6, TERRAIN_POINT_MIN_RADIUS, TERRAIN_POINT_MAX_RADIUS);
		if (requiredNavmeshClearance > baseRadius)
			baseRadius = OCD_ZombieDirectorMath.ClampFloat(requiredNavmeshClearance, TERRAIN_POINT_MIN_RADIUS, TERRAIN_POINT_MAX_RADIUS);

		array<float> searchRadii = {
			0.0,
			OCD_ZombieDirectorMath.ClampFloat(baseRadius * 0.5, 0.0, TERRAIN_POINT_MAX_RADIUS),
			baseRadius,
			OCD_ZombieDirectorMath.ClampFloat(baseRadius * 1.5, TERRAIN_POINT_MIN_RADIUS, TERRAIN_POINT_MAX_RADIUS),
			OCD_ZombieDirectorMath.ClampFloat(baseRadius * 2.0, TERRAIN_POINT_MIN_RADIUS, TERRAIN_POINT_MAX_RADIUS)
		};

		foreach (float ringRadius : searchRadii)
		{
			if (TryResolveTerrainPointAtRadius(world, soldiersNavmesh, searchCenter, ringRadius, baseRadius, requiredNavmeshClearance, outPoint))
				return true;
		}

		return false;
	}

	protected NavmeshWorldComponent GetSoldiersNavmeshWorld()
	{
		ChimeraGame chimeraGame = ChimeraGame.Cast(GetGame());
		if (!chimeraGame)
			return null;

		AIWorld aiWorld = chimeraGame.GetAIWorld();
		if (!aiWorld)
			return null;

		return aiWorld.GetNavmeshWorldComponent("Soldiers");
	}

	protected bool TryResolveTerrainPointAtRadius(BaseWorld world, NavmeshWorldComponent soldiersNavmesh, vector searchCenter, float ringRadius, float terrainSearchRadius, float requiredNavmeshClearance, out vector outPoint)
	{
		int sampleCount = 1;
		if (ringRadius > 0.01)
			sampleCount = 10;

		for (int sampleIndex = 0; sampleIndex < sampleCount; sampleIndex++)
		{
			vector sampleCenter = searchCenter;
			if (ringRadius > 0.01)
			{
				float angle = (6.2831855 * sampleIndex) / sampleCount;
				sampleCenter[0] = searchCenter[0] + (Math.Cos(angle) * ringRadius);
				sampleCenter[2] = searchCenter[2] + (Math.Sin(angle) * ringRadius);
				sampleCenter[1] = world.GetSurfaceY(sampleCenter[0], sampleCenter[2]);
			}

			if (!TryFindEmptyTerrainAround(world, sampleCenter, terrainSearchRadius, outPoint))
				continue;

			vector constrainedPoint;
			if (!TryConstrainToSoldiersNavmeshPoint(world, soldiersNavmesh, outPoint, terrainSearchRadius + requiredNavmeshClearance, constrainedPoint))
				continue;

			if (!IsValidSoldiersNavmeshFootprint(world, soldiersNavmesh, constrainedPoint, requiredNavmeshClearance))
				continue;

			outPoint = constrainedPoint;
			return true;
		}

		return false;
	}

	protected bool TryFindEmptyTerrainAround(BaseWorld world, vector searchCenter, float terrainSearchRadius, out vector outPoint)
	{
		float resolvedSearchRadius = OCD_ZombieDirectorMath.ClampFloat(terrainSearchRadius, TERRAIN_POINT_MIN_RADIUS, TERRAIN_POINT_MAX_RADIUS);
		if (!SCR_WorldTools.FindEmptyTerrainPosition(outPoint, searchCenter, resolvedSearchRadius, 1.0, 2.0, TraceFlags.ENTS | TraceFlags.OCEAN, world))
			return false;

		outPoint[1] = world.GetSurfaceY(outPoint[0], outPoint[2]);
		return true;
	}

	protected bool TryConstrainToSoldiersNavmeshPoint(BaseWorld world, NavmeshWorldComponent soldiersNavmesh, vector desiredPoint, float searchRadius, out vector outPoint)
	{
		outPoint = desiredPoint;
		if (!soldiersNavmesh)
			return true;

		vector reachablePoint;
		if (soldiersNavmesh.GetReachablePoint(desiredPoint, searchRadius, reachablePoint))
		{
			reachablePoint[1] = world.GetSurfaceY(reachablePoint[0], reachablePoint[2]);
			outPoint = reachablePoint;
		}

		return IsValidSoldiersNavmeshPoint(soldiersNavmesh, outPoint);
	}

	protected bool IsValidSoldiersNavmeshPoint(NavmeshWorldComponent soldiersNavmesh, vector point)
	{
		if (!soldiersNavmesh)
			return true;

		return soldiersNavmesh.IsTileValid(point);
	}

	protected bool IsValidSoldiersNavmeshFootprint(BaseWorld world, NavmeshWorldComponent soldiersNavmesh, vector point, float requiredNavmeshClearance)
	{
		if (!IsValidSoldiersNavmeshPoint(soldiersNavmesh, point))
			return false;

		if (!soldiersNavmesh || requiredNavmeshClearance <= 0.01)
			return true;

		array<float> sampleRadii = {};
		float innerRadius = requiredNavmeshClearance * 0.55;
		if (innerRadius >= 2.0)
			sampleRadii.Insert(innerRadius);

		sampleRadii.Insert(requiredNavmeshClearance);

		foreach (float sampleRadius : sampleRadii)
		{
			for (int sampleIndex = 0; sampleIndex < NAVMESH_FOOTPRINT_SAMPLE_COUNT; sampleIndex++)
			{
				float angle = (6.2831855 * sampleIndex) / NAVMESH_FOOTPRINT_SAMPLE_COUNT;
				vector samplePoint = point;
				samplePoint[0] = point[0] + (Math.Cos(angle) * sampleRadius);
				samplePoint[2] = point[2] + (Math.Sin(angle) * sampleRadius);
				samplePoint[1] = world.GetSurfaceY(samplePoint[0], samplePoint[2]);
				if (!IsValidSoldiersNavmeshPoint(soldiersNavmesh, samplePoint))
					return false;
			}
		}

		return true;
	}

	protected void TrackSpawn(notnull OCD_ZombieDirectorZoneDefinition zone, notnull OCD_ZombieUnitTemplateConfig template, notnull IEntity spawnedEntity, int budgetCost, string reason, array<IEntity> dynamicWaypointEntities = null)
	{
		OCD_ZombieDirectorActiveWave wave = new OCD_ZombieDirectorActiveWave();
		wave.m_sZoneId = zone.GetZoneId();
		wave.m_sTemplateId = template.m_sId;
		wave.m_iBudgetCost = budgetCost;
		wave.m_Entity = spawnedEntity;
		if (dynamicWaypointEntities)
			wave.m_aDynamicWaypointEntities = dynamicWaypointEntities;
		m_aActiveWaves.Insert(wave);

		m_fBudgetCurrent -= budgetCost;
		if (m_fBudgetCurrent < 0)
			m_fBudgetCurrent = 0;

		OCD_ZombieDirectorZoneRuntimeState state = GetOrCreateZoneState(zone.GetZoneId());
		state.m_iActiveBudget += budgetCost;
		state.m_fCooldownRemaining = zone.GetBaseCooldownSeconds();
		state.m_fExternalWeight = 0;
		state.m_fExternalTtlSeconds = 0;
		state.m_sForcedTemplateId = string.Empty;
		state.m_iRequestedBudget = 0;
		state.m_sLastReason = reason;

		vector origin = "0 0 0";
		if (spawnedEntity && !spawnedEntity.IsDeleted())
			origin = spawnedEntity.GetOrigin();

		Log("Spawned wave template=" + template.m_sId + " zone=" + zone.GetZoneId() + " budget=" + budgetCost + " origin=" + origin + " reason=" + reason + ".");
	}

	protected void ReleaseWaveBudget(notnull OCD_ZombieDirectorActiveWave wave)
	{
		OCD_ZombieDirectorZoneRuntimeState state = GetOrCreateZoneState(wave.m_sZoneId);
		state.m_iActiveBudget -= wave.m_iBudgetCost;
		if (state.m_iActiveBudget < 0)
			state.m_iActiveBudget = 0;
	}

	protected OCD_ZombieDirectorZoneDefinition FindZoneById(string zoneId)
	{
		foreach (OCD_ZombieDirectorZoneDefinition zone : m_aZones)
		{
			if (zone && zone.GetZoneId() == zoneId)
				return zone;
		}

		return null;
	}

	protected OCD_ZombieDirectorPlayerTrack GetNearestPlayer(vector point)
	{
		float bestDistance = float.MAX;
		OCD_ZombieDirectorPlayerTrack bestTrack = null;

		foreach (int playerId, OCD_ZombieDirectorPlayerTrack track : m_mPlayers)
		{
			if (!track || !track.m_bHasPosition)
				continue;

			float distance = OCD_ZombieDirectorMath.Distance2D(track.m_vPosition, point);
			if (distance < bestDistance)
			{
				bestDistance = distance;
				bestTrack = track;
			}
		}

		return bestTrack;
	}

	protected void EnsureConcreteTemplates()
	{
		if (m_sConcreteProfileId != "dead_everon_bacon_concrete")
			return;

		if (!m_aTemplates)
			m_aTemplates = {};

		if (!m_aTemplates.IsEmpty())
			return;

		m_aTemplates.Insert(BuildGroupTemplate(
			"dead_everon_civilian_drifters",
			"village,ruin,woods,road",
			3,
			0.95,
			"{E1C3B50CFCC34910}Prefabs/Compatibility/Bacon/Groups/OCD_BaconZombieGroup_CIV_Compatibility.et"
		));

		m_aTemplates.Insert(BuildGroupTemplate(
			"dead_everon_checkpoint_mix",
			"checkpoint,roadblock,base,quarantine",
			4,
			1.15,
			"{E1C3B50CFCC34910}Prefabs/Compatibility/Bacon/Groups/OCD_BaconZombieGroup_CIV_Compatibility.et"
		));

		m_aTemplates.Insert(BuildGroupTemplate(
			"dead_everon_quarantine_spike",
			"quarantine,lab,roadblock,checkpoint",
			4,
			0.9,
			"{E1C3B50CFCC34910}Prefabs/Compatibility/Bacon/Groups/OCD_BaconZombieGroup_CIV_Compatibility.et"
		));

		m_aTemplates.Insert(BuildGroupTemplate(
			"dead_everon_tier1_sweep",
			"village,woods,road,ruin",
			4,
			1.35,
			"{E1C3B50CFCC34910}Prefabs/Compatibility/Bacon/Groups/OCD_BaconZombieGroup_CIV_Compatibility.et"
		));

		m_aTemplates.Insert(BuildGroupTemplate(
			"dead_everon_ussr_patrol_large",
			"checkpoint,base,roadblock,quarantine",
			5,
			1.0,
			"{E1C3B50CFCC34910}Prefabs/Compatibility/Bacon/Groups/OCD_BaconZombieGroup_CIV_Compatibility.et"
		));

		m_aTemplates.Insert(BuildGroupTemplate(
			"dead_everon_horde_push",
			"base,quarantine,village,ruin",
			6,
			0.8,
			"{E1C3B50CFCC34910}Prefabs/Compatibility/Bacon/Groups/OCD_BaconZombieGroup_CIV_Compatibility.et"
		));

		m_aTemplates.Insert(BuildGroupTemplate(
			"dead_everon_heavy_brute",
			"base,lab,quarantine",
			6,
			0.25,
			"{E1C3B50CFCC34910}Prefabs/Compatibility/Bacon/Groups/OCD_BaconZombieGroup_CIV_Compatibility.et"
		));

		Log("Seeded " + m_aTemplates.Count() + " concrete Bacon templates for dead_everon_bacon_concrete.");
	}

	protected OCD_ZombieUnitTemplateConfig BuildIndividualTemplate(
		string templateId,
		string preferredTagsCsv,
		int minMembers,
		int maxMembers,
		int budgetCost,
		float weight,
		ResourceName prefabA = ResourceName.Empty,
		ResourceName prefabB = ResourceName.Empty,
		ResourceName prefabC = ResourceName.Empty,
		ResourceName prefabD = ResourceName.Empty,
		ResourceName prefabE = ResourceName.Empty
	)
	{
		OCD_ZombieUnitTemplateConfig template = BuildTemplateShell(templateId, preferredTagsCsv, budgetCost, weight);
		template.m_bSpawnAsGroupPrefab = false;
		template.m_iMinMembers = minMembers;
		template.m_iMaxMembers = maxMembers;

		InsertMemberPrefab(template, prefabA);
		InsertMemberPrefab(template, prefabB);
		InsertMemberPrefab(template, prefabC);
		InsertMemberPrefab(template, prefabD);
		InsertMemberPrefab(template, prefabE);
		return template;
	}

	protected OCD_ZombieUnitTemplateConfig BuildGroupTemplate(string templateId, string preferredTagsCsv, int budgetCost, float weight, ResourceName groupPrefab)
	{
		OCD_ZombieUnitTemplateConfig template = BuildTemplateShell(templateId, preferredTagsCsv, budgetCost, weight);
		template.m_bSpawnAsGroupPrefab = true;
		template.m_sGroupPrefab = groupPrefab;
		return template;
	}

	protected OCD_ZombieUnitTemplateConfig BuildTemplateShell(string templateId, string preferredTagsCsv, int budgetCost, float weight)
	{
		OCD_ZombieUnitTemplateConfig template = new OCD_ZombieUnitTemplateConfig();
		template.m_sId = templateId;
		template.m_sPreferredZoneTagsCsv = preferredTagsCsv;
		template.m_iBudgetCost = budgetCost;
		template.m_fWeight = weight;
		return template;
	}

	protected void InsertMemberPrefab(notnull OCD_ZombieUnitTemplateConfig template, ResourceName prefab)
	{
		if (prefab.IsEmpty())
			return;

		template.m_aMemberPrefabs.Insert(prefab);
	}

	protected void ScheduleSmokeBootstrap()
	{
		int initialDelayMs = 6000;
		if (m_fSmokeBootstrapInitialDelaySeconds > 0.0)
			initialDelayMs = m_fSmokeBootstrapInitialDelaySeconds * 1000;

		int stageGapMs = 9000;
		if (m_fSmokeBootstrapStageGapSeconds > 0.0)
			stageGapMs = m_fSmokeBootstrapStageGapSeconds * 1000;

		GetGame().GetCallqueue().CallLater(SmokeBootstrapStageOne, initialDelayMs, false);
		GetGame().GetCallqueue().CallLater(SmokeBootstrapStageTwo, initialDelayMs + stageGapMs, false);
		Log("Scheduled smoke bootstrap stages for editor validation.");
	}

	protected void SmokeBootstrapStageOne()
	{
		QueueSmokeHint("dead_everon_civilian_drifters", false, 6, 2.1, "smoke bootstrap baseline drifters", "smoke_bootstrap_a");
	}

	protected void SmokeBootstrapStageTwo()
	{
		QueueSmokeHint("dead_everon_tier1_sweep", true, 9, 2.6, "smoke bootstrap sweep route", "smoke_bootstrap_b");
	}

	protected void QueueSmokeHint(string templateId, bool requireSweepPath, int requestedBudget, float weight, string reason, string correlationSuffix)
	{
		if (!HasTemplate(templateId))
		{
			Log("Skipping smoke hint because template is unavailable: " + templateId + ".");
			return;
		}

		OCD_ZombieDirectorZoneDefinition zone = FindSmokeZone(templateId, requireSweepPath);
		if (!zone)
		{
			Log("Skipping smoke hint because no zone matched template " + templateId + ".");
			return;
		}

		string correlationId = zone.GetZoneId() + "|" + correlationSuffix + "|" + templateId;
		OCD_ZombieDirectorAPI.QueueLocalHint("smoke_bootstrap", zone.GetZoneId(), templateId, requestedBudget, weight, reason, 55.0, zone.GetZoneOrigin(), correlationId);
		Log("Queued smoke hint template=" + templateId + " zone=" + zone.GetZoneId() + ".");
	}

	protected OCD_ZombieDirectorZoneDefinition FindSmokeZone(string templateId, bool requireSweepPath)
	{
		OCD_ZombieDirectorZoneDefinition firstAllowedZone = null;
		OCD_ZombieDirectorZoneDefinition firstPreferredZone = null;

		foreach (OCD_ZombieDirectorZoneDefinition zone : m_aZones)
		{
			if (!zone || !zone.AllowsTemplate(templateId))
				continue;

			if (requireSweepPath && !ZoneHasSweepPath(zone))
				continue;

			if (!firstAllowedZone)
				firstAllowedZone = zone;

			if (templateId == "dead_everon_civilian_drifters" && (zone.HasTag("village") || zone.HasTag("woods") || zone.HasTag("ruin")))
			{
				firstPreferredZone = zone;
				break;
			}

			if (templateId == "dead_everon_tier1_sweep" && ZoneHasSweepPath(zone) && (zone.HasTag("village") || zone.HasTag("woods") || zone.HasTag("road")))
			{
				firstPreferredZone = zone;
				break;
			}
		}

		if (firstPreferredZone)
			return firstPreferredZone;

		return firstAllowedZone;
	}

	protected bool ZoneHasSweepPath(notnull OCD_ZombieDirectorZoneDefinition zone)
	{
		if (zone.HasSweepRoutePoints())
			return true;

		array<string> waypointNames = {};
		zone.GetSweepWaypointNames(waypointNames);
		return !waypointNames.IsEmpty();
	}

	protected void Log(string message)
	{
		Print("[OCD/ZD] " + message);
	}

	protected void ApplyProfileBridgeOverrideIfPresent()
	{
		if (!m_RestBridge)
			return;

		string overridePath = m_RestBridge.ResolveProfileOverridePath();
		if (overridePath.IsEmpty())
			return;

		if (!FileIO.FileExists(overridePath))
			return;

		FileHandle overrideFile = FileIO.OpenFile(overridePath, FileMode.READ);
		if (!overrideFile || !overrideFile.IsOpen())
		{
			Log("Failed to open REST bridge override at " + overridePath + ".");
			return;
		}

		string overrideJson;
		overrideFile.Read(overrideJson, overrideFile.GetLength());
		overrideFile.Close();

		if (overrideJson.IsEmpty())
		{
			Log("Ignoring empty REST bridge override at " + overridePath + ".");
			return;
		}

		OCD_ZombieDirectorRestBridgeProfileOverride profileOverride = new OCD_ZombieDirectorRestBridgeProfileOverride();
		profileOverride.ExpandFromRAW(overrideJson);

		if (!profileOverride.IsValid())
		{
			Log("Ignoring invalid REST bridge override at " + overridePath + ". Expected kind=openclaw-zombie-director.rest-bridge-override version=1.");
			return;
		}

		m_RestBridge.ApplyProfileOverride(profileOverride);
		Log("Loaded REST bridge override from " + overridePath + ".");
	}

	protected OCD_ZombieDirectorSnapshotJson BuildSnapshot()
	{
		OCD_ZombieDirectorSnapshotJson snapshot = new OCD_ZombieDirectorSnapshotJson();
		snapshot.budgetCurrent = m_fBudgetCurrent;
		snapshot.budgetCap = m_iGlobalBudgetCap;
		snapshot.activeWaveCount = m_aActiveWaves.Count();

		foreach (int playerId, OCD_ZombieDirectorPlayerTrack track : m_mPlayers)
		{
			if (!track || !track.m_bHasPosition)
				continue;

			OCD_ZombieDirectorPlayerSnapshotJson playerJson = new OCD_ZombieDirectorPlayerSnapshotJson();
			playerJson.playerId = track.m_iPlayerId;
			playerJson.name = track.m_sName;
			playerJson.position = track.m_vPosition;
			playerJson.moveDir = track.m_vMoveDirection;
			snapshot.players.Insert(playerJson);
		}

		foreach (OCD_ZombieDirectorZoneDefinition zone : m_aZones)
		{
			if (!zone)
				continue;

			OCD_ZombieDirectorZoneRuntimeState state = GetOrCreateZoneState(zone.GetZoneId());
			OCD_ZombieDirectorZoneSnapshotJson zoneJson = new OCD_ZombieDirectorZoneSnapshotJson();
			zoneJson.zoneId = zone.GetZoneId();
			zoneJson.score = state.m_fLastScore;
			zoneJson.cooldownRemaining = state.m_fCooldownRemaining;
			zoneJson.activeBudget = state.m_iActiveBudget;
			zoneJson.reason = state.m_sLastReason;
			snapshot.zones.Insert(zoneJson);
		}

		return snapshot;
	}
}

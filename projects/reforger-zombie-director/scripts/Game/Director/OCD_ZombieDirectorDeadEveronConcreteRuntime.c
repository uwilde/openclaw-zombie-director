class OCD_ZombieDirectorDeadEveronBaseState
{
	static const float FIRST_THRESHOLD = 0.50;
	static const float SECOND_THRESHOLD = 0.85;
	static const float FINAL_THRESHOLD = 1.00;

	protected SCR_MilitaryBaseComponent m_Base;
	protected ref array<SCR_SeizingComponent> m_aCapturePoints = {};
	protected Faction m_AttackingFaction;
	protected string m_sAttackingFactionKey = string.Empty;
	protected bool m_bAttemptActive;
	protected bool m_bTriggeredFirstWave;
	protected bool m_bTriggeredSecondWave;
	protected bool m_bTriggeredFinalWave;

	void BindBase(notnull SCR_MilitaryBaseComponent baseComponent)
	{
		if (m_Base == baseComponent && !m_aCapturePoints.IsEmpty())
			return;

		m_Base = baseComponent;
		m_aCapturePoints.Clear();
		m_Base.GetCapturePoints(m_aCapturePoints);
	}

	void Reset()
	{
		m_Base = null;
		m_aCapturePoints.Clear();
		ResetAttemptState();
	}

	void Update()
	{
		if (!m_Base || m_aCapturePoints.IsEmpty())
			return;

		if (m_bAttemptActive && IsMatchingFaction(m_Base.GetFaction(), m_sAttackingFactionKey))
		{
			TryTriggerThreshold(FINAL_THRESHOLD);
			ResetAttemptState();
			return;
		}

		EvaluateCaptureProgress();
	}

	protected void EvaluateCaptureProgress()
	{
		if (!EnsureAttemptState())
			return;

		float progress = GetBaseCaptureProgress();
		if (progress <= 0.0)
			return;

		TryTriggerThreshold(progress);
	}

	protected bool EnsureAttemptState()
	{
		Faction capturingFaction = m_Base.GetCapturingFaction();
		Faction baseFaction = m_Base.GetFaction();

		if (!capturingFaction || IsMatchingFaction(capturingFaction, GetFactionKey(baseFaction)))
		{
			ResetAttemptState();
			return false;
		}

		string capturingKey = GetFactionKey(capturingFaction);
		if (capturingKey.IsEmpty())
		{
			ResetAttemptState();
			return false;
		}

		if (!m_bAttemptActive || m_sAttackingFactionKey != capturingKey)
			StartAttempt(capturingFaction);

		return true;
	}

	protected void StartAttempt(Faction capturingFaction)
	{
		m_AttackingFaction = capturingFaction;
		m_sAttackingFactionKey = GetFactionKey(capturingFaction);
		m_bAttemptActive = true;
		m_bTriggeredFirstWave = false;
		m_bTriggeredSecondWave = false;
		m_bTriggeredFinalWave = false;
	}

	protected void ResetAttemptState()
	{
		m_AttackingFaction = null;
		m_sAttackingFactionKey = string.Empty;
		m_bAttemptActive = false;
		m_bTriggeredFirstWave = false;
		m_bTriggeredSecondWave = false;
		m_bTriggeredFinalWave = false;
	}

	protected float GetBaseCaptureProgress()
	{
		if (!m_AttackingFaction)
			return 0.0;

		Faction baseFaction = m_Base.GetFaction();
		if (IsMatchingFaction(baseFaction, m_sAttackingFactionKey))
			return 1.0;

		float totalProgress = 0.0;
		int pointCount = 0;

		foreach (SCR_SeizingComponent capturePoint : m_aCapturePoints)
		{
			if (!capturePoint)
				continue;

			totalProgress += GetCapturePointProgress(capturePoint);
			pointCount++;
		}

		if (pointCount <= 0)
			return 0.0;

		return totalProgress / pointCount;
	}

	protected float GetCapturePointProgress(SCR_SeizingComponent capturePoint)
	{
		if (!capturePoint || !m_AttackingFaction)
			return 0.0;

		if (capturePoint.IsControlledByFaction(m_AttackingFaction))
			return 1.0;

		WorldTimestamp start = capturePoint.GetSeizingStartTimestamp();
		WorldTimestamp end = capturePoint.GetSeizingEndTimestamp();
		float duration = end.DiffSeconds(start);
		if (duration <= 0.001)
			return 0.0;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return 0.0;

		WorldTimestamp now = world.GetTimestamp();
		float elapsed = now.DiffSeconds(start);
		return Math.Clamp(elapsed / duration, 0.0, 1.0);
	}

	protected void TryTriggerThreshold(float progress)
	{
		if (!m_bTriggeredFirstWave && progress >= FIRST_THRESHOLD)
		{
			m_bTriggeredFirstWave = true;
			QueueThresholdHint(FIRST_THRESHOLD);
		}

		if (!m_bTriggeredSecondWave && progress >= SECOND_THRESHOLD)
		{
			m_bTriggeredSecondWave = true;
			QueueThresholdHint(SECOND_THRESHOLD);
		}

		if (!m_bTriggeredFinalWave && progress >= FINAL_THRESHOLD)
		{
			m_bTriggeredFinalWave = true;
			QueueThresholdHint(FINAL_THRESHOLD);
		}
	}

	protected void QueueThresholdHint(float threshold)
	{
		if (!m_Base)
			return;

		IEntity owner = m_Base.GetOwner();
		vector anchor = "0 0 0";
		if (owner)
			anchor = owner.GetOrigin();

		string zoneId = "dead_everon_base_" + m_Base.GetCallsign();
		string type = "reinforce_zone";
		string templateId = "dead_everon_checkpoint_mix";
		int requestedBudget = 10;
		float weight = 1.4;
		float ttlSeconds = 40.0;

		if (threshold >= FINAL_THRESHOLD)
		{
			type = "force_spike";
			templateId = "dead_everon_horde_push";
			requestedBudget = 18;
			weight = 2.3;
			ttlSeconds = 70.0;
		}
		else if (threshold >= SECOND_THRESHOLD)
		{
			type = "force_spike";
			if (m_Base.GetRadius() <= 45.0)
			{
				templateId = "dead_everon_quarantine_spike";
				requestedBudget = 10;
				weight = 1.75;
				ttlSeconds = 50.0;
			}
			else
			{
				templateId = "dead_everon_tier1_sweep";
				requestedBudget = 14;
				weight = 1.9;
				ttlSeconds = 60.0;
			}
		}

		string thresholdLabel = "50";
		if (threshold >= FINAL_THRESHOLD)
			thresholdLabel = "100";
		else if (threshold >= SECOND_THRESHOLD)
			thresholdLabel = "85";

		string reason = "Dead Everon base #" + m_Base.GetCallsign() + " capture pressure at " + thresholdLabel + "%";
		string correlationId = zoneId + "|capture|" + thresholdLabel;
		OCD_ZombieDirectorAPI.QueueLocalHint(type, zoneId, templateId, requestedBudget, weight, reason, ttlSeconds, anchor, correlationId);
	}

	protected string GetFactionKey(Faction faction)
	{
		if (!faction)
			return string.Empty;

		return faction.GetFactionKey();
	}

	protected bool IsMatchingFaction(Faction faction, string factionKey)
	{
		if (!faction || factionKey.IsEmpty())
			return false;

		return faction.GetFactionKey() == factionKey;
	}
}

class OCD_ZombieDirectorDeadEveronConcreteRuntime
{
	protected ref map<int, ref OCD_ZombieDirectorDeadEveronBaseState> m_mBaseStates = new map<int, ref OCD_ZombieDirectorDeadEveronBaseState>();

	protected float MaxFloat(float left, float right)
	{
		if (left > right)
			return left;

		return right;
	}

	void Shutdown()
	{
		foreach (int callsign, OCD_ZombieDirectorDeadEveronBaseState baseState : m_mBaseStates)
		{
			if (baseState)
				baseState.Reset();
		}

		m_mBaseStates.Clear();
	}

	void RefreshZones(out notnull array<ref OCD_ZombieDirectorZoneDefinition> outZones)
	{
		outZones.Clear();

		SCR_MilitaryBaseSystem baseSystem = SCR_MilitaryBaseSystem.GetInstance();
		if (!baseSystem)
			return;

		array<SCR_MilitaryBaseComponent> bases = {};
		baseSystem.GetBases(bases);
		array<int> activeCallsigns = {};

		foreach (SCR_MilitaryBaseComponent baseComponent : bases)
		{
			if (!baseComponent)
				continue;

			int callsign = baseComponent.GetCallsign();
			if (callsign < 0 || !baseComponent.GetOwner())
				continue;

			activeCallsigns.Insert(callsign);
			GetOrCreateBaseState(baseComponent);

			OCD_ZombieDirectorZoneDefinition definition = BuildZoneForBase(baseComponent);
			if (definition)
				outZones.Insert(definition);

			AppendEncounterZones(baseComponent, outZones);
		}

		CleanupMissingStates(activeCallsigns);
	}

	void UpdateCapturePressure()
	{
		foreach (int callsign, OCD_ZombieDirectorDeadEveronBaseState baseState : m_mBaseStates)
		{
			if (!baseState)
				continue;

			baseState.Update();
		}
	}

	protected OCD_ZombieDirectorDeadEveronBaseState GetOrCreateBaseState(notnull SCR_MilitaryBaseComponent baseComponent)
	{
		int callsign = baseComponent.GetCallsign();
		OCD_ZombieDirectorDeadEveronBaseState baseState = m_mBaseStates.Get(callsign);
		if (!baseState)
		{
			baseState = new OCD_ZombieDirectorDeadEveronBaseState();
			m_mBaseStates.Set(callsign, baseState);
		}

		baseState.BindBase(baseComponent);
		return baseState;
	}

	protected void CleanupMissingStates(notnull array<int> activeCallsigns)
	{
		array<int> callsigns = {};
		foreach (int callsign, OCD_ZombieDirectorDeadEveronBaseState baseState : m_mBaseStates)
		{
			callsigns.Insert(callsign);
		}

		foreach (int callsign : callsigns)
		{
			if (activeCallsigns.Find(callsign) != -1)
				continue;

			OCD_ZombieDirectorDeadEveronBaseState baseState = m_mBaseStates.Get(callsign);
			if (baseState)
				baseState.Reset();

			m_mBaseStates.Remove(callsign);
		}
	}

	protected OCD_ZombieDirectorZoneDefinition BuildZoneForBase(notnull SCR_MilitaryBaseComponent baseComponent)
	{
		IEntity owner = baseComponent.GetOwner();
		if (!owner)
			return null;

		OCD_ZombieDirectorZoneDefinition definition = new OCD_ZombieDirectorZoneDefinition();
		definition.m_sZoneId = "dead_everon_base_" + baseComponent.GetCallsign();
		definition.m_vOrigin = owner.GetOrigin();
		definition.m_bAllowExternalHints = true;
		definition.m_bCycleWaypoints = true;

		vector forward;
		vector right;
		GetBaseAxes(owner, forward, right);

		float baseRadius = baseComponent.GetRadius();
		if (baseRadius <= 45.0)
		{
			definition.m_sTagsCsv = "checkpoint,roadblock,base";
			definition.m_fRadius = MaxFloat(72.0, baseRadius + 18.0);
			definition.m_fMinSpawnDistance = 24.0;
			definition.m_fMaxSpawnDistance = MaxFloat(60.0, baseRadius + 16.0);
			definition.m_fDespawnDistance = 180.0;
			definition.m_iLocalBudgetCap = 16;
			definition.m_fBaseCooldownSeconds = 40.0;
			definition.m_fAmbientWeight = 1.0;
			definition.m_fFieldWeight = 1.0;
			definition.m_fBaseWeight = 1.35;
			definition.m_sAllowedTemplateIdsCsv = "dead_everon_checkpoint_mix,dead_everon_tier1_sweep,dead_everon_quarantine_spike";
			AddCheckpointRoute(definition, owner.GetOrigin(), forward, right, MaxFloat(24.0, baseRadius));
			return definition;
		}

		definition.m_sTagsCsv = "base,roadblock,quarantine";
		definition.m_fRadius = MaxFloat(95.0, baseRadius + 24.0);
		definition.m_fMinSpawnDistance = 30.0;
		definition.m_fMaxSpawnDistance = MaxFloat(82.0, baseRadius + 22.0);
		definition.m_fDespawnDistance = 215.0;
		definition.m_iLocalBudgetCap = 18;
		definition.m_fBaseCooldownSeconds = 58.0;
		definition.m_fAmbientWeight = 1.05;
		definition.m_fFieldWeight = 0.75;
		definition.m_fBaseWeight = 1.55;
		definition.m_sAllowedTemplateIdsCsv = "dead_everon_checkpoint_mix,dead_everon_tier1_sweep,dead_everon_horde_push";
		AddPerimeterRoute(definition, owner.GetOrigin(), forward, right, MaxFloat(30.0, baseRadius));
		return definition;
	}

	protected void AppendEncounterZones(notnull SCR_MilitaryBaseComponent baseComponent, notnull array<ref OCD_ZombieDirectorZoneDefinition> outZones)
	{
		IEntity owner = baseComponent.GetOwner();
		if (!owner)
			return;

		vector forward;
		vector right;
		GetBaseAxes(owner, forward, right);
		float baseRadius = MaxFloat(24.0, baseComponent.GetRadius());

		OCD_ZombieDirectorZoneDefinition forestTrail = BuildForestTrailZone(baseComponent, owner.GetOrigin(), forward, right, baseRadius);
		if (forestTrail)
			outZones.Insert(forestTrail);

		if (baseRadius <= 45.0)
		{
			OCD_ZombieDirectorZoneDefinition checkpointGate = BuildCheckpointGateZone(baseComponent, owner.GetOrigin(), forward, right, baseRadius);
			if (checkpointGate)
				outZones.Insert(checkpointGate);

			return;
		}

		OCD_ZombieDirectorZoneDefinition villageEdge = BuildVillageEdgeZone(baseComponent, owner.GetOrigin(), forward, right, baseRadius);
		if (villageEdge)
			outZones.Insert(villageEdge);

		OCD_ZombieDirectorZoneDefinition labCorridor = BuildLabCorridorZone(baseComponent, owner.GetOrigin(), forward, right, baseRadius);
		if (labCorridor)
			outZones.Insert(labCorridor);
	}

	protected void GetBaseAxes(notnull IEntity owner, out vector forward, out vector right)
	{
		forward = OCD_ZombieDirectorMath.NormalizeOr(owner.GetWorldTransformAxis(2), "0 0 1");
		right = OCD_ZombieDirectorMath.NormalizeOr(owner.GetWorldTransformAxis(0), "1 0 0");
	}

	protected vector BuildOffsetPoint(vector origin, vector forward, vector right, float forwardMeters, float rightMeters)
	{
		return origin + (forward * forwardMeters) + (right * rightMeters);
	}

	protected void AddCheckpointRoute(notnull OCD_ZombieDirectorZoneDefinition definition, vector origin, vector forward, vector right, float radius)
	{
		definition.m_aSweepRoutePoints.Insert(BuildOffsetPoint(origin, forward, right, radius + 26.0, -10.0));
		definition.m_aSweepRoutePoints.Insert(BuildOffsetPoint(origin, forward, right, radius + 12.0, 0.0));
		definition.m_aSweepRoutePoints.Insert(BuildOffsetPoint(origin, forward, right, radius - 2.0, 10.0));
	}

	protected void AddPerimeterRoute(notnull OCD_ZombieDirectorZoneDefinition definition, vector origin, vector forward, vector right, float radius)
	{
		definition.m_aSweepRoutePoints.Insert(BuildOffsetPoint(origin, forward, right, radius + 16.0, radius * 0.45));
		definition.m_aSweepRoutePoints.Insert(BuildOffsetPoint(origin, forward, right, 4.0, radius * 0.7));
		definition.m_aSweepRoutePoints.Insert(BuildOffsetPoint(origin, forward, right, -radius * 0.55, radius * 0.35));
		definition.m_aSweepRoutePoints.Insert(BuildOffsetPoint(origin, forward, right, -radius * 0.7, -radius * 0.2));
	}

	protected OCD_ZombieDirectorZoneDefinition BuildCheckpointGateZone(notnull SCR_MilitaryBaseComponent baseComponent, vector origin, vector forward, vector right, float baseRadius)
	{
		OCD_ZombieDirectorZoneDefinition definition = new OCD_ZombieDirectorZoneDefinition();
		definition.m_sZoneId = "dead_everon_base_" + baseComponent.GetCallsign() + "_checkpoint_gate";
		definition.m_vOrigin = BuildOffsetPoint(origin, forward, right, baseRadius + 18.0, 0.0);
		definition.m_sTagsCsv = "checkpoint,roadblock,base";
		definition.m_fRadius = 42.0;
		definition.m_fMinSpawnDistance = 14.0;
		definition.m_fMaxSpawnDistance = 32.0;
		definition.m_fDespawnDistance = 125.0;
		definition.m_iLocalBudgetCap = 9;
		definition.m_fBaseCooldownSeconds = 22.0;
		definition.m_fAmbientWeight = 1.1;
		definition.m_fFieldWeight = 0.8;
		definition.m_fBaseWeight = 1.35;
		definition.m_sAllowedTemplateIdsCsv = "dead_everon_checkpoint_mix,dead_everon_quarantine_spike";
		definition.m_bAllowExternalHints = true;
		AddCheckpointRoute(definition, origin, forward, right, baseRadius);
		return definition;
	}

	protected OCD_ZombieDirectorZoneDefinition BuildVillageEdgeZone(notnull SCR_MilitaryBaseComponent baseComponent, vector origin, vector forward, vector right, float baseRadius)
	{
		OCD_ZombieDirectorZoneDefinition definition = new OCD_ZombieDirectorZoneDefinition();
		definition.m_sZoneId = "dead_everon_base_" + baseComponent.GetCallsign() + "_village_edge";
		definition.m_vOrigin = BuildOffsetPoint(origin, forward, right, baseRadius + 40.0, -baseRadius * 0.7);
		definition.m_sTagsCsv = "village,ruin,road";
		definition.m_fRadius = 76.0;
		definition.m_fMinSpawnDistance = 22.0;
		definition.m_fMaxSpawnDistance = 48.0;
		definition.m_fDespawnDistance = 160.0;
		definition.m_iLocalBudgetCap = 11;
		definition.m_fBaseCooldownSeconds = 24.0;
		definition.m_fAmbientWeight = 1.2;
		definition.m_fFieldWeight = 1.0;
		definition.m_fBaseWeight = 0.7;
		definition.m_sAllowedTemplateIdsCsv = "dead_everon_tier1_sweep,dead_everon_civilian_drifters";
		definition.m_bAllowExternalHints = true;
		definition.m_aSweepRoutePoints.Insert(BuildOffsetPoint(origin, forward, right, baseRadius + 58.0, -baseRadius * 1.05));
		definition.m_aSweepRoutePoints.Insert(BuildOffsetPoint(origin, forward, right, baseRadius + 34.0, -baseRadius * 0.65));
		definition.m_aSweepRoutePoints.Insert(BuildOffsetPoint(origin, forward, right, baseRadius + 12.0, -baseRadius * 0.2));
		return definition;
	}

	protected OCD_ZombieDirectorZoneDefinition BuildForestTrailZone(notnull SCR_MilitaryBaseComponent baseComponent, vector origin, vector forward, vector right, float baseRadius)
	{
		OCD_ZombieDirectorZoneDefinition definition = new OCD_ZombieDirectorZoneDefinition();
		definition.m_sZoneId = "dead_everon_base_" + baseComponent.GetCallsign() + "_forest_trail";
		definition.m_vOrigin = BuildOffsetPoint(origin, forward, right, -baseRadius - 34.0, baseRadius * 0.9);
		definition.m_sTagsCsv = "woods,anomaly,road";
		definition.m_fRadius = 82.0;
		definition.m_fMinSpawnDistance = 20.0;
		definition.m_fMaxSpawnDistance = 52.0;
		definition.m_fDespawnDistance = 170.0;
		definition.m_iLocalBudgetCap = 10;
		definition.m_fBaseCooldownSeconds = 26.0;
		definition.m_fAmbientWeight = 1.15;
		definition.m_fFieldWeight = 1.05;
		definition.m_fBaseWeight = 0.65;
		definition.m_sAllowedTemplateIdsCsv = "dead_everon_tier1_sweep,dead_everon_civilian_drifters";
		definition.m_bAllowExternalHints = true;
		definition.m_aSweepRoutePoints.Insert(BuildOffsetPoint(origin, forward, right, -baseRadius - 56.0, baseRadius * 1.1));
		definition.m_aSweepRoutePoints.Insert(BuildOffsetPoint(origin, forward, right, -baseRadius - 26.0, baseRadius * 0.75));
		definition.m_aSweepRoutePoints.Insert(BuildOffsetPoint(origin, forward, right, -8.0, baseRadius * 0.4));
		return definition;
	}

	protected OCD_ZombieDirectorZoneDefinition BuildLabCorridorZone(notnull SCR_MilitaryBaseComponent baseComponent, vector origin, vector forward, vector right, float baseRadius)
	{
		OCD_ZombieDirectorZoneDefinition definition = new OCD_ZombieDirectorZoneDefinition();
		definition.m_sZoneId = "dead_everon_base_" + baseComponent.GetCallsign() + "_lab_corridor";
		definition.m_vOrigin = BuildOffsetPoint(origin, forward, right, baseRadius * 0.35, baseRadius * 0.7);
		definition.m_sTagsCsv = "lab,quarantine,roadblock";
		definition.m_fRadius = 54.0;
		definition.m_fMinSpawnDistance = 12.0;
		definition.m_fMaxSpawnDistance = 28.0;
		definition.m_fDespawnDistance = 118.0;
		definition.m_iLocalBudgetCap = 12;
		definition.m_fBaseCooldownSeconds = 20.0;
		definition.m_fAmbientWeight = 1.15;
		definition.m_fFieldWeight = 0.6;
		definition.m_fBaseWeight = 1.2;
		definition.m_sAllowedTemplateIdsCsv = "dead_everon_quarantine_spike,dead_everon_checkpoint_mix,dead_everon_horde_push";
		definition.m_bAllowExternalHints = true;
		definition.m_aSweepRoutePoints.Insert(BuildOffsetPoint(origin, forward, right, baseRadius * 0.1, baseRadius * 0.95));
		definition.m_aSweepRoutePoints.Insert(BuildOffsetPoint(origin, forward, right, baseRadius * 0.35, baseRadius * 0.75));
		definition.m_aSweepRoutePoints.Insert(BuildOffsetPoint(origin, forward, right, baseRadius * 0.65, baseRadius * 0.55));
		return definition;
	}
}

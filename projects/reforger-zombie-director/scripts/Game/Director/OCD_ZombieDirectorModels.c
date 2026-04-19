[BaseContainerProps()]
class OCD_ZombieDirectorRestBridgeConfig
{
	[Attribute(defvalue: "0", desc: "Enable polling of external director hints. Leave disabled for standalone dedicated-server use or enable it through the profile-side override file")]
	bool m_bEnabled;

	[Attribute(defvalue: "http://127.0.0.1:18890/", desc: "REST base URL. Use loopback for same-host OpenClaw or HTTPS for remote dedicated-server control")]
	string m_sBaseUrl;

	[Attribute(defvalue: "reforger/zombie-director/hints", desc: "Route used to pull hint batches")]
	string m_sHintRoute;

	[Attribute(defvalue: "reforger/zombie-director/snapshot", desc: "Route used to publish snapshots")]
	string m_sSnapshotRoute;

	[Attribute(defvalue: "", desc: "Optional query-token appended as token=.... Strongly recommended when the bridge is not loopback-only")]
	string m_sToken;

	[Attribute(defvalue: "0.75", desc: "Hint poll period in seconds")]
	float m_fPollIntervalSeconds;

	[Attribute(defvalue: "1", desc: "Whether to POST snapshots back to the bridge")]
	bool m_bPostSnapshots;

	[Attribute(defvalue: "openclaw-zombie-director/rest-bridge.override.json", desc: "Optional override file path. Relative values resolve under $profile: so dedicated-server operators can change bridge settings without republishing the mission")]
	string m_sProfileOverridePath;

	void OCD_ZombieDirectorRestBridgeConfig()
	{
		m_bEnabled = false;
		m_sBaseUrl = "http://127.0.0.1:18890/";
		m_sHintRoute = "reforger/zombie-director/hints";
		m_sSnapshotRoute = "reforger/zombie-director/snapshot";
		m_fPollIntervalSeconds = 0.75;
		m_bPostSnapshots = true;
		m_sProfileOverridePath = "openclaw-zombie-director/rest-bridge.override.json";
	}

	string ResolveProfileOverridePath()
	{
		if (m_sProfileOverridePath.IsEmpty())
			return string.Empty;

		if (m_sProfileOverridePath.IndexOf(":") != -1)
			return m_sProfileOverridePath;

		if (m_sProfileOverridePath.IndexOf("/") == 0 || m_sProfileOverridePath.IndexOf("\\") == 0)
			return m_sProfileOverridePath;

		return "$profile:" + m_sProfileOverridePath;
	}

	void ApplyProfileOverride(notnull OCD_ZombieDirectorRestBridgeProfileOverride profileOverride)
	{
		if (profileOverride.enabled >= 0)
			m_bEnabled = profileOverride.enabled > 0;

		if (!profileOverride.baseUrl.IsEmpty())
			m_sBaseUrl = profileOverride.baseUrl;

		if (!profileOverride.hintRoute.IsEmpty())
			m_sHintRoute = profileOverride.hintRoute;

		if (!profileOverride.snapshotRoute.IsEmpty())
			m_sSnapshotRoute = profileOverride.snapshotRoute;

		if (profileOverride.clearToken)
			m_sToken = string.Empty;
		else if (!profileOverride.token.IsEmpty())
			m_sToken = profileOverride.token;

		if (profileOverride.pollIntervalSeconds > 0.0)
			m_fPollIntervalSeconds = profileOverride.pollIntervalSeconds;

		if (profileOverride.postSnapshots >= 0)
			m_bPostSnapshots = profileOverride.postSnapshots > 0;
	}
}

class OCD_ZombieDirectorRestBridgeProfileOverride : JsonApiStruct
{
	string kind;
	int version;
	int enabled;
	string baseUrl;
	string hintRoute;
	string snapshotRoute;
	string token;
	bool clearToken;
	float pollIntervalSeconds;
	int postSnapshots;

	void OCD_ZombieDirectorRestBridgeProfileOverride()
	{
		RegV("kind");
		RegV("version");
		RegV("enabled");
		RegV("baseUrl");
		RegV("hintRoute");
		RegV("snapshotRoute");
		RegV("token");
		RegV("clearToken");
		RegV("pollIntervalSeconds");
		RegV("postSnapshots");
	}

	bool IsValid()
	{
		return kind == "openclaw-zombie-director.rest-bridge-override" && version == 1;
	}
}

[BaseContainerProps()]
class OCD_ZombieUnitTemplateConfig
{
	[Attribute(defvalue: "z_walkers", desc: "Template identifier")]
	string m_sId;

	[Attribute(defvalue: "0", desc: "Spawn the group prefab instead of individual prefabs")]
	bool m_bSpawnAsGroupPrefab;

	[Attribute(defvalue: "", uiwidget: UIWidgets.ResourceNamePicker, desc: "Group prefab for authored sweeps")]
	ResourceName m_sGroupPrefab;

	[Attribute(desc: "Individual prefab list for pressure waves")]
	ref array<ResourceName> m_aMemberPrefabs = {};

	[Attribute(defvalue: "base,village,field", desc: "Preferred zone tags (CSV)")]
	string m_sPreferredZoneTagsCsv;

	[Attribute(defvalue: "3", desc: "Minimum members when spawning individuals")]
	int m_iMinMembers;

	[Attribute(defvalue: "6", desc: "Maximum members when spawning individuals")]
	int m_iMaxMembers;

	[Attribute(defvalue: "3", desc: "Budget cost per member or per group")]
	int m_iBudgetCost;

	[Attribute(defvalue: "1.0", desc: "Relative desirability weight")]
	float m_fWeight;

	void OCD_ZombieUnitTemplateConfig()
	{
		m_sId = "z_walkers";
		m_sPreferredZoneTagsCsv = "base,village,field";
		m_iMinMembers = 3;
		m_iMaxMembers = 6;
		m_iBudgetCost = 3;
		m_fWeight = 1.0;
	}

	void GetPreferredTags(notnull array<string> outTags)
	{
		OCD_ZombieDirectorCsv.Split(m_sPreferredZoneTagsCsv, outTags);
	}

	bool IsValid()
	{
		if (m_bSpawnAsGroupPrefab)
			return !m_sGroupPrefab.IsEmpty();

		return m_aMemberPrefabs && m_aMemberPrefabs.Count() > 0;
	}

	int RollMemberCount()
	{
		if (m_iMaxMembers <= m_iMinMembers)
			return m_iMinMembers;

		return Math.RandomInt(m_iMinMembers, m_iMaxMembers + 1);
	}

	ResourceName GetRandomMemberPrefab()
	{
		if (!m_aMemberPrefabs || m_aMemberPrefabs.IsEmpty())
			return ResourceName.Empty;

		return m_aMemberPrefabs[Math.RandomInt(0, m_aMemberPrefabs.Count())];
	}
}

class OCD_ZombieDirectorCsv
{
	static void Split(string csv, notnull array<string> outValues)
	{
		outValues.Clear();
		if (csv.IsEmpty())
			return;

		array<string> parts = {};
		csv.Split(",", parts, true);

		foreach (string part : parts)
		{
			string normalized = part;
			normalized.Replace(" ", "");
			normalized.ToLower();
			if (normalized.IsEmpty())
				continue;

			outValues.Insert(normalized);
		}
	}

	static bool Contains(notnull array<string> values, string wanted)
	{
		string normalized = wanted;
		normalized.Replace(" ", "");
		normalized.ToLower();

		foreach (string value : values)
		{
			if (value == normalized)
				return true;
		}

		return false;
	}

	static int CountMatches(notnull array<string> left, notnull array<string> right)
	{
		int score = 0;

		foreach (string candidate : left)
		{
			if (Contains(right, candidate))
				score++;
		}

		return score;
	}
}

class OCD_ZombieDirectorMath
{
	static float ClampFloat(float value, float minValue, float maxValue)
	{
		if (value < minValue)
			return minValue;
		if (value > maxValue)
			return maxValue;
		return value;
	}

	static int ClampInt(int value, int minValue, int maxValue)
	{
		if (value < minValue)
			return minValue;
		if (value > maxValue)
			return maxValue;
		return value;
	}

	static float MaxFloat(float left, float right)
	{
		if (left > right)
			return left;

		return right;
	}

	static float LerpFloat(float fromValue, float toValue, float alpha)
	{
		float clampedAlpha = ClampFloat(alpha, 0.0, 1.0);
		return fromValue + ((toValue - fromValue) * clampedAlpha);
	}

	static vector Flatten(vector input)
	{
		vector flat = input;
		flat[1] = 0;
		return flat;
	}

	static float Distance2D(vector left, vector right)
	{
		return Flatten(right - left).Length();
	}

	static vector NormalizeOr(vector input, vector fallback = "1 0 0")
	{
		vector flat = Flatten(input);
		if (flat.LengthSq() < 0.0001)
			return fallback;

		flat.Normalize();
		return flat;
	}

	static vector GetPerpendicular(vector input)
	{
		vector flat = NormalizeOr(input);
		return Vector(-flat[2], 0, flat[0]);
	}

	static vector BuildRingPoint(vector center, float minDistance, float maxDistance)
	{
		float angle = Math.RandomFloat01() * 6.2831855;
		float radius = minDistance + (Math.RandomFloat01() * (maxDistance - minDistance));
		vector offset = Vector(Math.Cos(angle) * radius, 0, Math.Sin(angle) * radius);
		return center + offset;
	}

	static vector BuildBiasedSpawnPoint(vector center, vector preferredDirection, float minDistance, float maxDistance, float lateralDistance)
	{
		vector forward = NormalizeOr(preferredDirection);
		vector right = GetPerpendicular(forward);
		float depth = minDistance + (Math.RandomFloat01() * (maxDistance - minDistance));
		float side = (Math.RandomFloat01() - 0.5) * 2.0 * lateralDistance;
		return center + (forward * depth) + (right * side);
	}

	static void BuildTransform(vector position, vector angles, out vector transform[4])
	{
		Math3D.AnglesToMatrix(angles, transform);
		transform[3] = position;
	}
}

class OCD_ZombieDirectorPlayerTrack
{
	int m_iPlayerId;
	string m_sName;
	vector m_vPosition;
	vector m_vPreviousPosition;
	vector m_vMoveDirection;
	bool m_bHasPosition;

	void UpdateFrom(IEntity entity, string playerName)
	{
		m_sName = playerName;
		m_vPreviousPosition = m_vPosition;
		m_vPosition = entity.GetOrigin();
		m_bHasPosition = true;

		vector delta = OCD_ZombieDirectorMath.Flatten(m_vPosition - m_vPreviousPosition);
		if (delta.LengthSq() > 4.0)
		{
			delta.Normalize();
			m_vMoveDirection = delta;
		}
	}
}

class OCD_ZombieDirectorZoneDefinition
{
	string m_sZoneId;
	vector m_vOrigin;
	string m_sTagsCsv;
	float m_fRadius;
	float m_fMinSpawnDistance;
	float m_fMaxSpawnDistance;
	float m_fDespawnDistance;
	int m_iLocalBudgetCap;
	float m_fBaseCooldownSeconds;
	float m_fAmbientWeight;
	float m_fFieldWeight;
	float m_fBaseWeight;
	string m_sAllowedTemplateIdsCsv;
	ref array<string> m_aSweepWaypointNames = {};
	ref array<vector> m_aSweepRoutePoints = {};
	bool m_bCycleWaypoints;
	bool m_bAllowExternalHints;

	void OCD_ZombieDirectorZoneDefinition()
	{
		m_sZoneId = string.Empty;
		m_vOrigin = "0 0 0";
		m_sTagsCsv = "base";
		m_fRadius = 100.0;
		m_fMinSpawnDistance = 35.0;
		m_fMaxSpawnDistance = 90.0;
		m_fDespawnDistance = 220.0;
		m_iLocalBudgetCap = 18;
		m_fBaseCooldownSeconds = 90.0;
		m_fAmbientWeight = 1.0;
		m_fFieldWeight = 1.0;
		m_fBaseWeight = 1.0;
		m_sAllowedTemplateIdsCsv = string.Empty;
		m_aSweepWaypointNames = {};
		m_aSweepRoutePoints = {};
		m_bCycleWaypoints = true;
		m_bAllowExternalHints = true;
	}

	string GetZoneId()
	{
		return m_sZoneId;
	}

	vector GetZoneOrigin()
	{
		return m_vOrigin;
	}

	float GetRadius()
	{
		return m_fRadius;
	}

	float GetMinSpawnDistance()
	{
		return m_fMinSpawnDistance;
	}

	float GetMaxSpawnDistance()
	{
		return m_fMaxSpawnDistance;
	}

	float GetDespawnDistance()
	{
		return m_fDespawnDistance;
	}

	int GetLocalBudgetCap()
	{
		return m_iLocalBudgetCap;
	}

	float GetBaseCooldownSeconds()
	{
		return m_fBaseCooldownSeconds;
	}

	float GetAmbientWeight()
	{
		return m_fAmbientWeight;
	}

	float GetFieldWeight()
	{
		return m_fFieldWeight;
	}

	float GetBaseWeight()
	{
		return m_fBaseWeight;
	}

	bool AllowsExternalHints()
	{
		return m_bAllowExternalHints;
	}

	bool ShouldCycleWaypoints()
	{
		return m_bCycleWaypoints;
	}

	void GetTags(notnull array<string> outTags)
	{
		OCD_ZombieDirectorCsv.Split(m_sTagsCsv, outTags);
	}

	void GetAllowedTemplateIds(notnull array<string> outTemplateIds)
	{
		OCD_ZombieDirectorCsv.Split(m_sAllowedTemplateIdsCsv, outTemplateIds);
	}

	void GetSweepWaypointNames(notnull array<string> outWaypointNames)
	{
		outWaypointNames.Clear();
		foreach (string waypointName : m_aSweepWaypointNames)
		{
			if (waypointName.IsEmpty())
				continue;

			outWaypointNames.Insert(waypointName);
		}
	}

	void GetSweepRoutePoints(notnull array<vector> outRoutePoints)
	{
		outRoutePoints.Clear();
		foreach (vector routePoint : m_aSweepRoutePoints)
			outRoutePoints.Insert(routePoint);
	}

	bool HasSweepRoutePoints()
	{
		return m_aSweepRoutePoints && !m_aSweepRoutePoints.IsEmpty();
	}

	bool HasTag(string tag)
	{
		array<string> tags = {};
		GetTags(tags);
		return OCD_ZombieDirectorCsv.Contains(tags, tag);
	}

	bool AllowsTemplate(string templateId)
	{
		array<string> allowed = {};
		GetAllowedTemplateIds(allowed);
		if (allowed.IsEmpty())
			return true;

		return OCD_ZombieDirectorCsv.Contains(allowed, templateId);
	}
}

class OCD_ZombieDirectorZoneRuntimeState
{
	string m_sZoneId;
	bool m_bSpawnPrimed;
	float m_fCooldownRemaining;
	float m_fLastScore;
	float m_fSmoothedScore;
	string m_sLastReason;
	int m_iActiveBudget;
	float m_fExternalWeight;
	float m_fExternalTtlSeconds;
	string m_sForcedTemplateId;
	int m_iRequestedBudget;
	bool m_bPlayerInside;
	bool m_bWasPlayerInside;
	float m_fPresenceSeconds;
	float m_fAbsenceSeconds;
	float m_fLastEntryAbsenceSeconds;
	float m_fMovementSeconds;
	float m_fBestApproach;
	float m_fEventCooldownRemaining;
	float m_fHintSuppressRemaining;
	string m_sLastHintSignature;
}

class OCD_ZombieDirectorActiveWave
{
	string m_sZoneId;
	string m_sTemplateId;
	int m_iBudgetCost;
	float m_fAgeSeconds;
	IEntity m_Entity;
	ref array<IEntity> m_aDynamicWaypointEntities = {};

	bool IsAlive()
	{
		return m_Entity && !m_Entity.IsDeleted();
	}

	vector GetOrigin()
	{
		if (!IsAlive())
			return "0 0 0";

		return m_Entity.GetOrigin();
	}

	void DeleteDynamicWaypointEntities()
	{
		if (!m_aDynamicWaypointEntities)
			return;

		foreach (IEntity waypointEntity : m_aDynamicWaypointEntities)
		{
			if (!waypointEntity || waypointEntity.IsDeleted())
				continue;

			SCR_EntityHelper.DeleteEntityAndChildren(waypointEntity);
		}

		m_aDynamicWaypointEntities.Clear();
	}
}

class OCD_ZombieDirectorHintJson : JsonApiStruct
{
	int seq;
	string type;
	string targetZoneId;
	string templateId;
	int requestedBudget;
	float weight;
	float ttlSeconds;
	string reason;
	string correlationId;
	vector anchor;

	void OCD_ZombieDirectorHintJson()
	{
		RegV("seq");
		RegV("type");
		RegV("targetZoneId");
		RegV("templateId");
		RegV("requestedBudget");
		RegV("weight");
		RegV("ttlSeconds");
		RegV("reason");
		RegV("correlationId");
		RegV("anchor");
	}
}

class OCD_ZombieDirectorHintBatchJson : JsonApiStruct
{
	string serverTime;
	ref array<ref OCD_ZombieDirectorHintJson> hints = {};

	void OCD_ZombieDirectorHintBatchJson()
	{
		RegV("serverTime");
		RegV("hints");
	}
}

class OCD_ZombieDirectorPlayerSnapshotJson : JsonApiStruct
{
	int playerId;
	string name;
	vector position;
	vector moveDir;

	void OCD_ZombieDirectorPlayerSnapshotJson()
	{
		RegV("playerId");
		RegV("name");
		RegV("position");
		RegV("moveDir");
	}
}

class OCD_ZombieDirectorZoneSnapshotJson : JsonApiStruct
{
	string zoneId;
	float score;
	float cooldownRemaining;
	int activeBudget;
	string reason;

	void OCD_ZombieDirectorZoneSnapshotJson()
	{
		RegV("zoneId");
		RegV("score");
		RegV("cooldownRemaining");
		RegV("activeBudget");
		RegV("reason");
	}
}

class OCD_ZombieDirectorSnapshotJson : JsonApiStruct
{
	float budgetCurrent;
	int budgetCap;
	int activeWaveCount;
	ref array<ref OCD_ZombieDirectorPlayerSnapshotJson> players = {};
	ref array<ref OCD_ZombieDirectorZoneSnapshotJson> zones = {};

	void OCD_ZombieDirectorSnapshotJson()
	{
		RegV("budgetCurrent");
		RegV("budgetCap");
		RegV("activeWaveCount");
		RegV("players");
		RegV("zones");
	}
}

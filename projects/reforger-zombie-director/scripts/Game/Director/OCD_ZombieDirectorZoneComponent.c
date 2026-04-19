[ComponentEditorProps(category: "OpenClaw/Zombie Director", description: "Spawn zone used by the OpenClaw zombie director", color: "220 70 70 255", visible: true)]
class OCD_ZombieDirectorZoneComponentClass : ScriptComponentClass
{
}

class OCD_ZombieDirectorZoneComponent : ScriptComponent
{
	protected static ref array<OCD_ZombieDirectorZoneComponent> s_aRegistry = new array<OCD_ZombieDirectorZoneComponent>();

	[Attribute(defvalue: "zone_alpha", desc: "Unique zone id")]
	protected string m_sZoneId;

	[Attribute(defvalue: "base", desc: "Zone tags (CSV). Examples: base,field,village,road,chokepoint")]
	protected string m_sTagsCsv;

	[Attribute(defvalue: "100", desc: "Zone radius in meters")]
	protected float m_fRadius;

	[Attribute(defvalue: "35", desc: "Minimum spawn distance from the zone center")]
	protected float m_fMinSpawnDistance;

	[Attribute(defvalue: "90", desc: "Maximum spawn distance from the zone center")]
	protected float m_fMaxSpawnDistance;

	[Attribute(defvalue: "220", desc: "Distance at which the director may despawn waves from this zone")]
	protected float m_fDespawnDistance;

	[Attribute(defvalue: "18", desc: "Maximum active budget this zone may hold")]
	protected int m_iLocalBudgetCap;

	[Attribute(defvalue: "90", desc: "Base cooldown after a successful spawn")]
	protected float m_fBaseCooldownSeconds;

	[Attribute(defvalue: "1.0", desc: "How strongly ambient pressure should value this zone")]
	protected float m_fAmbientWeight;

	[Attribute(defvalue: "1.0", desc: "Extra weight for open-field pressure")]
	protected float m_fFieldWeight;

	[Attribute(defvalue: "1.0", desc: "Extra weight for base reinforcement")]
	protected float m_fBaseWeight;

	[Attribute(defvalue: "walkers,ambush", desc: "Allowed template ids (CSV), empty means any")]
	protected string m_sAllowedTemplateIdsCsv;

	[Attribute(defvalue: "", desc: "Named world waypoints for authored sweeps (CSV)")]
	protected string m_sSweepWaypointNamesCsv;

	[Attribute(defvalue: "0 0 0", desc: "Dynamic sweep route point 1 (world coordinates)")]
	protected vector m_vSweepRoutePoint01;

	[Attribute(defvalue: "0 0 0", desc: "Dynamic sweep route point 2 (world coordinates)")]
	protected vector m_vSweepRoutePoint02;

	[Attribute(defvalue: "0 0 0", desc: "Dynamic sweep route point 3 (world coordinates)")]
	protected vector m_vSweepRoutePoint03;

	[Attribute(defvalue: "0 0 0", desc: "Dynamic sweep route point 4 (world coordinates)")]
	protected vector m_vSweepRoutePoint04;

	[Attribute(defvalue: "1", desc: "Cycle sweep waypoints if the spawned template is a group prefab")]
	protected bool m_bCycleWaypoints;

	[Attribute(defvalue: "1", desc: "Allow external hints from OpenClaw or local scripts")]
	protected bool m_bAllowExternalHints;

	override protected void OnPostInit(IEntity owner)
	{
		if (s_aRegistry.Find(this) == -1)
			s_aRegistry.Insert(this);
	}

	override protected void OnDelete(IEntity owner)
	{
		int index = s_aRegistry.Find(this);
		if (index != -1)
			s_aRegistry.RemoveOrdered(index);
	}

	static void GetRegistered(notnull array<OCD_ZombieDirectorZoneComponent> outZones)
	{
		outZones.Clear();
		foreach (OCD_ZombieDirectorZoneComponent zone : s_aRegistry)
		{
			if (!zone || !zone.GetOwner())
				continue;

			outZones.Insert(zone);
		}
	}

	string GetZoneId()
	{
		return m_sZoneId;
	}

	vector GetZoneOrigin()
	{
		return GetOwner().GetOrigin();
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
		if (m_sSweepWaypointNamesCsv.IsEmpty())
			return;

		m_sSweepWaypointNamesCsv.Split(",", outWaypointNames, true);
	}

	void GetSweepRoutePoints(notnull array<vector> outRoutePoints)
	{
		outRoutePoints.Clear();
		InsertRoutePoint(outRoutePoints, m_vSweepRoutePoint01);
		InsertRoutePoint(outRoutePoints, m_vSweepRoutePoint02);
		InsertRoutePoint(outRoutePoints, m_vSweepRoutePoint03);
		InsertRoutePoint(outRoutePoints, m_vSweepRoutePoint04);
	}

	protected void InsertRoutePoint(notnull array<vector> outRoutePoints, vector routePoint)
	{
		if (routePoint.LengthSq() <= 0.001)
			return;

		outRoutePoints.Insert(routePoint);
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

	OCD_ZombieDirectorZoneDefinition BuildDefinition()
	{
		OCD_ZombieDirectorZoneDefinition definition = new OCD_ZombieDirectorZoneDefinition();
		definition.m_sZoneId = m_sZoneId;
		definition.m_vOrigin = GetZoneOrigin();
		definition.m_sTagsCsv = m_sTagsCsv;
		definition.m_fRadius = m_fRadius;
		definition.m_fMinSpawnDistance = m_fMinSpawnDistance;
		definition.m_fMaxSpawnDistance = m_fMaxSpawnDistance;
		definition.m_fDespawnDistance = m_fDespawnDistance;
		definition.m_iLocalBudgetCap = m_iLocalBudgetCap;
		definition.m_fBaseCooldownSeconds = m_fBaseCooldownSeconds;
		definition.m_fAmbientWeight = m_fAmbientWeight;
		definition.m_fFieldWeight = m_fFieldWeight;
		definition.m_fBaseWeight = m_fBaseWeight;
		definition.m_sAllowedTemplateIdsCsv = m_sAllowedTemplateIdsCsv;
		definition.m_bCycleWaypoints = m_bCycleWaypoints;
		definition.m_bAllowExternalHints = m_bAllowExternalHints;

		array<string> waypointNames = {};
		GetSweepWaypointNames(waypointNames);
		foreach (string waypointName : waypointNames)
			definition.m_aSweepWaypointNames.Insert(waypointName);

		array<vector> routePoints = {};
		GetSweepRoutePoints(routePoints);
		foreach (vector routePoint : routePoints)
			definition.m_aSweepRoutePoints.Insert(routePoint);

		return definition;
	}
}

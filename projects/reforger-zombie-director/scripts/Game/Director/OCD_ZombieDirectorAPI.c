class OCD_ZombieDirectorAPI
{
	static void QueueLocalHint(
		string type,
		string targetZoneId = string.Empty,
		string templateId = string.Empty,
		int requestedBudget = -1,
		float weight = 1.0,
		string reason = string.Empty,
		float ttlSeconds = 30.0,
		vector anchor = "0 0 0",
		string correlationId = string.Empty
	)
	{
		OCD_ZombieDirectorGameModeComponent director = OCD_ZombieDirectorGameModeComponent.GetInstance();
		if (!director)
			return;

		OCD_ZombieDirectorHintJson hint = new OCD_ZombieDirectorHintJson();
		hint.type = type;
		hint.targetZoneId = targetZoneId;
		hint.templateId = templateId;
		hint.requestedBudget = requestedBudget;
		hint.weight = weight;
		hint.reason = reason;
		hint.ttlSeconds = ttlSeconds;
		hint.anchor = anchor;
		hint.correlationId = correlationId;

		director.EnqueueLocalHint(hint);
	}
}

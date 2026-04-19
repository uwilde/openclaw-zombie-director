class OCD_ZombieDirectorRestBridge
{
	protected ref OCD_ZombieDirectorRestBridgeConfig m_Config;
	protected RestContext m_Context;
	protected ref RestCallback m_HintsCallback;
	protected ref RestCallback m_SnapshotCallback;
	protected ref array<ref OCD_ZombieDirectorHintJson> m_aPendingHints = {};
	protected int m_iLastPulledSeq;
	protected string m_sLastError;

	void OCD_ZombieDirectorRestBridge(OCD_ZombieDirectorRestBridgeConfig config)
	{
		m_Config = config;
		if (m_Config && !m_Config.m_sBaseUrl.IsEmpty())
			m_Context = GetGame().GetRestApi().GetContext(m_Config.m_sBaseUrl);

		m_HintsCallback = new RestCallback();
		m_HintsCallback.SetOnSuccess(OnHintsPollSucceeded);
		m_HintsCallback.SetOnError(OnHintsPollFailed);

		m_SnapshotCallback = new RestCallback();
		m_SnapshotCallback.SetOnSuccess(OnSnapshotPostSucceededCallback);
		m_SnapshotCallback.SetOnError(OnSnapshotPostFailedCallback);
	}

	bool IsEnabled()
	{
		return m_Config && m_Config.m_bEnabled && m_Context;
	}

	string GetLastError()
	{
		return m_sLastError;
	}

	int GetLastPulledSeq()
	{
		return m_iLastPulledSeq;
	}

	void PollHints()
	{
		if (!IsEnabled())
			return;

		string request = m_Config.m_sHintRoute + "?afterSeq=" + m_iLastPulledSeq;
		if (!m_Config.m_sToken.IsEmpty())
			request = request + "&token=" + m_Config.m_sToken;

		m_Context.GET(m_HintsCallback, request);
	}

	void PublishSnapshot(notnull OCD_ZombieDirectorSnapshotJson snapshot)
	{
		if (!IsEnabled() || !m_Config.m_bPostSnapshots)
			return;

		snapshot.Pack();

		string request = m_Config.m_sSnapshotRoute;
		if (!m_Config.m_sToken.IsEmpty())
			request = request + "?token=" + m_Config.m_sToken;

		m_Context.POST(m_SnapshotCallback, request, snapshot.AsString());
	}

	void OnHintsPollSucceeded(RestCallback callback)
	{
		OCD_ZombieDirectorHintBatchJson batch = new OCD_ZombieDirectorHintBatchJson();
		if (callback)
		{
			string data = callback.GetData();
			if (!data.IsEmpty())
				batch.ExpandFromRAW(data);
		}

		OnHintPullSucceeded(batch);
	}

	void OnHintsPollFailed(RestCallback callback)
	{
		OnHintPullFailed(FormatRestFailure(callback));
	}

	void OnSnapshotPostSucceededCallback(RestCallback callback)
	{
		OnSnapshotPostSucceeded();
	}

	void OnSnapshotPostFailedCallback(RestCallback callback)
	{
		OnSnapshotPostFailed(FormatRestFailure(callback));
	}

	protected string FormatRestFailure(RestCallback callback)
	{
		if (!callback)
			return "request_failed";

		int restResult = callback.GetRestResult();
		int httpCode = callback.GetHttpCode();
		return "rest=" + restResult + " http=" + httpCode;
	}

	void OnHintPullSucceeded(notnull OCD_ZombieDirectorHintBatchJson batch)
	{
		m_sLastError = string.Empty;

		foreach (OCD_ZombieDirectorHintJson hint : batch.hints)
		{
			if (!hint)
				continue;

			if (hint.seq > m_iLastPulledSeq)
				m_iLastPulledSeq = hint.seq;

			m_aPendingHints.Insert(hint);
		}
	}

	void OnHintPullFailed(string reason)
	{
		m_sLastError = reason;
	}

	void OnSnapshotPostSucceeded()
	{
		m_sLastError = string.Empty;
	}

	void OnSnapshotPostFailed(string reason)
	{
		m_sLastError = reason;
	}

	int ConsumeHints(out notnull array<ref OCD_ZombieDirectorHintJson> outHints)
	{
		outHints = m_aPendingHints;
		m_aPendingHints = {};
		return outHints.Count();
	}
}

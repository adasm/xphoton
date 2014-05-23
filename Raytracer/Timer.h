#pragma once

typedef struct Timer
{
	inline u32 init()
	{
		m_frameID = 0;
		if (!QueryPerformanceFrequency(&m_ticsPerSecond ))return 0;
		else{ QueryPerformanceCounter(&m_appStartTime); m_startTime = m_appStartTime; return 1; }
	}
	inline void  update()
	{
		LARGE_INTEGER currentTime;
		QueryPerformanceCounter(&currentTime);
		m_lastInterval = ((f32)currentTime.QuadPart - (f32)m_startTime.QuadPart) / (f32)m_ticsPerSecond.QuadPart * 1000;
		m_startTime = currentTime;
		m_frameID++;
	}
	inline f32 get() { return m_lastInterval; }
	inline f32 getCurrent()
	{
		LARGE_INTEGER currentTime;
		QueryPerformanceCounter(&currentTime);
		m_frameStartTime = (m_startTime.QuadPart) / (f32)m_ticsPerSecond.QuadPart * 1000;
		return ((f32)currentTime.QuadPart - (f32)m_startTime.QuadPart) / (f32)m_ticsPerSecond.QuadPart * 1000;
	}
	inline f32 getFrameTime()
	{
		return m_frameStartTime;
	}
	inline u32 getFrameID()
	{
		return m_frameID;
	}
	inline f32 getAppTime()
	{
		LARGE_INTEGER currentTime;
		QueryPerformanceCounter(&currentTime);
		return ((f32)currentTime.QuadPart - (f32)m_appStartTime.QuadPart) / (f32)m_ticsPerSecond.QuadPart * 1000;
	}

protected:
	LARGE_INTEGER m_ticsPerSecond;
	LARGE_INTEGER m_startTime;
	f32			  m_lastInterval;
	f32			  m_frameStartTime;
	LARGE_INTEGER m_appStartTime;
	u32			  m_frameID;
}*Timer_ptr;
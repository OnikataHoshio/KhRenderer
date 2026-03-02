#pragma once

class KH_Timer
{
public:
	KH_Timer() = default;
	KH_Timer(float IntervalSeconds);
	~KH_Timer() = default;

	void Reset();

	void Active();

	void InActive();

	void Tick(float deltaTime);

	bool HasFinished() const;

private:
	float Interval = 3.0;
	float RemainingTime = 3.0;
	bool bIsTriggered = false;
	bool bIsActive = false;
};
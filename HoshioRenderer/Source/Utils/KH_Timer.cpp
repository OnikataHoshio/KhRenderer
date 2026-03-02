#include "KH_Timer.h"

#include "Editor/KH_Editor.h"

KH_Timer::KH_Timer(float IntervalSeconds)
	: Interval(IntervalSeconds), RemainingTime(IntervalSeconds) {
}


void KH_Timer::Reset()
{
	RemainingTime = Interval;
	bIsTriggered = false;
	Active();
}

void KH_Timer::Active()
{
	bIsActive = true;
}

void KH_Timer::InActive()
{
	bIsActive = false;
}

void KH_Timer::Tick(float deltaTime)
{
	if (!bIsActive || bIsTriggered)
		return;

	if (RemainingTime > 0.0f) {
		RemainingTime -= deltaTime;
		if (RemainingTime <= 0.0f) {
			bIsTriggered = true;
		}
	}
}

bool KH_Timer::HasFinished() const
{
	return bIsTriggered && bIsActive;
}


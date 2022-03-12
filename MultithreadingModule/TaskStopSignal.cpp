#include "TaskStopSignal.h"

TaskStopSignal::TaskStopSignal() : bState(false)
{
}

void TaskStopSignal::SetState(bool bNewState)
{
	StateMutex.lock();
	bState = bNewState;
	StateMutex.unlock();
}

bool TaskStopSignal::GetState() const
{
	std::lock_guard<std::mutex> Lock(StateMutex);
	return bState;
}

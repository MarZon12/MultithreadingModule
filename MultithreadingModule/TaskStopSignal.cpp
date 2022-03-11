#include "TaskStopSignal.h"

TaskStopSignal::TaskStopSignal() : State(false)
{
}

void TaskStopSignal::SetState(bool NewState)
{
	StateMutex.lock();
	State = NewState;
	StateMutex.unlock();
}

bool TaskStopSignal::GetState() const
{
	std::lock_guard<std::mutex> Lock(StateMutex);
	return State;
}

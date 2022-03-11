#pragma once
#include "TaskStopSignal.h"

class MultithreadingInterface final
{
public:
	virtual ~MultithreadingInterface() = 0;

	virtual void ThreadExecute() = 0;
	virtual void ThreadCallback() = 0;

	virtual void ThreadExecuteTick(const float& DeltaTime) = 0;
	virtual void ThreadCallbackTick() = 0;

	virtual void ThreadExecuteDedicated(const TaskStopSignal& StopSignal) = 0;
	virtual void ThreadCallbackDedicated() = 0;
};

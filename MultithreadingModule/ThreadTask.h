#pragma once
#include <mutex>
#include "MultithreadingInterface.h"
#include "TaskRepeatability.h"
#include "TaskStopSignal.h"


class ThreadTask
{
private:
	bool bNeedCallback;
	std::mutex NeedCallbackMutex;

	TaskRepeatability Repeatability;
	std::mutex RepeatabilityMutex;

	bool bExecuteOnDedicatedThread;
	std::mutex ExecuteOnDedicatedThreadMutex;

protected:
	TaskStopSignal ExecutionStopSignal;

public:
	// Callback = false, Repeatability = Once, OnDedicated = false
	ThreadTask() : bNeedCallback(false), Repeatability(TaskRepeatability::Once), bExecuteOnDedicatedThread(false) {};
	ThreadTask(bool bNewTaskNeedCallback, TaskRepeatability NewTaskRepeatability, bool bOnDedicated):
		bNeedCallback(bNewTaskNeedCallback), Repeatability(NewTaskRepeatability), bExecuteOnDedicatedThread(bOnDedicated) {};
	
	virtual ~ThreadTask() {}


	virtual void Execute(const float& DeltaTime) = 0;

	virtual void StopDedicatedExecution() final;

	
	virtual bool GetNeedCallback() final;

	virtual TaskRepeatability GetRepeatability() final;

	virtual bool GetExecuteOnDedicatedThread() final;
};

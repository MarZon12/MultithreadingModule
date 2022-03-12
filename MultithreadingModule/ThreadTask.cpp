#include "ThreadTask.h"

void ThreadTask::StopDedicatedExecution()
{
	ExecutionStopSignal.SetState(true);
}

bool ThreadTask::GetNeedCallback()
{
	std::lock_guard<std::mutex> Lock(NeedCallbackMutex);
	return bNeedCallback;
}

TaskRepeatability ThreadTask::GetRepeatability()
{
	std::lock_guard<std::mutex> Lock(RepeatabilityMutex);
	return Repeatability;
}

bool ThreadTask::GetExecuteOnDedicatedThread()
{
	std::lock_guard<std::mutex> Lock(ExecuteOnDedicatedThreadMutex);
	return bExecuteOnDedicatedThread;
}

#include "ThreadInterfaceTask.h"

void ThreadInterfaceTask::Execute(const float& DeltaTime)
{
	std::lock_guard<std::mutex> ObjectLock(ObjectMutex);

	if (Object == nullptr)
	{
		return;
	}

	if (GetExecuteOnDedicatedThread())
	{
		Object->ThreadExecuteDedicated(ExecutionStopSignal);
		if (GetNeedCallback())
		{
			Object->ThreadCallbackDedicated();
		}

		return;
	}

	if (GetRepeatability() == TaskRepeatability::Once)
	{
		Object->ThreadExecute();
		if (GetNeedCallback())
		{
			Object->ThreadCallback();
		}

		return;
	}

	if (GetRepeatability() == TaskRepeatability::EveryTick)
	{
		Object->ThreadExecuteTick(DeltaTime);
		if (GetNeedCallback())
		{
			Object->ThreadCallbackTick();
		}
	}
}

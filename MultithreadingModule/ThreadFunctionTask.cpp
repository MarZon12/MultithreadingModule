#include "ThreadFunctionTask.h"

void ThreadFunctionTask::Execute(const float& DeltaTime)
{
	std::lock_guard<std::mutex> LockFunction(FunctionMutex);
	std::lock_guard<std::mutex> LockCallback(CallbackFunctionMutex);

	if (GetExecuteOnDedicatedThread())
	{
		if (!DedicatedFunction)
		{
			return;
		}

		if (GetNeedCallback())
		{
			if (!CallbackFunction)
			{
				return;
			}
		}

		DedicatedFunction(ExecutionStopSignal);

		if (GetNeedCallback())
		{
			CallbackFunction();
		}

		return;
	}

	if (GetRepeatability() == TaskRepeatability::Once)
	{
		if (!Function)
		{
			return;
		}

		if (GetNeedCallback())
		{
			if (!CallbackFunction)
			{
				return;
			}
		}

		Function();

		if (GetNeedCallback())
		{
			CallbackFunction();
		}

		return;
	}

	if (GetRepeatability() == TaskRepeatability::EveryTick)
	{
		if (!TickFunction)
		{
			return;
		}

		if (GetNeedCallback())
		{
			if (!CallbackFunction)
			{
				return;
			}
		}

		TickFunction(DeltaTime);

		if (GetNeedCallback())
		{
			CallbackFunction();
		}
	}
}

#pragma once
#include <functional>
#include "ThreadTask.h"


class ThreadFunctionTask final : public ThreadTask
{
private:
	std::function<void()> Function;
	std::function<void(const float& DeltaTime)> TickFunction;
	std::function<void(const TaskStopSignal& StopSignal)> DedicatedFunction;

	std::function<void()> CallbackFunction;

	std::mutex FunctionMutex;
	std::mutex CallbackFunctionMutex;

public:
	ThreadFunctionTask() = delete;

	// Callback = false, Repeatability = Once, OnDedicated = false
	ThreadFunctionTask(std::function<void()> NewFunction) :
		ThreadTask(false, TaskRepeatability::Once, false), Function(NewFunction) {}
	// Callback = true, Repeatability = Once, OnDedicated = false
	ThreadFunctionTask(std::function<void()> NewFunction, std::function<void()> NewCallbackFunction) :
		ThreadTask(true, TaskRepeatability::Once, false), Function(NewFunction), CallbackFunction(NewCallbackFunction) {}

	// Callback = false, Repeatability = Once, OnDedicated = true
	ThreadFunctionTask(std::function<void(const TaskStopSignal& StopSignal)> NewDedicatedFunction) :
		ThreadTask(false, TaskRepeatability::Once, true), DedicatedFunction(NewDedicatedFunction) {}
	// Callback = true, Repeatability = Once, OnDedicated = true
	ThreadFunctionTask(std::function<void(const TaskStopSignal& StopSignal)> NewDedicatedFunction, std::function<void()> NewCallbackFunction) :
		ThreadTask(true, TaskRepeatability::Once, true), DedicatedFunction(NewDedicatedFunction), CallbackFunction(NewCallbackFunction) {}
	
	// Callback = false, Repeatability = Tick, OnDedicated = false
	ThreadFunctionTask(std::function<void(const float& DeltaTime)> NewTickFunction) :
		ThreadTask(false, TaskRepeatability::EveryTick, false), TickFunction(NewTickFunction) {}
	// Callback = true, Repeatability = Tick, OnDedicated = false
	ThreadFunctionTask(std::function<void(const float& DeltaTime)> NewTickFunction, std::function<void()> NewCallbackFunction) :
		ThreadTask(true, TaskRepeatability::EveryTick, false), TickFunction(NewTickFunction), CallbackFunction(NewCallbackFunction) {}
	

	void Execute(const float& DeltaTime) override {
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
	};
};

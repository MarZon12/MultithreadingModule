#pragma once
#include <functional>
#include "ThreadTask.h"

template<typename Class>
class ThreadMethodTask final : public ThreadTask
{
private:
	Class* Obj;

	std::function<void(Class&)> Method;
	std::function<void(Class&, const float& DeltaTime)> TickMethod;
	std::function<void(Class&, const TaskStopSignal& StopSignal)> DedicatedMethod;

	std::function<void(Class&)> CallbackMethod;

	std::mutex MethodMutex;
	std::mutex CallbackMethodMutex;
public:
	ThreadMethodTask() = delete;

	// Callback = false, Repeatability = Once, OnDedicated = false
	ThreadMethodTask(Class* Object, std::function<void(Class&)> NewMethod) :
		ThreadTask(false, TaskRepeatability::Once, false), Method(NewMethod), Obj(Object) {}
	// Callback = true, Repeatability = Once, OnDedicated = false
	ThreadMethodTask(Class* Object, std::function<void(Class&)> NewMethod, std::function<void(Class&)> NewCallbackMethod) :
		ThreadTask(true, TaskRepeatability::Once, false), Method(NewMethod), CallbackMethod(NewCallbackMethod), Obj(Object) {}

	// Callback = false, Repeatability = Once, OnDedicated = true
	ThreadMethodTask(Class* Object, std::function<void(Class&, const TaskStopSignal& StopSignal)> NewDedicatedMethod) :
		ThreadTask(false, TaskRepeatability::Once, true), DedicatedMethod(NewDedicatedMethod), Obj(Object) {}
	// Callback = true, Repeatability = Once, OnDedicated = true
	ThreadMethodTask(Class* Object, std::function<void(Class&, const TaskStopSignal& StopSignal)> NewDedicatedMethod, std::function<void(Class&)> NewCallbackMethod) :
		ThreadTask(true, TaskRepeatability::Once, true), DedicatedMethod(NewDedicatedMethod), CallbackMethod(NewCallbackMethod), Obj(Object) {}

	// Callback = false, Repeatability = Tick, OnDedicated = false
	ThreadMethodTask(Class* Object, std::function<void(Class&, const float& DeltaTime)> NewTickMethod) :
		ThreadTask(false, TaskRepeatability::EveryTick, false), TickMethod(NewTickMethod), Obj(Object) {}
	// Callback = true, Repeatability = Tick, OnDedicated = false
	ThreadMethodTask(Class* Object, std::function<void(Class&, const float& DeltaTime)> NewTickMethod, std::function<void(Class&)> NewCallbackMethod) :
		ThreadTask(true, TaskRepeatability::EveryTick, false), TickMethod(NewTickMethod), CallbackMethod(NewCallbackMethod), Obj(Object) {}

	void Execute(const float& DeltaTime) override;
};

template<typename Class>
inline void ThreadMethodTask<Class>::Execute(const float& DeltaTime)
{
	std::lock_guard<std::mutex> LockFunction(MethodMutex);
	std::lock_guard<std::mutex> LockCallback(CallbackMethodMutex);

	if (GetExecuteOnDedicatedThread())
	{
		if (!DedicatedMethod)
		{
			return;
		}

		if (GetNeedCallback())
		{
			if (!CallbackMethod)
			{
				return;
			}
		}

		DedicatedMethod(*Obj, ExecutionStopSignal);

		if (GetNeedCallback())
		{
			CallbackMethod(*Obj);
		}

		return;
	}

	if (GetRepeatability() == TaskRepeatability::Once)
	{
		if (!Method)
		{
			return;
		}

		if (GetNeedCallback())
		{
			if (!CallbackMethod)
			{
				return;
			}
		}

		Method(*Obj);

		if (GetNeedCallback())
		{
			CallbackMethod(*Obj);
		}

		return;
	}

	if (GetRepeatability() == TaskRepeatability::EveryTick)
	{
		if (!TickMethod)
		{
			return;
		}

		if (GetNeedCallback())
		{
			if (!CallbackMethod)
			{
				return;
			}
		}

		TickMethod(*Obj, DeltaTime);

		if (GetNeedCallback())
		{
			CallbackMethod(*Obj);
		}
	}
}

#pragma once
#include "ThreadTask.h"

class ThreadInterfaceTask final : public ThreadTask
{
private:
	MultithreadingInterface* Object;
	std::mutex ObjectMutex;

public:
	ThreadInterfaceTask() = delete;

	// Callback = false, Repeatability = Once, OnDedicated = false
	ThreadInterfaceTask(MultithreadingInterface* NewTaskObject) : ThreadTask(), Object(NewTaskObject) {}
	// Repeatability = Once
	ThreadInterfaceTask(MultithreadingInterface* NewTaskObject, bool bNewTaskNeedCallback,  bool bOnDedicated) :
		ThreadTask(bNewTaskNeedCallback, TaskRepeatability::Once, bOnDedicated), Object(NewTaskObject) {}
	// Repeatability = Tick, OnDedicated = false
	ThreadInterfaceTask(MultithreadingInterface* NewTaskObject, bool bNewTaskNeedCallback) :
		ThreadTask(bNewTaskNeedCallback, TaskRepeatability::EveryTick, false), Object(NewTaskObject) {}

	void Execute(const float& DeltaTime) override;
};

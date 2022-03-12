#include "MultithreadingModule.h"

unsigned int MultithreadingModule::MultithreadingManagerRefCounter = 0;
std::mutex MultithreadingModule::MultithreadingManagerRefCounterMutex;

MultithreadingManager* MultithreadingModule::MultithreadingManagerRef = nullptr;
std::mutex MultithreadingModule::MultithreadingManagerRefMutex;

void MultithreadingModule::IncreaseRefCounter()
{
	std::lock_guard<std::mutex> Lock(MultithreadingManagerRefCounterMutex);
	MultithreadingManagerRefCounter++;
}

void MultithreadingModule::DecreaseRefCounter()
{
	std::lock_guard<std::mutex> Lock(MultithreadingManagerRefCounterMutex);
	MultithreadingManagerRefCounter--;
}

unsigned int MultithreadingModule::GetRefCounterValue()
{
	std::lock_guard<std::mutex> Lock(MultithreadingManagerRefCounterMutex);
	return MultithreadingManagerRefCounter;
}

MultithreadingModule::MultithreadingModule()
{
	IncreaseRefCounter();

	std::lock_guard<std::mutex> Lock(MultithreadingManagerRefMutex);
	if (MultithreadingManagerRef == nullptr)
	{
		MultithreadingManagerRef = new MultithreadingManager();
	}
}

MultithreadingModule::~MultithreadingModule()
{
	DecreaseRefCounter();

	std::lock_guard<std::mutex> Lock(MultithreadingManagerRefMutex);
	if (GetRefCounterValue() == 0)
	{
		delete MultithreadingManagerRef;
		MultithreadingManagerRef = nullptr;
	}
}

void MultithreadingModule::Tick(float DeltaTime)
{
	MultithreadingManagerRef->Tick(DeltaTime);
}

void MultithreadingModule::StartThreads()
{
	MultithreadingManagerRef->StartThreads();
}

void MultithreadingModule::StopThreads()
{
	MultithreadingManagerRef->StopThreads();
}

void MultithreadingModule::ChangeNumOfRunningThreads(unsigned int NewNumOfStandardThreads)
{
	MultithreadingManagerRef->ChangeNumOfRunningThreads(NewNumOfStandardThreads);
}

std::vector<ThreadState> MultithreadingModule::GetThreadsStates()
{
	return MultithreadingManagerRef->GetThreadsStates();
}

unsigned int MultithreadingModule::GetNumOfThreads()
{
	return MultithreadingManagerRef->GetNumOfThreads();
}

void MultithreadingModule::SetMaxNumOfThreads(unsigned int NewMax)
{
	MultithreadingManagerRef->SetMaxNumOfThreads(NewMax);
}

unsigned int MultithreadingModule::GetMaxNumOfThreads()
{
	return MultithreadingManagerRef->GetMaxNumOfThreads();
}

void MultithreadingModule::SetMaxNumOfStoppedThreads(unsigned int NewMax)
{
	MultithreadingManagerRef->SetMaxNumOfStoppedThreads(NewMax);
}

unsigned int MultithreadingModule::GetMaxNumOfStoppedThreads()
{
	return MultithreadingManagerRef->GetMaxNumOfStoppedThreads();
}

void MultithreadingModule::AddTask(ThreadTask* Task)
{
	MultithreadingManagerRef->AddTask(Task);
}

void MultithreadingModule::RemoveTask(ThreadTask* Task)
{
	MultithreadingManagerRef->RemoveTask(Task);
}

void MultithreadingModule::RemoveAllTasks()
{
	MultithreadingManagerRef->RemoveAllTasks();
}

void MultithreadingModule::RemoveAllTickTasks()
{
	MultithreadingManagerRef->RemoveAllTickTasks();
}

void MultithreadingModule::RemoveAllOnceTasks()
{
	MultithreadingManagerRef->RemoveAllOnceTasks();
}

void MultithreadingModule::StopDedicatedThreads()
{
	MultithreadingManagerRef->StopDedicatedThreads();
}


std::vector<ThreadState> MultithreadingModule::GetDedicatedThreadsStates()
{
	return MultithreadingManagerRef->GetDedicatedThreadsStates();
}

void MultithreadingModule::SetMaxTickTasksPerIteration(unsigned int NewMax)
{
	AdvancedThread::SetMaxTickTasksPerIteration(NewMax);
}

unsigned int MultithreadingModule::GetMaxTickTasksPerIteration()
{
	return AdvancedThread::GetMaxTickTasksPerIteration();
}

void MultithreadingModule::SetMaxOnceTasksPerIteration(unsigned int NewMax)
{
	AdvancedThread::SetMaxOnceTasksPerIteration(NewMax);
}

unsigned int MultithreadingModule::GetMaxOnceTasksPerIteration()
{
	return AdvancedThread::GetMaxOnceTasksPerIteration();
}

#include "MultithreadingManager.h"

unsigned int MultithreadingManager::MaxNumOfThreads = 4;
std::mutex MultithreadingManager::MaxNumOfThreadsMutex;

unsigned int MultithreadingManager::MaxNumOfStoppedThreads = 8;
std::mutex MultithreadingManager::MaxNumOfStoppedThreadsMutex;



MultithreadingManager::MultithreadingManager() : ThreadsManager(nullptr), NumOfThreads(0), DeltaTime(0.0f), bThreadCompletedTickTasks(false)
{
	// Starting Threads Manager
	ThreadMethodTask<MultithreadingManager>* ThreadsManagerTask = new ThreadMethodTask<MultithreadingManager>(this, &MultithreadingManager::ThreadsManagerExecution);
	ThreadsManager = new AdvancedThread();
	ThreadsManager->Initialize(ThreadsManagerTask);
	ThreadsManager->Start();
}

MultithreadingManager::~MultithreadingManager()
{
	StopThreads();
	StopDedicatedThreads();

	delete ThreadsManager;

	RemoveAllTasks();
}


void MultithreadingManager::Tick(float DeltaTime)
{
	SetTickDeltaTime(DeltaTime);

	std::unique_lock<std::mutex> LockTickTasks(TickTasksMutex);
	
	// If a request was made to execute Tick tasks, but there are no such tasks, then we stop the execution
	if (TickTasks.empty())
	{
		return;
	}

	// Copy all tasks to the execution queue
	std::unique_lock<std::mutex> LockTickTasksForExecution(TickTasksForExecutionMutex);
	for (size_t i = 0; i < TickTasks.size(); i++)
	{
		TickTasksForExecution.push(TickTasks[i]);
	}
	LockTickTasks.unlock();
	LockTickTasksForExecution.unlock();


	std::vector<AdvancedThread*> ExecutingThreads;

	// Telling all threads to execute Tick tasks
	std::unique_lock<std::mutex> LockStandardWorkers(StandardWorkersMutex);
	for (size_t i = 0; i < StandardWorkers.size(); i++)
	{
		StandardWorkers[i]->NotifyTickTaskAvailable();

		StandardWorkers[i]->SetCanBeDestroyed(false);
		ExecutingThreads.push_back(StandardWorkers[i]);
	}
	LockStandardWorkers.unlock();

	// Waiting for end of execution
	while (true)
	{
		bool bExit = false;

		// Sleep until the thread reports the completion of the task, 
		// wake up every 500 ms and additionally check this
		std::unique_lock<std::mutex> LockThreadCompletedTickTasks(ThreadCompletedTickTasksMutex);
		while (!bThreadCompletedTickTasks)
		{
			ThreadCompletedTickTasksCondition.wait_for(LockThreadCompletedTickTasks, std::chrono::milliseconds(500));
		}

		// Reset the state for the next thread
		bThreadCompletedTickTasks = false;
		LockThreadCompletedTickTasks.unlock();


		// As soon as the thread has reported that it has completed the assigned tasks, 
		// it is necessary to check whether all threads have completed work on Tick-type tasks
		for (size_t i = 0; i < ExecutingThreads.size(); i++)
		{
			if (!ExecutingThreads[i]->GetThreadCompletedTick())
			{
				break;
			}

			if (i == ExecutingThreads.size() - 1)
			{
				bExit = true;
			}
		}

		if (bExit)
		{
			for (size_t i = 0; i < ExecutingThreads.size(); i++)
			{
				ExecutingThreads[i]->SetCanBeDestroyed(true);
			}

			break;
		}
	}
}

void MultithreadingManager::StartThreads()
{
	std::unique_lock<std::mutex> Lock(StandardWorkersMutex);

	if (!StandardWorkers.empty())
	{
		return;
	}

	Lock.unlock();

	for (size_t i = 0; i < GetMaxNumOfThreads(); i++)
	{
		StartNewThread();
	}
}

void MultithreadingManager::StopThreads()
{
	while (GetNumOfThreads() != 0)
	{
		StopOneThread();
	}
}

void MultithreadingManager::StopThreadsWithWaiting()
{
	while (GetNumOfThreads() != 0)
	{
		StopOneThreadWithWaiting();
	}
}

void MultithreadingManager::StopDedicatedThreads()
{
	std::lock_guard<std::mutex> LockDedicatedWorkers(DedicatedWorkersMutex);
	std::lock_guard<std::mutex> LockPendingStopWorkers(PendingStopWorkersMutex);
	while (DedicatedWorkers.size() != 0)
	{
		DedicatedWorkers.back()->Stop();
		PendingStopWorkers.push_back(DedicatedWorkers.back());
		DedicatedWorkers.pop_back();
	}
}

void MultithreadingManager::StopDedicatedThreadsWithWaiting()
{
	std::lock_guard<std::mutex> LockDedicatedWorkers(DedicatedWorkersMutex);
	std::lock_guard<std::mutex> LockPendingStopWorkers(PendingStopWorkersMutex);
	while (DedicatedWorkers.size() != 0)
	{
		DedicatedWorkers.back()->StopWithWaiting();
		PendingStopWorkers.push_back(DedicatedWorkers.back());
		DedicatedWorkers.pop_back();
	}
}

std::vector<ThreadState> MultithreadingManager::GetDedicatedThreadsStates()
{
	std::vector<ThreadState> States;

	DedicatedWorkersMutex.lock();
	for (size_t i = 0; i < DedicatedWorkers.size(); i++)
	{
		States.push_back(DedicatedWorkers[i]->GetState());
	}
	DedicatedWorkersMutex.unlock();

	return States;
}

void MultithreadingManager::ChangeNumOfRunningThreads(unsigned int NewNumOfStandardThreads)
{
	if (NewNumOfStandardThreads == GetNumOfThreads())
	{
		return;
	}

	if (NewNumOfStandardThreads < GetNumOfThreads())
	{
		// Stopping
		while (NewNumOfStandardThreads < GetNumOfThreads())
		{
			StopOneThread();
		}
	}
	else
	{
		if (NewNumOfStandardThreads > GetMaxNumOfThreads())
		{
			NewNumOfStandardThreads = GetMaxNumOfThreads();
		}

		// Starting
		while (NewNumOfStandardThreads > GetNumOfThreads())
		{
			StartNewThread();
		}
	}
}

std::vector<ThreadState> MultithreadingManager::GetThreadsStates()
{
	std::vector<ThreadState> States;

	StandardWorkersMutex.lock();
	for (size_t i = 0; i < StandardWorkers.size(); i++)
	{
		States.push_back(StandardWorkers[i]->GetState());
	}
	StandardWorkersMutex.unlock();

	return States;
}

unsigned int MultithreadingManager::GetNumOfThreads()
{
	std::unique_lock<std::mutex> Lock(NumOfThreadsMutex);
	return NumOfThreads;
}

void MultithreadingManager::SetMaxNumOfThreads(unsigned int NewMax)
{
	std::unique_lock<std::mutex> Lock(MaxNumOfThreadsMutex);
	
	if (NewMax <= 1)
	{
		MaxNumOfThreads = 1;
	}
	else
	{
		MaxNumOfThreads = NewMax;
	}
}

unsigned int MultithreadingManager::GetMaxNumOfThreads()
{
	std::unique_lock<std::mutex> Lock(MaxNumOfThreadsMutex);
	return MaxNumOfThreads;
}


void MultithreadingManager::SetMaxNumOfStoppedThreads(unsigned int NewMax)
{
	std::unique_lock<std::mutex> Lock(MaxNumOfStoppedThreadsMutex);
	MaxNumOfStoppedThreads = NewMax;
}

unsigned int MultithreadingManager::GetMaxNumOfStoppedThreads()
{
	std::unique_lock<std::mutex> Lock(MaxNumOfStoppedThreadsMutex);
	return MaxNumOfStoppedThreads;
}

void MultithreadingManager::AddTask(ThreadTask* Task)
{
	if (Task == nullptr)
	{
		return;
	}

	if (Task->GetExecuteOnDedicatedThread())
	{
		StartDedicatedThread(Task);
		return;
	}

	switch (Task->GetRepeatability())
	{
	case TaskRepeatability::Once:
		AddOnceTask(Task);
		break;

	case TaskRepeatability::EveryTick:
		AddTickTask(Task);
		break;

	default:
		break;
	}
	
}

void MultithreadingManager::RemoveTask(ThreadTask* Task)
{
	if (Task == nullptr)
	{
		return;
	}

	if (Task->GetExecuteOnDedicatedThread())
	{
		return;
	}

	switch (Task->GetRepeatability())
	{
	case TaskRepeatability::Once:
		return;
		break;

	case TaskRepeatability::EveryTick:
		RemoveTickTask(Task);
		break;

	default:
		break;
	}
}

void MultithreadingManager::StartDedicatedThread(ThreadTask* Task)
{
	AdvancedThread* NewThread = new AdvancedThread();
	NewThread->Initialize(Task);
	NewThread->Start();

	std::lock_guard<std::mutex> LockDedicatedWorkers(DedicatedWorkersMutex);
	DedicatedWorkers.push_back(NewThread);
}

unsigned int MultithreadingManager::GetNumOfDedicatedThreads()
{
	std::unique_lock<std::mutex> Lock(DedicatedWorkersMutex);
	return DedicatedWorkers.size();
}

void MultithreadingManager::RemoveAllTasks()
{
	RemoveAllOnceTasks();
	RemoveAllTickTasks();
}

void MultithreadingManager::RemoveAllTickTasks()
{
	std::unique_lock<std::mutex> Lock(TickTasksMutex);
	while (!TickTasks.empty())
	{
		delete TickTasks.back();
		TickTasks.pop_back();
	}
}

void MultithreadingManager::RemoveAllOnceTasks()
{
	std::unique_lock<std::mutex> Lock(OnceTasksMutex);
	while (!OnceTasks.empty())
	{
		delete OnceTasks.front();
		OnceTasks.pop();
	}
}

float MultithreadingManager::GetTickDeltaTime()
{
	std::unique_lock<std::mutex> Lock(DeltaTimeMutex);
	return DeltaTime;
}

void MultithreadingManager::SetTickDeltaTime(float ActualDeltaTime)
{
	std::unique_lock<std::mutex> Lock(DeltaTimeMutex);
	DeltaTime = ActualDeltaTime;
}

void MultithreadingManager::UpdateNumOfThreads()
{
	std::unique_lock<std::mutex> Lock(NumOfThreadsMutex);
	std::unique_lock<std::mutex> LockStandardWorkers(StandardWorkersMutex);
	NumOfThreads = StandardWorkers.size();
}

void MultithreadingManager::StopOneThread()
{
	bool bOneThreadStopped = false;

	std::unique_lock<std::mutex> LockStandardWorkers(StandardWorkersMutex);
	
	if (StandardWorkers.empty())
	{
		return;
	}

	for (size_t i = 0; i < StandardWorkers.size(); i++)
	{
		if (StandardWorkers[i]->GetState() == ThreadState::Asleep || StandardWorkers[i]->GetState() == ThreadState::NotReadyToStart || StandardWorkers[i]->GetState() == ThreadState::Stopped)
		{
			StandardWorkers[i]->Stop();

			std::lock_guard<std::mutex> LockPendingStopWorkers(PendingStopWorkersMutex);
			PendingStopWorkers.push_back(StandardWorkers[i]);

			StandardWorkers.erase(StandardWorkers.begin() + i);
			bOneThreadStopped = true;
			break;
		}
	}

	if (!bOneThreadStopped)
	{
		StandardWorkers.back()->Stop();
		std::lock_guard<std::mutex> LockPendingStopWorkers(PendingStopWorkersMutex);
		PendingStopWorkers.push_back(StandardWorkers.back());
		StandardWorkers.pop_back();
	}

	LockStandardWorkers.unlock();
	UpdateNumOfThreads();
}

void MultithreadingManager::StopOneThreadWithWaiting()
{
	std::unique_lock<std::mutex> LockStandardWorkers(StandardWorkersMutex);

	if (StandardWorkers.empty())
	{
		return;
	}


	bool bOneThreadStopped = false;

	for (size_t i = 0; i < StandardWorkers.size(); i++)
	{
		if (StandardWorkers[i]->GetState() == ThreadState::Asleep || StandardWorkers[i]->GetState() == ThreadState::NotReadyToStart || StandardWorkers[i]->GetState() == ThreadState::Stopped)
		{
			StandardWorkers[i]->StopWithWaiting();

			std::unique_lock<std::mutex> LockPendingStopWorkers(PendingStopWorkersMutex);
			PendingStopWorkers.push_back(StandardWorkers[i]);

			StandardWorkers.erase(StandardWorkers.begin() + i);
			bOneThreadStopped = true;
			break;
		}
	}

	if (!bOneThreadStopped)
	{
		StandardWorkers.back()->StopWithWaiting();

		std::unique_lock<std::mutex> LockPendingStopWorkers(PendingStopWorkersMutex);
		PendingStopWorkers.push_back(StandardWorkers.back());

		StandardWorkers.pop_back();
	}

	LockStandardWorkers.unlock();
	UpdateNumOfThreads();
}

void MultithreadingManager::StartNewThread()
{
	AdvancedThread* StartedThread = nullptr;

	std::unique_lock<std::mutex> LockStandardWorkers(StandardWorkersMutex);

	if (StandardWorkers.size() >= GetMaxNumOfThreads())
	{
		return;
	}

	if ((StartedThread = GetStoppedThread()) == nullptr)
	{
		StartedThread = new AdvancedThread();
	}

	StartedThread->Initialize(&OnceTasks, &OnceTasksMutex,
		&TickTasksForExecution, &TickTasksForExecutionMutex,
		&bThreadCompletedTickTasks, &ThreadCompletedTickTasksMutex, &ThreadCompletedTickTasksCondition,
		&DeltaTime, &DeltaTimeMutex);
	StartedThread->Start();
	StandardWorkers.push_back(StartedThread);

	LockStandardWorkers.unlock();

	UpdateNumOfThreads();
}

AdvancedThread* MultithreadingManager::GetStoppedThread()
{
	AdvancedThread* StoppedThread = nullptr;

	std::unique_lock<std::mutex> LockStoppedWorkers(StoppedWorkersMutex);

	if (!StoppedWorkers.empty())
	{
		StoppedThread = StoppedWorkers.back();
		StoppedWorkers.pop_back();
	}

	return StoppedThread;
}

void MultithreadingManager::AddOnceTask(ThreadTask* Task)
{
	std::unique_lock<std::mutex> Lock(OnceTasksMutex);
	OnceTasks.push(Task);
}

void MultithreadingManager::AddTickTask(ThreadTask* Task)
{
	std::unique_lock<std::mutex> Lock(TickTasksMutex);
	TickTasks.push_back(Task);
}

void MultithreadingManager::RemoveTickTask(ThreadTask* Task)
{
	if (Task == nullptr)
	{
		return;
	}

	std::unique_lock<std::mutex> Lock(TickTasksMutex);
	for (size_t i = 0; i < TickTasks.size(); i++)
	{
		if (Task == TickTasks[i])
		{
			delete TickTasks[i];
			TickTasks.erase(TickTasks.begin() + i);
			return;
		}
	}
}

void MultithreadingManager::ThreadsManagerExecution(const TaskStopSignal& StopSignal)
{
	while (true)
	{
		bool bManagerCanFinishWork = true;

		// Dedicated threads: move to the list of pending stop those that have completed their work
		std::unique_lock<std::mutex> LockDedicatedWorkers(DedicatedWorkersMutex);
		for (size_t i = 0; i < DedicatedWorkers.size(); i++)
		{
			if (DedicatedWorkers[i]->GetState() == ThreadState::Stopped || DedicatedWorkers[i]->GetState() == ThreadState::NotReadyToStart)
			{
				std::lock_guard<std::mutex> LockPendingStopWorkers(PendingStopWorkersMutex);
				PendingStopWorkers.push_back(DedicatedWorkers[i]);

				DedicatedWorkers.erase(DedicatedWorkers.begin() + i);
			}
		}
		LockDedicatedWorkers.unlock();

		// Standard threads: move to the list of pending stop those that have completed their work
		bool bNeedToUpdateNumOfThreads = false;
		std::unique_lock<std::mutex> LockStandardWorkers(StandardWorkersMutex);
		for (size_t i = 0; i < StandardWorkers.size(); i++)
		{
			if (StandardWorkers[i]->GetState() == ThreadState::Stopped || StandardWorkers[i]->GetState() == ThreadState::NotReadyToStart)
			{
				std::lock_guard<std::mutex> LockPendingStopWorkers(PendingStopWorkersMutex);
				PendingStopWorkers.push_back(StandardWorkers[i]);

				StandardWorkers.erase(StandardWorkers.begin() + i);
				bNeedToUpdateNumOfThreads = true;
			}
		}
		LockStandardWorkers.unlock();
		if (bNeedToUpdateNumOfThreads)
		{
			UpdateNumOfThreads();
		}

		// Pending stop threads: move to the list of stopped threads or destroy those that have completed work
		std::unique_lock<std::mutex> LockPendingStopWorkers(PendingStopWorkersMutex);
		for (size_t i = 0; i < PendingStopWorkers.size(); i++)
		{
			// If the thread is marked as indestructible, then skip it
			if (!PendingStopWorkers[i]->GetCanBeDestroyed())
			{
				continue;
			}

			if (PendingStopWorkers[i]->GetState() == ThreadState::Stopped || PendingStopWorkers[i]->GetState() == ThreadState::NotReadyToStart)
			{
				std::lock_guard<std::mutex> LockStoppedWorkers(StoppedWorkersMutex);

				if (StoppedWorkers.size() < GetMaxNumOfStoppedThreads())
				{
					PendingStopWorkers[i]->Deinitialize();
					StoppedWorkers.push_back(PendingStopWorkers[i]);
				}
				else
				{
					delete PendingStopWorkers[i];
				}
				PendingStopWorkers.erase(PendingStopWorkers.begin() + i);
			}
		}
		LockPendingStopWorkers.unlock();

		if (StopSignal.GetState())
		{
			std::lock_guard<std::mutex> LockStoppedWorkers(StoppedWorkersMutex);
			while (!StoppedWorkers.empty())
			{
				delete StoppedWorkers.back();
				StoppedWorkers.pop_back();
			}
		}


		// If there are still running, stopping, or stopped threads, then shutting down the manager is unacceptable
		if (GetNumOfThreads() != 0 || GetNumOfDedicatedThreads() != 0 || GetNumOfPendingStopThreads() != 0 || GetNumOfStoppedThreads() != 0)
		{
			bManagerCanFinishWork = false;
		}

		if (StopSignal.GetState() && bManagerCanFinishWork)
		{
			break;
		}

		if (!StopSignal.GetState())
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}
}

unsigned int MultithreadingManager::GetNumOfPendingStopThreads()
{
	std::unique_lock<std::mutex> Lock(PendingStopWorkersMutex);
	return PendingStopWorkers.size();
}

unsigned int MultithreadingManager::GetNumOfStoppedThreads()
{
	std::unique_lock<std::mutex> Lock(StoppedWorkersMutex);
	return StoppedWorkers.size();
}
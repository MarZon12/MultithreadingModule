#pragma once
#include <vector>
#include <queue>
#include "AdvancedThread.h"
#include "ThreadTask.h"
#include "ThreadMethodTask.h"

class MultithreadingModule;

class MultithreadingManager final
{
private:
	// Handler thread that helps in managing threads
	AdvancedThread* ThreadsManager;


	// Standard threads (for Once and Tick tasks)
	std::vector<AdvancedThread*> StandardWorkers;
	std::mutex StandardWorkersMutex;

	// Number of running standart threads
	unsigned int NumOfThreads;
	std::mutex NumOfThreadsMutex;

	static unsigned int MaxNumOfThreads;
	static std::mutex MaxNumOfThreadsMutex;


	// Dedicated threads for dedicated tasks
	std::vector<AdvancedThread*> DedicatedWorkers;
	std::mutex DedicatedWorkersMutex;


	// Threads that terminate and require further processing
	std::vector<AdvancedThread*> PendingStopWorkers;
	std::mutex PendingStopWorkersMutex;

	// Saved stopped threads that are ready to init and start
	std::vector<AdvancedThread*> StoppedWorkers;
	std::mutex StoppedWorkersMutex;

	static unsigned int MaxNumOfStoppedThreads;
	static std::mutex MaxNumOfStoppedThreadsMutex;


	


	std::queue<ThreadTask*> OnceTasks;
	std::mutex OnceTasksMutex;

	std::queue<ThreadTask*> TickTasksForExecution;
	std::mutex TickTasksForExecutionMutex;

	std::vector<ThreadTask*> TickTasks;
	std::mutex TickTasksMutex;


	float DeltaTime;
	std::mutex DeltaTimeMutex;

	bool bThreadCompletedTickTasks;
	std::mutex ThreadCompletedTickTasksMutex;
	std::condition_variable ThreadCompletedTickTasksCondition;

	
private:
	MultithreadingManager();
	~MultithreadingManager();

	friend MultithreadingModule;
public:
	// Standard threads

	// Causes threads to perform Tick tasks and waits until they are completed
	// @param DeltaTime - Execution time of the previous Tick
	void Tick(float DeltaTime);

	// Prepares and starts the maximum number of standard threads
	// Only works if no standard thread is running
	void StartThreads();

	// Stops all standard threads
	void StopThreads();
	// Stops all standard threads and waits until it completes
	void StopThreadsWithWaiting();

	// Changes the number of running standard threads, stopping or starting them, bringing their number to the given number 
	// Will not exceed the maximum number of standart threads limit
	// @param NewNumOfStandardThreads - Updated number of working standart threads
	void ChangeNumOfRunningThreads(unsigned int NewNumOfStandardThreads);

	// Returns the states of standard threads
	std::vector<ThreadState> GetThreadsStates();

	// Returns the number of running standard threads
	unsigned int GetNumOfThreads();

	// Sets the maximum number of simultaneously working standard threads
	// @param NewMax - Updated limit
	static void SetMaxNumOfThreads(unsigned int NewMax);
	// Returns the maximum number of simultaneously working standard threads
	static unsigned int GetMaxNumOfThreads();

	// Sets the maximum number of saved stopped standard threads
	// @param NewMax - Updated limit
	static void SetMaxNumOfStoppedThreads(unsigned int NewMax);
	// Returns the maximum number of stored standard threads
	static unsigned int GetMaxNumOfStoppedThreads();

	// Adds a task to execute
	// @param Task - Task to add
	void AddTask(ThreadTask* Task);
	// Removes Tick task from execution
	// @param Task - Task for removal
	void RemoveTask(ThreadTask* Task);

	// Removes all tasks from execution
	void RemoveAllTasks();
	// Removes all Tick tasks from execution
	void RemoveAllTickTasks();
	// Removes all Once tasks from execution
	void RemoveAllOnceTasks();
	

	// Dedicated threads

	// Stops all dedicated threads
	void StopDedicatedThreads();
	// Stops all dedicated threads and waits until it completes
	void StopDedicatedThreadsWithWaiting();

	// Returns the states of dedicated threads
	std::vector<ThreadState> GetDedicatedThreadsStates();

private:
	// Methods for standart threads

	float GetTickDeltaTime();
	void SetTickDeltaTime(float ActualDeltaTime);

	void UpdateNumOfThreads();

	void StopOneThread();
	void StopOneThreadWithWaiting();

	void StartNewThread();

	AdvancedThread* GetStoppedThread();

	void AddOnceTask(ThreadTask* Task);
	void AddTickTask(ThreadTask* Task);

	void RemoveTickTask(ThreadTask* Task);

private:
	// Methods for dedicated threads
	
	void StartDedicatedThread(ThreadTask* Task);

	unsigned int GetNumOfDedicatedThreads();

private:
	// Methods for Threads Manager

	void ThreadsManagerExecution(const TaskStopSignal& StopSignal);

	unsigned int GetNumOfPendingStopThreads();

	unsigned int GetNumOfStoppedThreads();
};

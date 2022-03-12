#pragma once
#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include "ThreadState.h"
#include "ThreadTask.h"

class AdvancedThread final
{
private:
	// All types

	std::thread* ControlledThread;
	std::mutex ControlledThreadMutex;

	bool bCanBeDestroyed;
	std::mutex CanBeDestroyedMutex;

	bool bMustStop;
	std::mutex MustStopMutex;

	bool bMustSleep;
	std::mutex MustSleepMutex;
	std::condition_variable MustSleepCondition;

	ThreadState State;
	std::mutex StateMutex;


	// Dedicated type

	bool bIsDedicatedThread;
	std::mutex IsDedicatedThreadMutex;

	ThreadTask* TaskForDedicatedExecution;
	std::mutex TaskForDedicatedExecutionMutex;


	// Standard type

	bool bThreadCompletedTick;
	std::mutex ThreadCompletedTickMutex;

	static unsigned int MaxTickTasksPerIteration;
	static std::mutex MaxTickTasksPerIterationMutex;

	static unsigned int MaxOnceTasksPerIteration;
	static std::mutex MaxOnceTasksPerIterationMutex;


	// Standard type: External Data

	std::queue<ThreadTask*>* OnceTasksRef;
	std::mutex* OnceTasksMutexRef;

	std::queue<ThreadTask*>* TickTasksRef;
	std::mutex* TickTasksMutexRef;

	bool* bThreadCompletedTickTasksRef;
	std::mutex* ThreadCompletedTickTasksMutexRef;
	std::condition_variable* ThreadCompletedTickTasksConditionRef;

	float* DeltaTickRef;
	std::mutex* DeltaTickMutexRef;

public:
	AdvancedThread();
	~AdvancedThread();

	// Initialize as standart thread
	// @param OnceTasks - pointer to Once task list
	// @param OnceTasksMutex - pointer to corresponding mutex
	// @param TickTasks - pointer to Tick task list
	// @param OnceTasksMutex - pointer to corresponding mutex
	// @param ThreadCompletedTickTasks - pointer to a boolean variable that indicates that the thread has finished working on Tick tasks
	// @param ThreadCompletedTickTasksMutex - pointer to corresponding mutex
	// @param ThreadCompletedTickTasksCondition - pointer to a condition variable that will be notified when the thread has finished working on Tick tasks
	// @param DeltaTick - pointer to a variable that stores the actual execution time of the previous Tick
	// @param DeltaTickMutex - pointer to corresponding mutex
	void Initialize(
		std::queue<ThreadTask*>* OnceTasks, std::mutex* OnceTasksMutex,
		std::queue<ThreadTask*>* TickTasks, std::mutex* TickTasksMutex,
		bool* bThreadCompletedTickTasks, std::mutex* ThreadCompletedTickTasksMutex, std::condition_variable* ThreadCompletedTickTasksCondition,
		float* DeltaTick, std::mutex* DeltaTickMutex);
	// Initialize as dedicated thread
	// @param Task - pointer to a task for dedicated execution
	void Initialize(ThreadTask* Task);


	// All types

	// Returns the current state of the thread
	ThreadState GetState();

	// Returns true if the thread is running as dedicated
	bool IsDedicated();

	// Runs a new thread
	void Start();
	// Tells the thread to terminate
	void Stop();
	// Tells a thread to terminate and waits until it completes
	void StopWithWaiting();

	// Prepares a thread for restart
	// It must be executed if the thread is completely stopped and it is planned to start it again
	void Deinitialize();

	// Marks the thread as impossible/possible for destruction (does not affect anything, only describes the state)
	// @param NewState - new state value
	void SetCanBeDestroyed(bool bNewState);
	// Returns the state of the possibility of destroying the thread
	bool GetCanBeDestroyed();


	// Standard type

	// Returns true if the thread completed Tick tasks
	bool GetThreadCompletedTick();
	// Notifies the thread that there are available Tick tasks
	void NotifyTickTaskAvailable();

	// Sets the maximum number of Tick tasks to be executed in one iteration
	// @param NewMax - Updated limit
	static void SetMaxTickTasksPerIteration(unsigned int NewMax);
	// Returns the maximum number of Tick tasks that the thread executes in one iteration
	static unsigned int GetMaxTickTasksPerIteration();


	// Sets the maximum number of Once tasks to be executed in one iteration
	// @param NewMax - Updated limit
	static void SetMaxOnceTasksPerIteration(unsigned int NewMax);
	// Returns the maximum number of Once tasks that the thread executes in one iteration
	static unsigned int GetMaxOnceTasksPerIteration();

private:
	// All types

	void SetState(ThreadState NewState);

	void SetIsDedicated(bool bNewState);

	bool GetMustStop();
	bool GetMustSleep();

	void Sleep();
	void WakeUp();

	// Suspends execution of the current thread until the thread is notified to wake up or it determines on its own that it needs to
	void SleepExecution();
	

	// Standard type

	void Execute();

	void SetThreadCompletedTick(bool bNewState);

	// Works with an external object
	bool GetNeedToCompleteOnceTasks();

	// Works with an external object
	void NotifyManagerThreadCompletedTickTasks();


	// Dedicated type

	void ExecuteDedicated();
};


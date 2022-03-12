#pragma once
#include "AdvancedThread.h"
#include "MultithreadingManager.h"

class MultithreadingModule final
{
private:
	static unsigned int MultithreadingManagerRefCounter;
	static std::mutex MultithreadingManagerRefCounterMutex;

	static MultithreadingManager* MultithreadingManagerRef;
	static std::mutex MultithreadingManagerRefMutex;

private:
	static void IncreaseRefCounter();
	static void DecreaseRefCounter();
	static unsigned int GetRefCounterValue();

public:
	MultithreadingModule();
	~MultithreadingModule();

	void* operator new(std::size_t count) = delete;
	void* operator new[](std::size_t count) = delete;



	// Standard threads

	// Causes threads to perform Tick tasks and waits until they are completed
	// @param DeltaTime - Execution time of the previous Tick
	void Tick(float DeltaTime);

	// Prepares and starts the maximum number of standard threads
	// Only works if no standard thread is running
	void StartThreads();

	// Stops all standard threads
	void StopThreads();
	// Deprecated
	// Stops all standard threads and waits until it completes
	// void StopThreadsWithWaiting(); 
	
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
	// Deprecated
	// Stops all dedicated threads and waits until it completes
	// void StopDedicatedThreadsWithWaiting();
	
	// Returns the states of dedicated threads
	std::vector<ThreadState> GetDedicatedThreadsStates();

public:
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
};

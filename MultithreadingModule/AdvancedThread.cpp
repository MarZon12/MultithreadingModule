#include "AdvancedThread.h"

unsigned int AdvancedThread::MaxTickTasksPerIteration = 2000;
std::mutex AdvancedThread::MaxTickTasksPerIterationMutex;
unsigned int AdvancedThread::MaxOnceTasksPerIteration = 1;
std::mutex AdvancedThread::MaxOnceTasksPerIterationMutex;

AdvancedThread::AdvancedThread() :
    ControlledThread(nullptr),
    CanBeDestroyed(true),
    IsDedicatedThread(false),
    TaskForDedicatedExecution(nullptr),
    MustStop(false), MustSleep(false),
    State(ThreadState::NotReadyToStart),
    ThreadCompletedTick(false),
    OnceTasksRef(nullptr), OnceTasksMutexRef(nullptr),
    TickTasksRef(nullptr), TickTasksMutexRef(nullptr),
    ThreadCompletedTickTasksRef(nullptr), ThreadCompletedTickTasksMutexRef(nullptr), ThreadCompletedTickTasksConditionRef(nullptr),
    DeltaTickRef(nullptr), DeltaTickMutexRef(nullptr) {}

AdvancedThread::~AdvancedThread()
{
    StopWithWaiting();
}

void AdvancedThread::Initialize(std::queue<ThreadTask*>* OnceTasks, std::mutex* OnceTasksMutex,
    std::queue<ThreadTask*>* TickTasks, std::mutex* TickTasksMutex,
    bool* ThreadCompletedTickTasks, std::mutex* ThreadCompletedTickTasksMutex, std::condition_variable* ThreadCompletedTickTasksCondition,
    float* DeltaTick, std::mutex* DeltaTickMutex)
{
    // If the thread is running, then we forbid initialization
    if (!(GetState() == ThreadState::Stopped || GetState() == ThreadState::NotReadyToStart))
    {
        return;
    }

    if (OnceTasks == nullptr || OnceTasksMutex == nullptr)
    {
        return;
    }
    if (TickTasks == nullptr || TickTasksMutex == nullptr)
    {
        return;
    }
    if (ThreadCompletedTickTasks == nullptr || ThreadCompletedTickTasksMutex == nullptr || ThreadCompletedTickTasksCondition == nullptr)
    {
        return;
    }
    if (DeltaTick == nullptr || DeltaTickMutex == nullptr)
    {
        return;
    }
    
    if (ControlledThread != nullptr)
    {
        ControlledThread->join();
        delete ControlledThread;
        ControlledThread = nullptr;
    }

    OnceTasksRef = OnceTasks;
    OnceTasksMutexRef = OnceTasksMutex;

    TickTasksRef = TickTasks;
    TickTasksMutexRef = TickTasksMutex;
    
    ThreadCompletedTickTasksRef = ThreadCompletedTickTasks;
    ThreadCompletedTickTasksMutexRef = ThreadCompletedTickTasksMutex;
    ThreadCompletedTickTasksConditionRef = ThreadCompletedTickTasksCondition;

    DeltaTickRef = DeltaTick;
    DeltaTickMutexRef = DeltaTickMutex;

    SetIsDedicated(false);
    SetState(ThreadState::ReadyToStart);
}

void AdvancedThread::Initialize(ThreadTask* Task)
{
    // If the thread is running, then we forbid initialization
    if (!(GetState() == ThreadState::Stopped || GetState() == ThreadState::NotReadyToStart))
    {
        return;
    }

    if (Task == nullptr)
    {
        return;
    }

    if (ControlledThread != nullptr)
    {
        ControlledThread->join();
        delete ControlledThread;
        ControlledThread = nullptr;
    }

    std::lock_guard<std::mutex> LockTask(TaskForDedicatedExecutionMutex);
    TaskForDedicatedExecution = Task;

    SetIsDedicated(true);
    SetState(ThreadState::ReadyToStart);
}

ThreadState AdvancedThread::GetState()
{
    std::lock_guard<std::mutex> Lock(StateMutex);
    return State;
}

bool AdvancedThread::IsDedicated()
{
    std::lock_guard<std::mutex> Lock(IsDedicatedThreadMutex);
    return IsDedicatedThread;
}


void AdvancedThread::Start()
{
    std::lock_guard<std::mutex> Lock(ControlledThreadMutex);

    if (GetState() != ThreadState::ReadyToStart)
    {
        return;
    }
    
    if (IsDedicated())
    {
        ControlledThread = new std::thread(&AdvancedThread::ExecuteDedicated, this);
    }
    else
    {
        ControlledThread = new std::thread(&AdvancedThread::Execute, this);
    }
}

void AdvancedThread::Stop()
{
    std::lock_guard<std::mutex> Lock(MustStopMutex);
    MustStop = true;

    if (IsDedicated())
    {
        std::lock_guard<std::mutex> LockTask(TaskForDedicatedExecutionMutex);
        TaskForDedicatedExecution->StopDedicatedExecution();
    }
}

void AdvancedThread::StopWithWaiting()
{
    // If the thread is not stopped
    if (!(GetState() == ThreadState::Stopped || GetState() == ThreadState::NotReadyToStart))
    {
        // Then stop it
        Stop();
        WakeUp();

        // and wait for it to stop
        while (!(GetState() == ThreadState::Stopped || GetState() == ThreadState::NotReadyToStart))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    // free memory
    ControlledThreadMutex.lock();
    if (ControlledThread != nullptr) {
        if (ControlledThread->joinable())
        {
            ControlledThread->join();
        }

        delete ControlledThread;
        ControlledThread = nullptr;
    }
    ControlledThreadMutex.unlock();
}


void AdvancedThread::Sleep()
{
    std::lock_guard<std::mutex> Lock(MustSleepMutex);
    MustSleep = true;
}

void AdvancedThread::WakeUp()
{
    MustSleepMutex.lock();
    MustSleep = false;
    MustSleepMutex.unlock();

    MustSleepCondition.notify_all();
}

void AdvancedThread::SleepExecution()
{
    std::unique_lock<std::mutex> LockMustSleep(MustSleepMutex);

    while (MustSleep)
    {
        // We are waiting for a signal about the need to complete tasks, from time to time we wake up for independent checks
        MustSleepCondition.wait_for(LockMustSleep, std::chrono::milliseconds(5));

        // If we need to perform some tasks or stop, then exit the sleep
        if (GetNeedToCompleteOnceTasks() || !GetThreadCompletedTick() || GetMustStop())
        {
            LockMustSleep.unlock();
            WakeUp();
            break;
        }
    }
}


void AdvancedThread::SetMaxTickTasksPerIteration(unsigned int NewMax)
{
    std::unique_lock<std::mutex> Lock(MaxTickTasksPerIterationMutex);
    MaxTickTasksPerIteration = NewMax;
}

unsigned int AdvancedThread::GetMaxTickTasksPerIteration()
{
    std::unique_lock<std::mutex> Lock(MaxTickTasksPerIterationMutex);
    return MaxTickTasksPerIteration;
}

void AdvancedThread::SetMaxOnceTasksPerIteration(unsigned int NewMax)
{
    std::unique_lock<std::mutex> Lock(MaxOnceTasksPerIterationMutex);
    MaxOnceTasksPerIteration = NewMax;
}

unsigned int AdvancedThread::GetMaxOnceTasksPerIteration()
{
    std::unique_lock<std::mutex> Lock(MaxOnceTasksPerIterationMutex);
    return MaxOnceTasksPerIteration;
}

bool AdvancedThread::GetThreadCompletedTick()
{
    std::unique_lock<std::mutex> Lock(ThreadCompletedTickMutex);
    return ThreadCompletedTick;
}

void AdvancedThread::NotifyTickTaskAvailable()
{
    SetThreadCompletedTick(false);
    WakeUp();
}



void AdvancedThread::SetState(ThreadState NewState)
{
    std::lock_guard<std::mutex> Lock(StateMutex);
    State = NewState;
}

void AdvancedThread::SetIsDedicated(bool NewState)
{
    std::lock_guard<std::mutex> Lock(IsDedicatedThreadMutex);
    IsDedicatedThread = NewState;
}

bool AdvancedThread::GetMustStop()
{
    std::lock_guard<std::mutex> Lock(MustStopMutex);
    return MustStop;
}

bool AdvancedThread::GetMustSleep()
{
    std::lock_guard<std::mutex> Lock(MustSleepMutex);
    return MustSleep;
}


void AdvancedThread::Execute() {
    SetState(ThreadState::Started);

    while (true)
    {
        if (GetMustStop())
        {
            break;
        }

        if (GetMustSleep())
        {
            SetState(ThreadState::Asleep);

            SleepExecution();

            if (GetMustStop())
            {
                continue;
            }

            SetState(ThreadState::Working);
        }

        std::queue<ThreadTask*> CopyOfTasks;

        // Execute tasks like Tick if they need to be executed
        if (!GetThreadCompletedTick())
        {
            DeltaTickMutexRef->lock();
            float DeltaTickCopy = *DeltaTickRef;
            DeltaTickMutexRef->unlock();

            while (true)
            {
                // Take part of the tasks, if they are available
                TickTasksMutexRef->lock();
                if (!TickTasksRef->empty())
                {
                    const unsigned int MaxTasksPerIteration = GetMaxTickTasksPerIteration();
                    
                    while (!TickTasksRef->empty())
                    {
                        CopyOfTasks.push(TickTasksRef->front());
                        TickTasksRef->pop();

                        if (CopyOfTasks.size() >= MaxTasksPerIteration)
                        {
                            break;
                        }
                    }
                    
                    TickTasksMutexRef->unlock();
                }
                else
                {
                    TickTasksMutexRef->unlock();

                    // Otherwise, we mark that tasks of type Tick have been completed
                    SetThreadCompletedTick(true);

                    // And inform the manager that we have completed work on tasks of the Tick type
                    NotifyManagerThreadCompletedTickTasks();

                    break;
                }

                // Execute assigned tasks
                while (!CopyOfTasks.empty())
                {
                    try
                    {
                        CopyOfTasks.front()->Execute(DeltaTickCopy);
                    }
                    catch (const std::exception& exc)
                    {
                        Stop();
                    }
                    CopyOfTasks.pop();
                }
            }
        }


        // Check if we need to go to sleep or exit
        if (GetMustStop() || GetMustSleep())
        {
            continue;
        }


        // Execute a portion of tasks of the Once type, if available
        if (GetNeedToCompleteOnceTasks())
        {
            OnceTasksMutexRef->lock();
            if (!OnceTasksRef->empty())
            {
                const unsigned int MaxTasksPerIteration = GetMaxOnceTasksPerIteration();

                // Take part of the tasks, if they are available
                while (!OnceTasksRef->empty())
                {
                    CopyOfTasks.push(OnceTasksRef->front());
                    OnceTasksRef->pop();

                    if (CopyOfTasks.size() >= MaxTasksPerIteration)
                    {
                        break;
                    }
                }

                OnceTasksMutexRef->unlock();

                // Execute assigned tasks
                while (!CopyOfTasks.empty())
                {
                    try
                    {
                        CopyOfTasks.front()->Execute(0);
                    }
                    catch (const std::exception& exc)
                    {
                        Stop();
                    }
                    CopyOfTasks.pop();
                }
            }
            else
            {
                OnceTasksMutexRef->unlock();

                // Otherwise go to sleep
                Sleep();
            }
        }
        else {
            // Otherwise go to sleep
            Sleep();
        }
    }

    SetState(ThreadState::Stopped);
}

void AdvancedThread::ExecuteDedicated()
{
    SetState(ThreadState::Started);

    // We do not use a mutex because otherwise we will not be able to tell the executor that it is necessary to stop the execution
    // We also cannot change the task in any way, since there is protection against changes at runtime
    // Security within an executable function must be guaranteed by the user
    try
    {
        TaskForDedicatedExecution->Execute(0);
    }
    catch (const std::exception& exc)
    {
        Stop();
    }

    SetState(ThreadState::Stopped);
}


void AdvancedThread::Deinitialize()
{
    if (GetState() == ThreadState::Stopped || GetState() == ThreadState::Stopped)
    {
        StopWithWaiting();
    }

    MustStopMutex.lock();
    MustStop = false;
    MustStopMutex.unlock();

    MustSleepMutex.lock();
    MustSleep = false;
    MustSleepMutex.unlock();

    if (IsDedicated())
    {
        TaskForDedicatedExecutionMutex.lock();
        delete TaskForDedicatedExecution;
        TaskForDedicatedExecution = nullptr;
        TaskForDedicatedExecutionMutex.unlock();

        SetIsDedicated(false);
    }
    else
    {
        SetThreadCompletedTick(false);

        OnceTasksRef = nullptr;
        OnceTasksMutexRef = nullptr;

        TickTasksRef = nullptr;
        TickTasksMutexRef = nullptr;

        ThreadCompletedTickTasksRef = nullptr;
        ThreadCompletedTickTasksMutexRef = nullptr;
        ThreadCompletedTickTasksConditionRef = nullptr;

        DeltaTickRef = nullptr;
        DeltaTickMutexRef = nullptr;
    }

    SetState(ThreadState::NotReadyToStart);
}

void AdvancedThread::SetCanBeDestroyed(bool NewState)
{
    std::lock_guard<std::mutex> Lock(CanBeDestroyedMutex);
    CanBeDestroyed = NewState;
}

bool AdvancedThread::GetCanBeDestroyed()
{
    std::lock_guard<std::mutex> Lock(CanBeDestroyedMutex);
    return CanBeDestroyed;
}

void AdvancedThread::SetThreadCompletedTick(bool NewState)
{
    std::lock_guard<std::mutex> Lock(ThreadCompletedTickMutex);
    ThreadCompletedTick = NewState;
}



bool AdvancedThread::GetNeedToCompleteOnceTasks()
{
    std::lock_guard<std::mutex> Lock(*OnceTasksMutexRef);
    return !OnceTasksRef->empty();
}

void AdvancedThread::NotifyManagerThreadCompletedTickTasks()
{
    {
        std::lock_guard<std::mutex> Lock(*ThreadCompletedTickTasksMutexRef);
        *ThreadCompletedTickTasksRef = true;
    }

    ThreadCompletedTickTasksConditionRef->notify_all();
}

#pragma once
#include <iostream>
#include "MultithreadingModule.h"
#include "ThreadFunctionTask.h"
#include "ThreadMethodTask.h"

void TickExecution(float DeltaTime) {
    std::cout << "Tick task execution started\n";

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::cout << "Tick task execution finished\n";
};

void FunctionDedicatedExecution(const TaskStopSignal& StopSignal) {
    std::cout << "Function dedicated execution started\n";

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        if (StopSignal.GetState())
        {
            std::cout << "Function dedicated execution must be stopped\n";
            break;
        }
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "Function dedicated execution finished\n";
};

class TestMethodClass
{
public:
    TestMethodClass() {};
    ~TestMethodClass() {};

    void MethodDedicatedExecution(const TaskStopSignal& StopSignal) {
        std::cout << "Method dedicated execution started\n";

        std::this_thread::sleep_for(std::chrono::seconds(1));

        std::cout << "Method dedicated execution finished\n";
    };

private:

};


int main()
{
    MultithreadingModule MM;
    MM.SetMaxNumOfThreads(7);
    MM.SetMaxTickTasksPerIteration(1);

    MM.StartThreads();

    ThreadFunctionTask* FunctionTask;

    FunctionTask = new ThreadFunctionTask(&FunctionDedicatedExecution);
    MM.AddTask(FunctionTask);

    TestMethodClass* TMC = new TestMethodClass();
    ThreadMethodTask<TestMethodClass>* MethodTask = new ThreadMethodTask<TestMethodClass>(TMC, &TestMethodClass::MethodDedicatedExecution);
    MM.AddTask(MethodTask);

    for (size_t i = 0; i < 10; i++)
    {
        FunctionTask = new ThreadFunctionTask(&TickExecution);
        MM.AddTask(FunctionTask);
    }
    
    for (size_t i = 0; i < 5; i++)
    {
        std::cout << "Tick iteration: " << i + 1 << '\n';
        MM.Tick(0);
    }

    std::cout << "The end (waiting)\n";

    std::this_thread::sleep_for(std::chrono::seconds(1));

    delete TMC;

    std::cout << "The end\n";

    return 0;
}
#pragma once
#include <mutex>

class TaskStopSignal
{
private:
	bool State;
	mutable std::mutex StateMutex;

public:
	TaskStopSignal();

	void SetState(bool NewState);
	bool GetState() const;
};


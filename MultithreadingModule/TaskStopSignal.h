#pragma once
#include <mutex>

class TaskStopSignal
{
private:
	bool bState;
	mutable std::mutex StateMutex;

public:
	TaskStopSignal();

	void SetState(bool bNewState);
	bool GetState() const;
};


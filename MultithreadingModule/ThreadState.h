#pragma once

enum class ThreadState {
	NotReadyToStart,
	ReadyToStart,
	Started,
	Asleep,
	Awaiting,
	Working,
	Stopped
};

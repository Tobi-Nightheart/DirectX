#pragma once
class GameTimer
{
public:
	GameTimer();
	

	float DeltaTime() const; // in seconds
	float TotalTime() const;

	void Reset(); //Call before message loop
	void Start(); //Call when unpaused
	void Stop(); //Call when paused
	void Tick(); //Call every Frame
	~GameTimer();
private:
	double mSecondsPerCount;
	double mDeltaTime;

	__int64 mBaseTime;
	__int64 mPausedTime;
	__int64 mStopTime;
	__int64 mPrevTime;
	__int64 mCurrTime;

	bool mStopped;
};


#include "pch.h"
#include "GameTimer.h"
#include  <Windows.h>

GameTimer::GameTimer() : mSecondsPerCount(0.0), mDeltaTime(-1.0), mBaseTime(0), mPausedTime(0), mPrevTime(0), mCurrTime(0), mStopped(false)
{
	__int64 countsPerSec;
	QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&countsPerSec));
	mSecondsPerCount = 1.0 / static_cast<double>(countsPerSec);
}

void GameTimer::Tick()
{
	if(mStopped)
	{
		mDeltaTime = 0.0;
	}

	//Get the time this frame
	__int64 currTime;
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currTime));
	mCurrTime = currTime;

	//Time diffrence between the current and last frame
	mDeltaTime = (mCurrTime - mPrevTime) *mSecondsPerCount;

	//Prepare for next frame
	mPrevTime = mCurrTime;

	//Force nonnegative as this might occur due to power safe mode etc.
	if(mDeltaTime < 0.0f)
	{
		mDeltaTime = 0.0f;
	}
}

float GameTimer::DeltaTime() const
{
	return static_cast<float>(mDeltaTime);
}

void GameTimer::Reset()
{
	__int64 currTime;
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currTime));

	mBaseTime = currTime;
	mPrevTime = currTime;
	mStopTime = 0;
	mStopped = false;
}

void GameTimer::Stop()
{
	//if stopped is true then dont do anything
	if(!mStopTime)
	{
		__int64 currTime;
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currTime));

		//otherwise saving the time that was stopped at and set the boolean flag
		mStopTime = currTime;
		mStopped = true;
	}
}

void GameTimer::Start()
{
	__int64 startTime;
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&startTime));

	//if resumed from stop accumulate the paused time and set the curr- previous time appropriately
	if(mStopped)
	{
		mPausedTime += (startTime - mStopTime);
		mPrevTime = startTime;
		mStopTime = 0;
		mStopped = false;
	}
}

float GameTimer::TotalTime() const
{
	//if we are stopped the time since stop does not count
	if(mStopped)
	{
		return (float)(((mStopTime - mPausedTime) - mBaseTime)*mSecondsPerCount);
	}

	else
	{
		return (float)(((mCurrTime - mPausedTime) - mBaseTime) *mSecondsPerCount);
	}
}




GameTimer::~GameTimer()
{
}

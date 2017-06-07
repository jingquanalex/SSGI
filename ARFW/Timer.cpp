#include "Timer.h"

using namespace std;

// === Static Declarations ===

vector<Timer*> Timer::listTimers;

// Update all the timers in the list.
// If toDelete is true, delete that timer obj and remove from list.
void Timer::updateTimers(float dt)
{
	for_each(listTimers.begin(), listTimers.end(), [dt](Timer*& timer) {
		if (timer->getToDelete())
		{
			delete timer;
			timer = nullptr;
		}
		else
		{
			timer->update(dt);
		}
	});
	listTimers.erase(remove(listTimers.begin(), listTimers.end(), 
		static_cast<Timer*>(nullptr)), listTimers.end());
}

// New timer constructor. 
// interval - seconds before each tick, duration - seconds before timer stop.
Timer::Timer(float interval, float duration)
{
	this->tickInterval = interval;
	this->tickDuration = durationLeft = duration;
	listTimers.push_back(this);
}

Timer::~Timer()
{
	//printf("timer deleted \n");
}

// Flag timer for deletion.
void Timer::destroy()
{
	toDelete = 1;
}

// Update this particular timer.
void Timer::update(float dt)
{
	if (!isRunning) return;

	// Correct accumulation till dt > interval
	if (dt > tickInterval)
	{
		currentTime = 0;
		hasTicked = 1;
		return;
	}

	// Manage time in seconds.
	currentTime += dt;

	// If tick inverval is elasped, set tick.
	if (currentTime >= tickInterval)
	{
		// If timer is set to run over a duration.
		if (tickDuration != 0.0f)
		{
			durationLeft -= currentTime;
		}

		currentTime -= tickInterval;

		//currentTime -= max(tickInterval, dt);

		hasTicked = true;
	}

	// Stop timer if it has ran its duration
	if (tickDuration != 0.0f && durationLeft <= 0.0f)
	{
		isRunning = false;
	}
}

bool Timer::ticked()
{
	if (hasTicked)
	{
		hasTicked = false;
		return true;
	}

	return false;
}

void Timer::start()
{
	isRunning = true;
}

void Timer::stop()
{
	isRunning = false;
}

void Timer::reset()
{
	durationLeft = tickDuration;
	currentTime = 0.0f;
}

float Timer::getTickInterval() const
{
	return tickInterval;
}

float Timer::getDuration() const
{
	return tickDuration;
}

bool Timer::getIsRunning() const
{
	return isRunning;
}

bool Timer::getToDelete() const
{
	return toDelete;
}

void Timer::setTickInterval(float interval)
{
	tickInterval = interval;
}

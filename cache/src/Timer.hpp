#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

#include <functional>
#include <list>

// typedef void (*TimerExpired)(void *);

class Timer
{
public:
	Timer();

	Timer(int32_t timeout_ms, bool periodic, 
				std::function<void (void *)> TimerCallback, void *data);

	~Timer() { pTimers.remove(this); }

	void start() { running = true; }

	void stop();

	void reset() { current = timeout; }

	void pause() { running = false; }

	bool isRunning() { return running; }

	void setInterval(int32_t timeout_ms);

	void setPeriodic(bool isPeriodic) { periodic = isPeriodic; }

	// void setCallback(TimerExpired TimerCallback) { callback = TimerCallback; }
	void setCallback(std::function<void (void *)> TimerCallback) { callback = TimerCallback; }

	void setData(void *);

private:
	int32_t timeout;  // 超时时间 ms

	bool periodic;  //false: 单次定时事件；true: 循环定时

	bool running;

	std::function<void (void *)> callback;

	void *pData;

	int32_t current;

	static std::list<Timer *> pTimers;

	static bool initialized;

	static void timer_handler(int);

	void createTimer(int32_t timeout_ms, bool periodic, 
									 std::function<void (void *)> TimerCallback, void *data);
};

#endif
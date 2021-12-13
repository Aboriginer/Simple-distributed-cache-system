#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include <list>

#include "timer.hpp"

std::list<Timer *> Timer::pTimers;

bool Timer::initialized = false;

void Timer::createTimer(const int32_t timeout_ms, bool _periodic, 
												std::function<void (void *)> TimerCallback, void *data) {
	static struct sigaction sa;
	static struct itimerval timer;

	timeout = timeout_ms;
	periodic = _periodic;
	callback = TimerCallback;
	setData(data);
	current = timeout_ms;

	if (initialized) {
		pTimers.push_back(this);
		return;
	}

	pTimers.push_back(this);

	running = false;

	// 将 timer_handler 作为超时信号 SIGVTALRM 的信号处理函数*/
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &timer_handler;
	sigaction(SIGALRM, &sa, nullptr);

	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = TIME_UNIT;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = TIME_UNIT;

	initialized = true;

	setitimer(ITIMER_REAL, &timer, nullptr);
}

Timer::Timer() {
	createTimer(100000 / TIME_UNIT, false, nullptr, nullptr);
}

Timer::Timer(int32_t timeout_ms, bool _periodic, 
						 std::function<void (void *)> TimerCallback, void *data) {
	createTimer(timeout_ms, _periodic, TimerCallback, data);
}

void Timer::stop() {
	running = false;
	current = timeout;
}

void Timer::setData(void *data) {
	if (data == nullptr) {
		data = this;
	}
	pData = data;
}

void Timer::setInterval(int32_t timeout_ms) {
	timeout = timeout_ms;
	current = timeout;
}

void Timer::timer_handler(int signum) {
	std::list<Timer *>::iterator it;
	std::list<Timer *> callbackList;

	// 遍历到时间的定时器
	for (it = Timer::pTimers.begin(); it != Timer::pTimers.end(); it++) {
		Timer *pTimer = *it;
		if (pTimer->running) {
			// pTimer->current--;
			--pTimer->current;
			if (pTimer->current == 0) {
				if (pTimer->callback) {
					callbackList.push_back(pTimer);
				}
			}
		}
	}
	// 执行 callbacks
	for (it = callbackList.begin(); it != callbackList.end(); it++) {
		Timer *pTimer = *it;
		pTimer->callback(pTimer->pData);
	}
	
	for (it = Timer::pTimers.begin(); it != Timer::pTimers.end(); it++) {
		Timer *pTimer = *it;
		if (pTimer->running) {
			if (pTimer->current == 0) {
				if (pTimer->periodic) {
					pTimer->current = pTimer->timeout;
				} else {
					pTimer->running = false;
				}
			}
		}
	}
}

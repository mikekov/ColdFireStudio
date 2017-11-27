#pragma once

class WndTimer
{
public:
	WndTimer() : timer_(0), hwnd_(nullptr) {}

	~WndTimer() { Stop(); }

	bool Start(HWND hwnd, int timer_id, UINT delay)
	{
		Stop();

		hwnd_ = hwnd;
		timer_ = hwnd ? ::SetTimer(hwnd, timer_id, delay, nullptr) : 0;
		return timer_ != 0;
	}

	void Stop()
	{
		if (timer_ && hwnd_)
			::KillTimer(hwnd_, timer_);
		timer_ = 0;
	}

	UINT_PTR Id() const { return timer_; }

private:
	UINT_PTR timer_;
	HWND hwnd_;
};

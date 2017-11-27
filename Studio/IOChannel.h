#ifndef _io_channel_
#define _io_channel_

// Terminal window communication channel

class IOChannel
{
public:
	// erase terminal window
	virtual void Clear() = 0;

	// output single character interpreting control special chars, like new line
	virtual void PutChar(int chr) = 0;

	// read input character if any, returns 0 if nothing's ready
	virtual int GetChar() = 0;

	// cursor position
	virtual int GetCursorXPos() = 0;
	virtual int GetCursorYPos() = 0;
	virtual void SetCursorXPos(int x) = 0;
	virtual void SetCursorYPos(int y) = 0;

	virtual int GetTerminalWidth() = 0;
	virtual int GetTerminalHeight() = 0;
	virtual void SetTerminalWidth(int width) = 0;
	virtual void SetTerminalHeight(int height) = 0;

	//TODO: define escape sequences understood by this simple terminal
	// currently none
};

#endif

/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#ifndef _debugger_h_
#define _debugger_h_

#include "Defs.h"
#include "../ColdFire/Simulator.h"
#include "../ColdFire/DebugData.h"
#include "IOChannel.h"
#include "SettingsClient.h"

// Debugger class - mainly delegating calls to CF simulator. Also configuring CF simulator.

class Debugger : SettingsClient
{
	enum IOFunc			// simulator functions
	{ 
		IO_NONE = -1,
		TERMINAL_CLS,
		TERMINAL_IN_OUT,
		TERMINAL_X_POS,
		TERMINAL_Y_POS,
		TERMINAL_WIDTH,
		TERMINAL_HEIGHT,
		RANDOM_NUMBER,

		IO_LAST_FUNC= RANDOM_NUMBER
	};

	enum DebuggerIO
	{
		RAM_BASE= 0, RAM_END, GET_TIMER, GET_DATE_TIME, PROG_START, OTHER_FUNCTIONS
	};

	CWnd* io_window();		// find/create terminal window
	IOChannel* terminal_;

	cf::uint32 io_port_addr_;	// 'ports' mapped into memory address space used by simulator to invoke IOFunc
	void io_function(int command, cf::uint32& value, bool read);

	bool PeripheralIO(cf::uint32 addr, int access_size, cf::uint32& ret_val, bool read);
	bool Exception(cf::uint32 addr, CpuExceptions vector, cf::uint32 pc);

public:
	struct MainWindow
	{
		virtual LRESULT SendMsg(int msg, WPARAM, LPARAM) = 0;
		virtual LRESULT PostMsg(int msg, WPARAM, LPARAM) = 0;
		virtual void DeviceIO(PeripheralDevice* device, cf::DeviceAccess access, cf::uint32 addr) = 0;
	};

	CString GetStatusMessage(SimulatorStatus status) const;
	SimulatorStatus GetStatus() const;

	Debugger();

	void SetMainWnd(MainWindow* wnd, int get_terminal_msg);

	void Restart();

	SimulatorStatus StepInto();
	SimulatorStatus StepOver();
	SimulatorStatus StepOut();
	SimulatorStatus Run();
	SimulatorStatus StepIntoException();
	void RunToAddress(uint32 addr);
	void SkipToAddress(uint32 addr);
	void RunToAddress(int line, const CString& path);
	void SkipToAddress(int line, const CString& path);

	// load monitor program from file
	void LoadMonitorCode(const wchar_t* path);
	// load monitor code into simulator
	void LoadMonitor();

	// breakpoints
	Defs::Breakpoint ToggleBreakpoint(int line, const CString& path);
	Defs::Breakpoint ToggleBreakpoint(cf::uint32 addr);
	Defs::Breakpoint GetBreakpoint(cf::uint32 addr);
	Defs::Breakpoint SetBreakpoint(int line, const CString& path);
	void SetBreakpoint(cf::uint32 addr);
	void ClearAllBreakpoints();

	// running simulated program
	bool IsFinished() const;
	bool IsRunning() const;		// is simulator program running?
	void Break();				// break program execution
	void AbortProg();
	bool IsStopped() const;
	bool IsActive() const;
	bool IsStoppedAtException() const;

	int GetCurrentLine() const;

	void SetProgram(const cf::BinaryProgram& code, bool run_monitor);

	cf::uint32 GetRegister(cf::Register reg) const;
	void SetRegister(cf::Register reg, cf::uint32 value);
	void ModifyRegister(cf::Register reg, cf::uint32 add, cf::uint32 remove);

	// read condition code (CCR)
	bool GetFlag(cf::Flag flag) const;

	// disassemble opcode at 'addr'
	DecodedInstruction DecodeInstruction(uint32 addr);

	void SetDebugInfo(std::unique_ptr<DebugData> debug);

	const cf::BinaryProgram& GetCode() const	{ return code_; }

	std::wstring GetCurrentLinePath() const;
	cf::uint32 GetCurrentProgCounter() const;

	// using debug info determine opcode address corresponding to 'line' in source 'file'
	boost::optional<cf::uint32> GetCodeAddress(int line, const CString& file);

	// simulator memory access
	enum Bank { RAM= 0, Flash };
	cf::MemoryBankInfo GetMemoryBankInfo(int bank) const;
	// change memory at given address
	void SetMemory(const std::vector<cf::uint8>& m, cf::uint32 address);
	void SetMemory(const cf::uint8* m, cf::uint32 size, cf::uint32 address);
	// read memory; handles "no man's lend" too returning zeros
	cf::uint32 ReadMemory(cf::uint8* dest_buf, cf::uint32 address, cf::uint32 length);

	ISA GetDefaultIsa() const;

	// simulator's current ISA
	ISA GetCurrentIsa() const;

	// if simulator status is SIM_EXCEPTION this function tells which one it was
	CpuExceptions GetLastExceptionVector() const;

	//TODO: cycles so far
	cf::uint32 CyclesTaken() const;
	cf::uint32 ExecutedInstructions() const;

	PeripheralDevice* FindDevice(const char* category, const char* version) const;

private:
	virtual void ApplySettings(SettingsSection& settings);
	void SimEvent(cf::Event event, const Simulator::EventArgs& params);
	Simulator simulator_;	// ColdFire simulator
	int current_line_;
	std::unique_ptr<DebugData> debug_info_;
	cf::BinaryProgram code_;
	cf::BinaryProgram monitor_;
	void init();
	MainWindow* main_wnd_;
	CWnd* terminal_wnd_;
	int get_terminal_msg_;

	// exception info; populated when simulator enters exception handled by debugger
	struct ExceptionInfo
	{
		ExceptionInfo() : pc(0), addr(0), vector(EX_SIZE)
		{}
		cf::uint32 pc;
		cf::uint32 addr;
		CpuExceptions vector;
	} ex_info_;

	mutable std::mutex lock_;
	// copy of program counter; this is what PC was when update event was received;
	// it's kept here so clients can poll its value when they process updates
	cf::uint32 cur_prog_counter_;
	// if debug info is present PC will be used during update event to determine where
	// the source file is; this path is kept for debugger clients
	std::wstring cur_line_path_;
	ISA default_isa_;
};

#endif

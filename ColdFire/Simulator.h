/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

// This is the main interface to most of the simulator functionality (load configuration is separate).
// It is expected to be compiled along with a client app, as it uses STL and doesn't offer ABI stability.

#pragma once
#include "Import.h"
#include "Isa.h"
#include "Types.h"
#include "DecodedInstr.h"
#include "BinaryProgram.h"
#include "PeripheralDevice.h"


enum SimulatorStatus
{
	SIM_OK= 0,
	//todo:
	//SIM_BPT_EXECUTE,		// exec breakpoint
	//SIM_BPT_READ,			// mem read breakpoint
	//SIM_BPT_WRITE,		// mem write breakpoint
	//SIM_BPT_TEMP,			// not used
	SIM_STOPPED,			// simulator stopped - TODO: consolidate OK & STOPPED states?
	SIM_BREAKPOINT_HIT,		// simulator stopped at breakpoint
	SIM_EXCEPTION,			// stopped at CF exception
	SIM_IS_RUNNING,			// program execution pending
	SIM_FINISHED,			// program simulation finished
	SIM_INTERNAL_ERROR,		// error inside simulator (a bug)
};


class CF_DECL Simulator
{
public:
	Simulator();
	~Simulator();

	// simulated instruction set
	void SetIsa(ISA isa);
	ISA GetIsa() const;

	// create memory bank
	void CreateMemoryBank(std::string name, cf::uint32 base, cf::uint32 size, int bank, cf::MemoryAccess access);

	// memory banks
	cf::MemoryBankInfo GetMemoryBankInfo(int bank) const;

	// modify memory
	void SetMemory(cf::uint32 address, const cf::uint8* begin, const cf::uint8* end);
	void ZeroMemory(cf::uint32 address, cf::uint32 size);
	// read memory
	cf::uint32 ReadMemory(cf::uint8* dest_buf, cf::uint32 address, cf::uint32 length);
	// erase entire memory
	void ClearMemory();
	// once memory size is established, this routine will try to write two values to the VBR location:
	// initial stack pointer address, and initial PC address; stack is at the end of RAM, reset PC is given
	void SetInitialStackAndPC(cf::uint32 reset_start);

	// read simulator configuration file add and add all peripherals to the simulator
	void LoadConfiguration(const wchar_t* cfg_file);

	// find (created) peripheral device
	PeripheralDevice* FindPeripheral(const char* category, const char* version) const;

	// move SP to the top of RAM
	void InitSP();

	// copy programm to the simulator's memory; start_addr is loaded into PC reg
	void SetProgram(const cf::BinaryProgram& code);
	void SetProgram(cf::uint32 program_addr, const cf::uint8* begin, const cf::uint8* end, cf::uint32 start_addr);
	void SetProgram(cf::uint32 program_addr, const std::vector<cf::uint16>& code, cf::uint32 start_addr);

	// reset CPU
	void Reset();

	// simulate execution
	SimulatorStatus Step();
	SimulatorStatus StepOver();
	SimulatorStatus Run();
	SimulatorStatus StepOut();
	SimulatorStatus RunToAddress(cf::uint32 address);
	SimulatorStatus OneStep();	// exec one instruction, enter exceptions/traps as needed

	SimulatorStatus BreakExecution();
	SimulatorStatus AbortExecution();

	// current simulator state
	SimulatorStatus GetStatus() const;
	std::string GetStatusMsg(SimulatorStatus status) const;

	std::string GetExceptionMsg(uint32 address, CpuExceptions vector, uint32 pc) const;

	// manipulate registers
	cf::uint32 GetRegister(cf::Register reg) const;
	void SetRegister(cf::Register reg, cf::uint32 value);
	void SetRegister(cf::Register reg, cf::uint32 add, cf::uint32 remove);

	// condition flags
	bool GetFlag(cf::Flag flag) const;

	bool IsStopped() const;

	struct EventArgs
	{
		EventArgs(cf::uint32 param, PeripheralDevice* device, cf::DeviceAccess access)
			: param(param), device(device), access(access)
		{}

		PeripheralDevice* device;
		cf::uint32 param;
		cf::DeviceAccess access;
	};

	// event callback to notify client about changes
	void SetEventCallback(const std::function<void (cf::Event ev, const EventArgs& params /*cf::uint32 param*/)>& callback);

	// disassemble single instruction
	DecodedInstruction DecodeInstruction(uint32 addr);

	// simulator read/write callback
	typedef std::function<bool (uint32 addr, int access_size, uint32& val, bool read)> PeripheralCallback;
	void SetSimulatorCallback(const PeripheralCallback& io);
	cf::uint32 GetSimulatorIOArea() const;

	// CPU exceptions
	typedef std::function<bool (uint32 address, CpuExceptions vector, uint32 pc)> ExceptionCallback;
	void SetExceptionCallback(const ExceptionCallback& ex);

	// execution breakpoints
	void SetBreakpoint(uint32 address, bool set);
	void SetTempBreakpoint(uint32 address);
	void EnableBreakpoint(uint32 address, bool enable);
	bool GetBreakpoint(uint32 address) const;
	void ClearAllBreakpoints();
	std::vector<uint32> GetAllBreakpoints() const;

	// set exception handling for exception 'ex':
	// stop = true -> stop execution
	// stop = false -> go to exception handler
	void ExceptionHandling(CpuExceptions ex, bool stop);

	// TODO: count of cycles used by executed instructions
	uint32 CyclesTaken() const;
	// count of instructions executed by simulator
	uint32 ExecutedInstructions() const;
	void ZeroStats();	// clear cycle and instruciton counter

	// set default values for some MCU configuration registers (VBR, MBAR)
	void SetConfigDefaults(cf::Register reg, uint32 value);

private:
	Simulator(const Simulator&);
	Simulator& operator = (const Simulator&);

	struct Impl;
	Impl* impl_;
};

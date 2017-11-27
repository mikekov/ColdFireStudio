/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#include "MachineDefs.h"
#include "BasicTypes.h"
#include "Types.h"
#include "CpuExceptions.h"
#include "InstructionMap.h"
#include "InterruptController.h"

#undef OVERFLOW		// undef offensive definition from math.h


class McuException
{
public:
};

// exception internal to the simulator thrown on invalid memory access or invalid addressing mode
// they are caught and translated into EnterException call

class MemoryAccessException : public McuException
{
public:
	MemoryAccessException(uint32 bad_address) : bad_address(bad_address)
	{}

	uint32 bad_address;
};


class AddressingModeException : public McuException
{
public:
	AddressingModeException(uint16 mode_reg) : mode_reg(mode_reg)
	{}

	uint16 mode_reg;
};


class FaultOnFault : public McuException
{
public:
	// if a ColdFire processor encounters any fault while processing another fault,
	// it immediately halts execution with a catastrophic fault-on-fault condition
	FaultOnFault()
	{}
};


// simple struct to keep track of physical addresses (physical to the simulator)
//
struct DecodedAddress
{
	enum AddrType : uint8	// address points to:
	{
		RAM,			// CPU's RAM, SRAM
		FLASH,			// CPU's flash memory
		NONE,			// null area, no memory behind it; can be read/written to without access violation or side effects
		REGISTER,		// CPU's register (Dx or Ax)
		PERIPHERALS,	// MCU's peripherals
		SIMULATOR_IO,	// communication with simulator
		INVALID			// unmapped area
	};

	DecodedAddress(const void* address, uint32 cf_addr, AddrType type)
		: address(const_cast<void*>(address)), big_endian(true), type(type), cf_addr(cf_addr)
	{}

	DecodedAddress(void* address, uint32 cf_addr, AddrType type)
		: address(address), big_endian(true), type(type), cf_addr(cf_addr)
	{}

	DecodedAddress(void* address, uint32 cf_addr, AddrType type, bool big_endian)
		: address(address), big_endian(big_endian), type(type), cf_addr(cf_addr)
	{}

	DecodedAddress(uint32 port, AddrType type)
		: address(nullptr), type(type), cf_addr(port), big_endian(true)
	{}

	void* address;	// decoded, physical address (not visible to code executing on CF)
	bool big_endian;
	AddrType type;
	uint32 cf_addr;	// original address as seen by ColdFire
};


enum EffectiveAddressModeField
{
	EAF_Dx= 0,
	EAF_Ax,
	EAF_Ax_Ind,
	EAF_Ax_Incr,
	EAF_Decr_Ax,
	EAF_Disp_Ax,
	EAF_Disp_Ax_Idx,
	EAF_Other,
	EAF_Immediate= (7 << 3) | 4
};


class Context;

class CPU
{
public:
	CPU(ISA isa);

	void SetContext(Context& ctx);

	void ResetRegs();	// zero all regs

	void SetStackPointers(uint32 user, uint32 system);
	std::pair<uint32, uint32> GetStackPointers() const;

	void SetSR(uint16 sr);
	uint16 GetSR();
	void SetCCR(uint16 ccr);

	void EnterSupervisorState();
	void ExitSupervisorState();

	bool Supervisor() const				{ return !!sr.s; }
	bool Trace() const					{ return !!sr.t; }
	int InterruptLevel() const			{ return sr.i; }
	void SetInterruptLevel(int level)	{ sr.i = level; }

	void EnterException(CpuExceptions vector, uint32 opcode_addr);

	// cpu registers

	uint32 d_reg[8];	// order here is important; to save time index form 0 to 15 may be
	uint32 a_reg[8];	// used to access registers D and A

	uint32 pc;

	bool extend;
	bool carry;
	bool zero;
	bool negative;
	bool overflow;

	uint32 vbr;
	uint32 mbar;
#if 0
	//TODO: MBAR structure is MCU-specific
#if BIT_FIELDS_LSB_TO_MSB
	union MBAR	// MBAR in 5206
	{
		struct
		{
			uint32 valid : 1;
			uint32 user_data : 1;
			uint32 user_code : 1;
			uint32 super_data : 1;
			uint32 super_code : 1;
			uint32 mask_cycles : 1;
			uint32 alternate_master_mask : 1;
			uint32 reserved1 : 1;
			uint32 write_protect : 1;
			uint32 reserved2 : 3;
			uint32 base_address : 20;
		};
		uint32 mbar;
	};
#else
	#error "to do MBAR"
#endif
	MBAR mbar;
	uint32 GetMBAR() const { return /*mbar.valid ?*/ mbar.mbar & CPU::MBAR_ADDR_MASK /*: 0*/; }
#endif
	enum : uint32 { MBAR_ADDR_MASK= 0xfffff000 };

	uint32 rambar1;
	uint32 rambar2;
	uint32 rombar1;
	uint32 rombar2;

//	uint32 GetMBAR() const	{ return mbar & CPU::MBAR_ADDR_MASK; }

	ISA GetISA() const		{ return isa_; }
	void SetISA(ISA isa)	{ isa_ = isa; }

	void SetUSP(uint32 addr);
	uint32 GetUSP() const;

	void SetDefaults(cf::Register reg, uint32 value);

private:

#if BIT_FIELDS_LSB_TO_MSB
	union StatusReg
	{
		struct
		{
			uint16 c : 1;
			uint16 v : 1;
			uint16 z : 1;
			uint16 n : 1;
			uint16 x : 1;
			uint16 reserved1 : 2;
			uint16 p : 1;	// branch prediction
			uint16 i : 3;	// interrupt priority
			uint16 reserved2 : 1;
			uint16 m : 1;
			uint16 s : 1;	// supervisor
			uint16 reserved3 : 1;
			uint16 t : 1;	// trace
		};
		uint16 sr;
	} sr;
#else
	#error "to do StatusReg"
#endif
	uint32 other_a7;

	ISA isa_;
	uint32 default_vbr_;
	uint32 default_mbar_;

	Context* ctx_;
};


// to interrupt execution when entering exception
struct ExceptionReported
{};


class Context
{
public:
	Context(ISA isa);
	virtual ~Context();

	void SetIsa(ISA isa);
	ISA GetIsa() const;

	// establish size of RAM and flash memory
	void InitMemory(std::string name, uint32 base_addr, uint32 mem_size, int bank, cf::MemoryAccess access);

	// clear memory set (fill with zeros)
	void ClearMemory(int bank);

	// clear 'size' bytes of memory (from a single bank only) starting from 'address'
	void ZeroMemory(uint32 address, uint32 size);

	// read memory content and copy it to the provided buffer
	cf::uint32 ReadMemory(cf::uint8* dest_buf, cf::uint32 address, cf::uint32 length);

	// report configured memory area; currently bank = 0 is RAM, bank = 1 is flash
	cf::MemoryBankInfo GetMemoryBankInfo(int bank) const;

	// copy program to the memory (all throw MemoryAccessException if program doesn't fit)
	void CopyProgram(const std::vector<uint16>& program, uint32 prg_base);
	void CopyProgram(const std::vector<uint8>& program, uint32 start);
	void CopyProgram(const uint8* begin, const uint8* end, uint32 start_address);

	// memory/peripherals access routines
	uint16 GetWord(uint32 addr) const;
	int16 GetSWord(uint32 addr) const;
	uint32 GetLongWord(uint32 addr) const;

	// low level routines for memory and peripherals read/write access
	uint32 ReadFromAddress(const DecodedAddress& da, InstrSize size, bool disable_io= false) const;
	void WriteToAddress(const DecodedAddress& da, uint32 value, InstrSize size);

	// convenience functions to read word from (PC), and advance program counter
	uint16 GetNextPCWord()				{ uint16 v= GetWord(cpu_.pc); cpu_.pc += 2; return v; }
	uint32 GetNextPCLongWord()			{ uint32 v= GetLongWord(cpu_.pc); cpu_.pc += 4; return v; }

	// this memory read ignores peripherals; used by disassembler to avoid triggering IO changes
	uint16 ReadMemoryWord(uint32 addr) const;
	uint32 ReadMemoryLongWord(uint32 addr) const;

	const Instruction* GetInstruction(uint16 opcode) const	{ return instr_map_[opcode]; }

	// returns address of requested memory cell, or throws if it is not valid
	DecodedAddress GetMemoryAddress(uint32 addr, InstrSize size) const;
	DecodedAddress GetMemoryAddress(uint32 addr, uint32 size) const;
	// ditto, but returns INVALID memory type on access errors; doesn't throw
	DecodedAddress GetMemoryAddress(uint32 addr, uint32 size, bool no_throw) const;

	// decode effective address using 3 bit mode & 3 bit register code; size is an [in/out] param;
	// words is [out] - amount of extension words read (used by this addressing mode)
	// big_endian is [out] - memory addresses point to big endian numbers,
	// whereas register addresses are pointing to low endian values
	DecodedAddress DecodeMemoryAddress(uint16 mode_reg, InstrSize& size, int& words);

	// decode effective address in ColdFire address space
	uint32 DecodeEffectiveAddress(uint16 mode_reg, int& ext_words);

	// retrieve source value
	uint8 DecodeSrcByteValue(uint16 mode_reg, InstrSize size, int& words);
	uint16 DecodeSrcWordValue(uint16 mode_reg, InstrSize size, int& words);
	uint32 DecodeSrcLongWordValue(uint16 mode_reg, InstrSize size, int& words);

	// store value 'src' at the destination
	void DecodeAndSetDestByte(uint16 mode_reg, InstrSize size, int& words, uint8 src);
	void DecodeAndSetDestWord(uint16 mode_reg, InstrSize size, int& words, uint16 src);
	void DecodeAndSetDestLongWord(uint16 mode_reg, InstrSize size, int& words, uint32 src);

	uint32 GetDataRegister(uint32 index);

	uint16 OpCode() const				{ return current_opcode_; }

	template<class STENCIL>
	void OpCode(STENCIL& s) const		{ s.opcode = current_opcode_; }

	void StepPC(int ext_words= 1)		{ cpu_.pc += ext_words << 1; }

	// push/pull addres to/from the stack; modify SP
	void PushLongWord(uint32 addr);
	uint32 PullLongWord();
	uint16 PullWord();

	uint32 InstructionPointer(int ext_words= 0) const	{ return cpu_.pc + (ext_words << 1); }

	const Instruction* ExecuteInstruction(bool continue_on_exceptions);
	void HaltExecution(bool halt);
	void EnterStopState();

	bool IsExecutionHalted() const;

	void EnterException(CpuExceptions vector, uint32 address);

	CPU& Cpu()							{ return cpu_; }

	// CCR
	bool Carry() const					{ return cpu_.carry; }
	bool Zero() const					{ return cpu_.zero; }
	bool Negative() const				{ return cpu_.negative; }
	bool Overflow() const				{ return cpu_.overflow; }
	bool Extend() const					{ return cpu_.extend; }

	void SetZero(uint32 zero)			{ cpu_.zero = zero != 0; }
	void SetCarry(uint32 carry)			{ cpu_.carry = carry != 0; }
	void SetCarry(bool carry)			{ cpu_.carry = carry; }
	void SetNegative(uint32 neg)		{ cpu_.negative = neg != 0; }
	void SetOverflow(uint32 overflow)	{ cpu_.overflow = overflow != 0; }
	void SetOverflow(bool overflow)		{ cpu_.overflow = overflow; }
	void SetExtend(uint32 ext)			{ cpu_.extend = ext != 0; }

	bool Supervisor() const				{ return cpu_.Supervisor(); }

	// test result and set flags accordingly to reflect operation
	enum SetCC { ADD, SUB, CMP, NEG };
	void SetAllFlags(uint32 result, uint32 arg1, uint32 arg2, InstrSize size, bool zero_conditional, SetCC operation);
	// set Neg & Zero flags
	void SetNZ(uint32 result);
	// set N & Z according ot result, clear C & V
	void SetNZ_ClrCV(uint32 result);
	void SetNZ_ClrCV(uint16 result);
	void SetNZ_ClrCV(uint8 result);
	void SetNZ_ClrCV(uint32 result, uint32 sign_mask);

	// set Dx register (index 0-7) or Ax register (index 8-15)
	void SetRegister(int index, uint32 value);
	uint32 GetRegister(int index) const;

	int IsHS() const					{ return !Carry(); }
	int IsHI() const					{ return !Zero() && !Carry(); }
	int IsLO() const					{ return Carry(); }
	int IsEQ() const					{ return Zero(); }
	int IsGE() const					{ return Negative() && Overflow() || !Negative() && !Overflow(); }
	int IsGT() const					{ return !Zero() && (Negative() && Overflow() || !Negative() && !Overflow()); }
	int IsLE() const					{ return Zero() || Negative() && !Overflow() || !Negative() && Overflow(); }
	int IsLS() const					{ return Zero() || Carry(); }
	int IsLT() const					{ return Negative() && !Overflow() || !Negative() && Overflow(); }
	int IsMI() const					{ return Negative(); }
	int IsNE() const					{ return !Zero(); }
	int IsPL() const					{ return !Negative(); }
	int IsVC() const					{ return !Overflow(); }
	int IsVS() const					{ return Overflow(); }
	int IsCC() const					{ return !Carry(); }
	int IsCS() const					{ return Carry(); }

	typedef std::function<bool (uint32 addr, int access_size, uint32& ret_val, bool read)> PeripheralCallback;
	void SetSimulatorCallback(const PeripheralCallback& io);
	void SetPeripheralCallback(const PeripheralCallback& io);

	void SetSimulatorIOArea(uint32 simulator_io_area);
	uint32 GetSimulatorIOArea() const;
	cf::uint32 SimRead(cf::SimPort port);
	void SimWrite(cf::SimPort port, cf::uint32 value);

	typedef std::function<bool (uint32 address, CpuExceptions vector, uint32 pc)> ExceptionCallback;
	void SetExceptionCallback(const ExceptionCallback& ex);

	// pass interrupt requests to system integration module
	void InterruptAssert(int interrupt_source, CpuExceptions vector);
	void InterruptClear(int interrupt_source);

	void SetICM(std::vector<InterruptController*> icms);

	//TODO:
	// approx cycle count for running program, increased continually with each executed instruction
	uint32 CyclesTaken() const;
	uint32 ExecutedInstructions() const;
	void ZeroStats();

	//temporarily:
	//std::vector<uint8>& GetRAMRawPointer() { return memory_banks_.at(0).mem_; }

	enum : uint32 { MBAR_WINDOW= 0x10000 };
	enum { MAX_MEM_BANKS= 8 };

	// number of memory banks configured/available
	std::size_t GetMemoryBankCount() const;

	// what to do when simulator encounters exception 'ex':
	// stop = true -> stop execution
	// stop = false -> go to exception handler
	void ExceptionHandling(CpuExceptions ex, bool stop);

private:
	void CalcFlags();

	CPU cpu_;
	uint16 current_opcode_;
	InstructionMap instr_map_;
	PeripheralCallback peripheral_io_;
	PeripheralCallback simulator_io_;
	bool halted_;
	ExceptionCallback exception_callback_;
	uint32 current_opcode_addr_;
	std::vector<InterruptController*> icms_;	// interrupt controller module, if any (non-owning pointers)
	uint32 simulator_peripherals_;				// simulator i/o area, not part of any real MCU
	bool exception_notify_[EX_SIZE];			// which notifications are reported to the simulator
	uint32 cycles_;
	uint32 instructions_;
	bool continue_on_exceptions_;
	struct Memory
	{
		Memory();
		Memory(std::string name, uint32 base, uint32 end, cf::MemoryAccess access);

		uint32 base_;							// base address
		uint32 end_;							// last valid byte
		std::vector<uint8> mem_;				// memory buffer (could be empty)
		cf::MemoryAccess access_;				// access type
		std::string name_;						// name
	};
	std::vector<Memory> memory_banks_;
};


// memory source for disassembly
class InstrPointer
{
public:
	InstrPointer(const Context& ctx, uint32 addr);

	uint16 OpCode() const;
	bool ValidMemory() const;

	template<class STENCIL>
	void OpCode(STENCIL& s) const
	{
		s.opcode = opcode_;
	}

	uint32 GetCurAddress() const;
	uint32 GetInstrAddress() const;
	int GetWords() const;
	uint16 GetNextWord();
	int32 GetNextSWord();
	uint32 GetNextLongWord();
	uint32 GetExt() const;

private:
	const Context& ctx_;
	uint32 addr_;
	uint32 base_;
	uint16 opcode_;
	uint16 extw_;
	uint32 extl_;
	int words_;
	bool valid_memory_;
};

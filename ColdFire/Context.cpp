/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Context.h"
#include "Instruction.h"
#include "InstructionRepository.h"
#include <assert.h>
#include "Exceptions.h"

namespace {
	bool NoIO(uint32 addr, int access_size, uint32& ret_val, bool)
	{ return false; }

	bool EmptyIO(uint32 addr, int access_size, uint32& ret_val, bool read)
	{
		if (read)
			ret_val = 0;
		return true;
	}
}


Context::Context(ISA isa) : instr_map_(isa), cpu_(isa)
{
	current_opcode_ = 0;
	halted_ = false;
	cycles_ = 0;
	instructions_ = 0;
	continue_on_exceptions_ = false;
	peripheral_io_ = std::bind(&EmptyIO, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	simulator_io_ = &NoIO;
	current_opcode_addr_ = 0;
	// location of 0x1000 IO window for communication with simulator
	// this address is hard-coded in a monitor program
	simulator_peripherals_ = 0xffffa000;
	// reserve empty memory banks to prevent relocations when they are being defined
	memory_banks_.reserve(MAX_MEM_BANKS);

	cpu_.SetContext(*this);

	std::fill_n(exception_notify_, array_count(exception_notify_), true);
	// by default exclude TRAPs and interrupts from reporting
	for (int i= EX_Trap_0; i <= EX_Trap_15; ++i)
		exception_notify_[i] = false;
	for (int i= EX_InterruptLevel_1; i <= EX_InterruptLevel_7; ++i)
		exception_notify_[i] = false;
	for (int i= EX_DeviceSpecificStart; i < EX_SIZE; ++i)
		exception_notify_[i] = false;
}


Context::~Context()
{}


void Context::ExceptionHandling(CpuExceptions ex, bool stop)
{
	if (ex < EX_SIZE && ex > EX_InitialPC)
		exception_notify_[ex] = stop;
	else
	{ assert(false); }
}


void Context::SetIsa(ISA isa)
{
	if (cpu_.GetISA() != isa)
	{
		instr_map_.Build(isa);
		cpu_.SetISA(isa);
	}
}

ISA Context::GetIsa() const
{
	return cpu_.GetISA();
}


void Context::SetSimulatorCallback(const PeripheralCallback& io)
{
	simulator_io_ = io;
}

void Context::SetSimulatorIOArea(uint32 simulator_io_area)
{
	simulator_peripherals_ = simulator_io_area;
}

cf::uint32 Context::GetSimulatorIOArea() const
{
	return simulator_peripherals_;
}

cf::uint32 Context::SimRead(cf::SimPort port)
{
	uint32 ret_val= 0;
	simulator_io_(simulator_peripherals_ + static_cast<uint32>(port), 4, ret_val, true);
	return ret_val;
}

void Context::SimWrite(cf::SimPort port, cf::uint32 value)
{
	simulator_io_(simulator_peripherals_ + static_cast<uint32>(port), 4, value, false);
}


void Context::SetPeripheralCallback(const PeripheralCallback& io)
{
	peripheral_io_ = io;
}

void Context::SetExceptionCallback(const ExceptionCallback& ex)
{
	exception_callback_ = ex;
}

uint32 Context::GetDataRegister(uint32 index)
{
	if (index < 8)
		return cpu_.d_reg[index];
	else
		throw RunTimeError("invalid data register index " __FUNCTION__);
}


void Context::SetRegister(int index, uint32 value)
{
	if (index < 8)
		cpu_.d_reg[index] = value;
	else if (index < 16)
		cpu_.a_reg[index - 8] = value;
	else
		throw RunTimeError("invalid register index " __FUNCTION__);
}


uint32 Context::GetRegister(int index) const
{
	if (index < 8)
		return cpu_.d_reg[index];
	else if (index < 16)
		return cpu_.a_reg[index - 8];
	else
		throw RunTimeError("invalid register index " __FUNCTION__);
}


void Context::SetAllFlags(uint32 result, uint32 arg1, uint32 arg2, InstrSize size, bool zero_conditional, SetCC operation)
{
	uint32 sign_mask= 0;

	switch (size)
	{
	case S_LONG:
		sign_mask = uint32(1) << 31;
		break;

	case S_WORD:
		sign_mask = uint32(1) << 15;
		break;

	case S_BYTE:
		sign_mask = uint32(1) << 7;
		break;
	}

	uint32 r= result & sign_mask;
	uint32 a= arg1 & sign_mask;
	uint32 b= arg2 & sign_mask;

	if (zero_conditional)
	{
		if (result != 0)
			SetZero(false);
	}
	else
		SetZero(result == 0);

	SetNegative(r);

	if (operation == ADD)
	{
		SetOverflow(a == b && r != a);
		SetCarry(a && b || (!r && a != b));
	}
	else if (operation == NEG)
	{
		SetOverflow(r == a);	// todo: verify for both neg & negx
		SetCarry(result != 0);
	}
	else
	{
		// SUB or CMP: result = arg2 - arg1 (r : b  : a)
		SetOverflow(a != b && r != b);
		SetCarry(a && !b || (r && a == b));
	}

	if (operation != CMP)
		SetExtend(Carry());
}


void Context::SetNZ(uint32 result)
{
	SetZero(result == 0);
	const uint32 sign_mask= uint32(1) << 31;
	SetNegative(result & sign_mask);
}


void Context::SetNZ_ClrCV(uint32 result, uint32 sign_mask)
{
	SetZero(result == 0);
	SetNegative(result & sign_mask);
	SetCarry(false);
	SetOverflow(false);
}

void Context::SetNZ_ClrCV(uint32 result)
{
	const uint32 sign_mask= uint32(1) << 31;
	SetNZ_ClrCV(result, sign_mask);
}

void Context::SetNZ_ClrCV(uint16 result)
{
	const uint32 sign_mask= uint32(1) << 15;
	SetNZ_ClrCV(result, sign_mask);
}

void Context::SetNZ_ClrCV(uint8 result)
{
	const uint32 sign_mask= uint32(1) << 7;
	SetNZ_ClrCV(result, sign_mask);
}


int InstrSizeToAccessSize(InstrSize size)
{
	switch (size)
	{
	case S_LONG:	return 4;
	case S_WORD:	return 2;
	case S_BYTE:	return 1;
	default:		return 0;
	}
}


uint32 Context::ReadFromAddress(const DecodedAddress& da, InstrSize size, bool disable_io/*= false*/) const
{
	switch (da.type)
	{
	case DecodedAddress::RAM:		// RAM
	case DecodedAddress::FLASH:		// ROM
	case DecodedAddress::REGISTER:	// not really a CF memory, but a register
		switch (size)
		{
		case S_BYTE:
			// note: this happens to work for 'REGISTER's in little endian machines
			return *static_cast<const uint8*>(da.address);
		case S_WORD:
			// CF allows misalligned reads/writes apart from opcode fetch
			if (da.big_endian)
			{
				const uint8* c= static_cast<const uint8*>(da.address);
				return uint16(c[0]) << 8 | uint16(c[1]);
			}
			else
			{
				const uint16* c= static_cast<const uint16*>(da.address);
				return *c;
			}
		case S_LONG:
			// CF allows misalligned reads/writes
			if (da.big_endian)
			{
				const uint8* c= static_cast<const uint8*>(da.address);
				return uint32(c[0]) << 24 | uint32(c[1]) << 16 | uint32(c[2]) << 8 | uint32(c[3]);
			}
			else
				return *static_cast<const uint32*>(da.address);
		}
		throw RunTimeError("Illegal size " __FUNCTION__);

	case DecodedAddress::NONE:
		return 0;

	case DecodedAddress::PERIPHERALS:
		if (disable_io)
			throw MemoryAccessException(da.cf_addr);
		{
			assert(da.big_endian);
			assert(size != S_NA);
			uint32 val= 0;
			if (!peripheral_io_(da.cf_addr, InstrSizeToAccessSize(size), val, true))
				throw MemoryAccessException(da.cf_addr);
			return val;
		}

	case DecodedAddress::SIMULATOR_IO:
		if (size == S_NA)
			throw RunTimeError("Illegal size in " __FUNCTION__);
		if (disable_io)
			throw MemoryAccessException(da.cf_addr);

		assert(da.big_endian);
		assert(size != S_NA);
		uint32 val= 0;
		if (!simulator_io_(da.cf_addr, InstrSizeToAccessSize(size), val, true))
			throw MemoryAccessException(da.cf_addr);
		return val;
	}

	throw RunTimeError("Illegal type in " __FUNCTION__);
}


void Context::WriteToAddress(const DecodedAddress& da, uint32 value, InstrSize size)
{
	switch (da.type)
	{
	case DecodedAddress::RAM:
	case DecodedAddress::REGISTER:
		switch (size)
		{
		case S_BYTE:
			// note: this happens to work for 'REGISTER's in little endian machines
			*static_cast<uint8*>(da.address) = static_cast<uint8>(value);
			break;

		case S_WORD:
			// CF allows misalligned reads/writes
			if (da.big_endian)
			{
				uint8* c= static_cast<uint8*>(da.address);
				c[0] = static_cast<uint8>(value >> 8);
				c[1] = static_cast<uint8>(value);
			}
			else
			{
				uint16* c= static_cast<uint16*>(da.address);
				*c = static_cast<uint16>(value);
			}
			break;

		case S_LONG:
			// CF allows misalligned reads/writes
			if (da.big_endian)
			{
				uint8* c= static_cast<uint8*>(da.address);
				c[0] = static_cast<uint8>(value >> 24);
				c[1] = static_cast<uint8>(value >> 16);
				c[2] = static_cast<uint8>(value >> 8);
				c[3] = static_cast<uint8>(value);
			}
			else
				*static_cast<uint32*>(da.address) = value;
			break;

		default:
			throw RunTimeError("Illegal size " __FUNCTION__);
		}
		break;

	case DecodedAddress::FLASH:
		assert(size != S_NA);
		// regular write attempt to flash memory should cause exception
		//TODO: distinguish form invalid memory adresses if needed
		throw MemoryAccessException(da.cf_addr);

	case DecodedAddress::NONE:
		break;	// no op

	case DecodedAddress::PERIPHERALS:
		if (size == S_NA)
			throw RunTimeError("Illegal size in " __FUNCTION__);
		assert(da.big_endian);
		assert(size != S_NA);
		if (!peripheral_io_(da.cf_addr, InstrSizeToAccessSize(size), value, false))
			throw MemoryAccessException(da.cf_addr);
		break;

	case DecodedAddress::SIMULATOR_IO:
		if (size == S_NA)
			throw RunTimeError("Illegal size in " __FUNCTION__);
		assert(da.big_endian);
		assert(size != S_NA);
		if (!simulator_io_(da.cf_addr, InstrSizeToAccessSize(size), value, false))
			throw MemoryAccessException(da.cf_addr);
		break;

	default:
		throw RunTimeError("Illegal type in " __FUNCTION__);
	}
}


uint16 Context::DecodeSrcWordValue(uint16 mode_reg, InstrSize size, int& words)
{
	DecodedAddress da= DecodeMemoryAddress(mode_reg, size, words);
	return static_cast<uint16>(ReadFromAddress(da, S_WORD));
}


void Context::DecodeAndSetDestByte(uint16 mode_reg, InstrSize size, int& words, uint8 src)
{
	DecodedAddress da= DecodeMemoryAddress(mode_reg, size, words);

	if (size == S_BYTE)
		WriteToAddress(da, src, size);
	else
		throw RunTimeError("illegal size combination " __FUNCTION__);
}


void Context::DecodeAndSetDestWord(uint16 mode_reg, InstrSize size, int& words, uint16 src)
{
	DecodedAddress da= DecodeMemoryAddress(mode_reg, size, words);

	if (size == S_WORD)
		WriteToAddress(da, src, size);
	else if (size == S_LONG)	// this may happen for A register destination
		WriteToAddress(da, int32(int16(src)), size); // note: sign extended to long
	else
		throw RunTimeError("illegal size combination " __FUNCTION__);
}


void Context::DecodeAndSetDestLongWord(uint16 mode_reg, InstrSize size, int& words, uint32 src)
{
	DecodedAddress da= DecodeMemoryAddress(mode_reg, size, words);

	if (size == S_LONG)
		WriteToAddress(da, src, size);
	else
		throw RunTimeError("illegal size combination " __FUNCTION__);
}

// return single byte based on addressing mode, 'words' is amount of words to move PC by
uint8 Context::DecodeSrcByteValue(uint16 mode_reg, InstrSize size, int& words)
{
	DecodedAddress da= DecodeMemoryAddress(mode_reg, size, words);
	return static_cast<uint8>(ReadFromAddress(da, S_BYTE));
}


uint32 Context::DecodeSrcLongWordValue(uint16 mode_reg, InstrSize size, int& words)
{
	DecodedAddress da= DecodeMemoryAddress(mode_reg, size, words);
	return ReadFromAddress(da, S_LONG);
}


void Context::EnterException(CpuExceptions vector, uint32 address)
{
	if (!continue_on_exceptions_ && exception_notify_[vector] && exception_callback_ != nullptr)
	{
		// inform debugger about exception
		if (exception_callback_(address, vector, current_opcode_addr_))
		{
			// handled by debugger, stop execution now
			throw ExceptionReported();
		}
	}

	cpu_.EnterException(vector, current_opcode_addr_);
}


const Instruction* Context::ExecuteInstruction(bool continue_on_exceptions)
{
	if (halted_)
		return nullptr;

	const Instruction* i= nullptr;

	current_opcode_addr_ = cpu_.pc;
	continue_on_exceptions_ = continue_on_exceptions;

	try
	{
		if (current_opcode_addr_ & 1)
		{
			// misaligned memory access when reading opcode
			EnterException(EX_AccessError, current_opcode_addr_);
			return i;
		}

		current_opcode_ = GetNextPCWord();

		i = instr_map_[current_opcode_];

		if (i == nullptr)
		{
			if ((current_opcode_ & 0xf000) == 0xa000)
				EnterException(EX_UnimplementedLineAOpcode, cpu_.pc);
			else if ((current_opcode_ & 0xf000) == 0xf000)
				EnterException(EX_UnimplementedLineFOpcode, cpu_.pc);
			else
				EnterException(EX_IllegalInstruction, cpu_.pc);

			return i;
		}

		if (i->Privileged() && !cpu_.Supervisor())
			EnterException(EX_PrivilegeViolation, cpu_.pc);
		else
			i->Execute(*this);

		cycles_ += i->Cycles();	// TODO: take addressing modes into account
		instructions_++;

		// todo: find right placement for trace
		if (cpu_.Trace())
			EnterException(EX_Trace, cpu_.pc);
	}
	catch (MemoryAccessException& ex)
	{
		EnterException(EX_AccessError, ex.bad_address);
	}
	catch (AddressingModeException&)
	{
		EnterException(EX_AddressError, current_opcode_addr_);
	}

	return i;
}


bool Context::IsExecutionHalted() const
{
	return halted_;
}

void Context::HaltExecution(bool halt)
{
	halted_ = halt;
}


void Context::EnterStopState()
{
	//todo
	// stop normal execution
	// only interrupts enabled
}


void Context::PushLongWord(uint32 addr)
{
	const uint32 size= 4;
//	//todo: check alignment and correct it if needed?
	DecodedAddress da= GetMemoryAddress(cpu_.a_reg[7] - size, size);
	WriteToAddress(da, addr, S_LONG);
	cpu_.a_reg[7] -= size;
}


uint32 Context::PullLongWord()
{
	uint32 val= GetLongWord(cpu_.a_reg[7]);
	const uint32 size= 4;
	cpu_.a_reg[7] += size;
	return val;
}


uint16 Context::PullWord()
{
	uint16 val= GetWord(cpu_.a_reg[7]);
	const uint32 size= 2;
	cpu_.a_reg[7] += size;
	return val;
}


namespace {
	const uint32 ACCESS_SIZES[]= { 0, 1, 2, 4 };
}

DecodedAddress Context::GetMemoryAddress(uint32 addr, InstrSize size) const
{
	return GetMemoryAddress(addr, ACCESS_SIZES[size]);
}


DecodedAddress Context::GetMemoryAddress(uint32 addr, uint32 size) const
{
	return GetMemoryAddress(addr, size, false);
}


DecodedAddress Context::GetMemoryAddress(uint32 addr, uint32 size, bool no_throw) const
{
	bool invalid_size= false;
	uint32 end_addr= addr + size;

	// address verification is simplistic; it is assumed that entire access from addr to addr+size has to fit
	// in a single device/area/bank; if it doesn't it's rejected even if it was to work on a real hardware

	if (end_addr >= addr)
	{
		// simulator's I/O, not part of any real MCU
		const uint32 sim_area_size= 0x1000;
		if (addr >= simulator_peripherals_ && addr < simulator_peripherals_ + sim_area_size)
		{
			if (end_addr >= simulator_peripherals_ + sim_area_size)
				invalid_size = true;
			else
				return DecodedAddress(addr, DecodedAddress::SIMULATOR_IO);
		}
		else
		{
			// MCU slave peripherals
			uint32 mbar= cpu_.mbar;
			if (addr >= mbar && addr < mbar + MBAR_WINDOW)
			{
				if (end_addr >= mbar + MBAR_WINDOW)
					invalid_size = true;
				else
					return DecodedAddress(addr, DecodedAddress::PERIPHERALS);
			}
			else
			{
				// memory banks
				for (auto& m : memory_banks_)
					if (addr >= m.base_ && addr <= m.end_)
					{
						if (end_addr > m.end_ + 1)
						{
							invalid_size = true;
							break;
						}

						switch (m.access_)
						{
						case cf::MemoryAccess::Normal:
							ASSERT(m.mem_.size() == m.end_ - m.base_ + 1);
							return DecodedAddress(&m.mem_[addr - m.base_], addr, DecodedAddress::RAM);

						case cf::MemoryAccess::ReadOnly:
							ASSERT(m.mem_.size() == m.end_ - m.base_ + 1);
							return DecodedAddress(&m.mem_[addr - m.base_], addr, DecodedAddress::FLASH);

						case cf::MemoryAccess::Null:
							return DecodedAddress(static_cast<void*>(nullptr), addr, DecodedAddress::NONE);
						}
					}
			}
		}
	}
	else
		invalid_size = true;

//	ASSERT(!invalid_size);

	// todo: report invalid size too

	if (no_throw)
		return DecodedAddress(static_cast<void*>(nullptr), addr, DecodedAddress::INVALID);

	if (invalid_size)
		throw MemoryAccessException(end_addr);	// this could be misleading
	else
		throw MemoryAccessException(addr);
}


uint16 Context::ReadMemoryWord(uint32 addr) const
{
	DecodedAddress da= GetMemoryAddress(addr, S_WORD);
	// read ignoring peripherals
	return static_cast<uint16>(ReadFromAddress(da, S_WORD, true));
}


uint32 Context::ReadMemoryLongWord(uint32 addr) const
{
	DecodedAddress da= GetMemoryAddress(addr, S_LONG);
	// read ignoring peripherals
	return ReadFromAddress(da, S_LONG, true);
}


// retrieve word from CPU's memory or throw if address is invalid
uint16 Context::GetWord(uint32 addr) const
{
	DecodedAddress da= GetMemoryAddress(addr, S_WORD);
	return static_cast<uint16>(ReadFromAddress(da, S_WORD));
}

int16 Context::GetSWord(uint32 addr) const
{
	return static_cast<int16>(GetWord(addr));
}

uint32 Context::GetLongWord(uint32 addr) const
{
	DecodedAddress da= GetMemoryAddress(addr, S_LONG);
	return ReadFromAddress(da, S_LONG);
}


Context::Memory::Memory()
{
	base_ = 0;
	end_ = 0 - 1;
	access_ = cf::MemoryAccess::Null;
}


Context::Memory::Memory(std::string name, uint32 base, uint32 end, cf::MemoryAccess access)
	: name_(name)
{
	base_ = base;
	end_ = end;
	access_ = access;
	if (access != cf::MemoryAccess::Null)
		mem_.resize(static_cast<size_t>(end) - base + 1, 0);
}


void Context::InitMemory(std::string name, uint32 base_addr, uint32 mem_size, int bank, cf::MemoryAccess access)
{
	if (bank < 0 || bank >= MAX_MEM_BANKS)
		throw RunTimeError("Up to 8 banks supported " __FUNCTION__);

	if (static_cast<size_t>(bank) >= memory_banks_.size())
		memory_banks_.resize(bank + 1);

	if (mem_size == 0 || base_addr + mem_size < base_addr)
		throw RunTimeError("Invalid bank memory size " __FUNCTION__);

	memory_banks_[bank] = Memory(name, base_addr, base_addr + mem_size - 1, access);
}


std::size_t Context::GetMemoryBankCount() const
{
	return memory_banks_.size();
}


void Context::ClearMemory(int bank)
{
	size_t index= bank;
	if (index < memory_banks_.size())
	{
		auto& mem= memory_banks_[index];
		if (mem.access_ != cf::MemoryAccess::Null)
			std::fill(begin(mem.mem_), end(mem.mem_), 0);
	}
}


void Context::ZeroMemory(uint32 address, uint32 size)
{
	// validate begin and end; this fn will throw if valid size is exceeded
	DecodedAddress da= GetMemoryAddress(address, size);

	if (size > 0)
	{
		if (da.type == DecodedAddress::RAM || da.type == DecodedAddress::FLASH)
			memset(da.address, 0, size);
		else
			throw RunTimeError("Invalid memory type for clearing " __FUNCTION__);
	}
}


cf::uint32 Context::ReadMemory(cf::uint8* dest_buf, cf::uint32 address, cf::uint32 length)
{
	//TODO: improve, for now it works, but it's slow

	if (dest_buf == nullptr)
		return 0;

	DecodedAddress area= GetMemoryAddress(address, length, true);
	if (area.type == DecodedAddress::RAM || area.type == DecodedAddress::FLASH)
	{
		// requested block fits in a single memory bank; copy it

		memcpy(dest_buf, area.address, length);
	}
	else
	{
		// do it a (very) slow way, byte by byte

		for (long long i= 0; i < length; ++i)
		{
			DecodedAddress da= GetMemoryAddress(address++, 0, true);

			switch (da.type)
			{
			case DecodedAddress::RAM:
			case DecodedAddress::FLASH:
				// only read from RAM/ROM
				*dest_buf++ = static_cast<uint8>(ReadFromAddress(da, S_BYTE));
				break;

			default:
				// ignore all special areas
				*dest_buf++ = 0xff;
				break;
			}

			if (address == 0)
				return static_cast<cf::uint32>(i + 1);	// address wrapped around, exit
		}
	}

	return length;
}


cf::MemoryBankInfo Context::GetMemoryBankInfo(int bank) const
{
	size_t index= bank;
	if (index < memory_banks_.size())
	{
		auto& mem= memory_banks_[index];
		return cf::MemoryBankInfo(mem.name_, mem.access_, mem.base_, mem.end_);
	}

	return cf::MemoryBankInfo();
}


void Context::CopyProgram(const uint8* begin, const uint8* end, uint32 start_address)
{
	size_t size= end - begin;

	// validate begin and end
	DecodedAddress da= GetMemoryAddress(start_address, uint32(size));

	if (size > 0)
	{
		if (da.type == DecodedAddress::RAM || da.type == DecodedAddress::FLASH)
			memcpy(da.address, begin, size);
		else
			throw RunTimeError("Invalid memory type for copying program to; " __FUNCTION__);
	}
}


void Context::CopyProgram(const std::vector<uint8>& program, uint32 start)
{
	if (!program.empty())
		CopyProgram(&program.front(), &program.back() + 1, start);
}


void Context::CopyProgram(const std::vector<uint16>& program, uint32 prg_base)
{
	const size_t count= program.size();
	if (count == 0)
		return;

	std::vector<uint8> copy(count * 2, 0);

	auto p= const_cast<uint8*>(copy.data());
	for (size_t i= 0; i < count; ++i)
	{
		uint16 word= program[i];
		*p++ = static_cast<char>(word >> 8);
		*p++ = static_cast<char>(word & 0xff);
	}

	CopyProgram(copy, prg_base);
}


void Context::InterruptAssert(int interrupt_source, CpuExceptions vector)
{
	// route it to the system integration module or interrupt controller module(s)
	for (auto icm : icms_)
		icm->InterruptAssert(interrupt_source, vector);
}


void Context::InterruptClear(int interrupt_source)
{
	for (auto icm : icms_)
		icm->InterruptClear(interrupt_source);
}


void Context::SetICM(std::vector<InterruptController*> icms)
{
	icms_ = icms;
}


uint32 Context::CyclesTaken() const
{
	return cycles_;
}


uint32 Context::ExecutedInstructions() const
{
	return instructions_;
}


void Context::ZeroStats()
{
	cycles_ = 0;
	instructions_ = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////


uint16 CPU::GetSR()
{
	uint16 reg= sr.sr & uint16(~0x1f);

	if (zero) reg |= cf::SR_ZERO;
	if (carry) reg |= cf::SR_CARRY;
	if (extend) reg |= cf::SR_EXTEND;
	if (negative) reg |= cf::SR_NEGATIVE;
	if (overflow) reg |= cf::SR_OVERFLOW;

	return reg;
}


void CPU::SetCCR(uint16 ccr)
{
	sr.sr &= 0xff00;
	sr.sr |= ccr & 0xff;

	// change flags
	zero = !!(ccr & cf::SR_ZERO);
	carry = !!(ccr & cf::SR_CARRY);
	extend = !!(ccr & cf::SR_EXTEND);
	negative = !!(ccr & cf::SR_NEGATIVE);
	overflow = !!(ccr & cf::SR_OVERFLOW);
}


void CPU::SetSR(uint16 new_sr)
{
	if (sr.s && !(new_sr & cf::SR_SUPERVISOR))
		ExitSupervisorState();
	else if (!sr.s && (new_sr & cf::SR_SUPERVISOR))
		EnterSupervisorState();	// this can happen when use changes SR register manually

	sr.sr = new_sr;
	sr.reserved1 = 0;
	sr.reserved2 = 0;
	sr.reserved3 = 0;

	// change flags
	SetCCR(new_sr);
}


void CPU::EnterSupervisorState()
{
	if (!sr.s && Architecture(isa_) != ISA::A)
		std::swap(other_a7, a_reg[7]);
	sr.s = 1;
	sr.t = 0;
}


void CPU::ExitSupervisorState()
{
	if (sr.s && Architecture(isa_) != ISA::A)
		std::swap(other_a7, a_reg[7]);
	sr.s = 0;
}


void CPU::EnterException(CpuExceptions vector, uint32 opcode_addr)
{
	try
	{
		uint32 orig_sr= GetSR();

		uint32 addr= ctx_->GetLongWord(vbr + vector * 4);

		// swap SP and SSP if needed
		EnterSupervisorState();

		uint32& sp= a_reg[7];

		uint16 misalignment= sp & 3;

		// align stack pointer
		if (misalignment)
			sp = sp & ~3;

		// construct exception stack frame

		// push correct PC:
		// In some exception types, the program counter (PC) in the exception stack frame contains the address
		// of the faulting instruction (fault); in others, the PC contains the next instruction to be executed (next). 
		// If the exception is caused by an FPU instruction, the PC contains the address of either the next 
		// floating-point instruction (nextFP) if the exception is pre-instruction, or the faulting instruction 
		// (fault) if the exception is post-instruction.

		switch (vector)
		{
		case EX_AccessError:
		case EX_AddressError:
		case EX_IllegalInstruction:
		case EX_DivideByZero:
		case EX_PrivilegeViolation:
		case EX_UnimplementedLineAOpcode:
		case EX_UnimplementedLineFOpcode:
		case EX_FormatError:
		case EX_FP_BranchOnUnorderedCond:
		case EX_UnsupportedInstruction:
			ctx_->PushLongWord(opcode_addr);
			break;

		default:
			ctx_->PushLongWord(pc);
			break;
		}

		uint32 frame= 0x40000000 | (misalignment << 28) | (vector << 18) | orig_sr;

		// TODO: fault status field in above format

		ctx_->PushLongWord(frame);

		sr.m = 0;
		sr.i = 0; // TODO: interrupt level

		pc = addr;

		ctx_->GetWord(pc);	// try to fetch first opcode of the exception handler routine
	}
	catch (McuException&)
	{
		// exception here enters CPU's halt state
		throw FaultOnFault();	// todo: capture original exception?
	}
}


CPU::CPU(ISA isa)
{
	isa_ = isa;
	ctx_ = nullptr;
	default_vbr_ = 0;
	default_mbar_ = 0x10000000;
	ResetRegs();
}

void CPU::SetContext(Context& ctx)
{
	ctx_ = &ctx;
}

void CPU::ResetRegs()
{
	memset(d_reg, 0, sizeof d_reg);
	memset(a_reg, 0, sizeof a_reg);

	sr.sr = 0;
	sr.s = 1;	// in supervisor state after reset
	pc = 0;
	other_a7 = 0;

	extend = carry = zero = negative = overflow = false;

	vbr = default_vbr_;

	mbar = default_mbar_ & CPU::MBAR_ADDR_MASK;	// MCU-specific address

	rambar1 = 0;	// currently those are not used
	rambar2 = 0;
	rombar1 = 0;
	rombar2 = 0;

	// todo: load hardware config into D0/D1
	//
	d_reg[0] = 0xCF204000;	// ColdFire with DIV hardware
	switch (Architecture(isa_))
	{
	case ISA::A:		d_reg[0] |= 0x00; break;
	case ISA::A_PLUS:	d_reg[0] |= 0x80; break;
	case ISA::B:		d_reg[0] |= 0x10; break;
	case ISA::C:		d_reg[0] |= 0x20; break;
	default:
		assert(false);
		break;
	}

	//todo: D1 - Local Memory Configuration
	//

	//todo: load PC & SP?
	if (ctx_)
	{
		other_a7 = a_reg[7] = ctx_->GetLongWord(0);
		pc = ctx_->GetLongWord(4);
	}
}

void CPU::SetDefaults(cf::Register reg, uint32 value)
{
	switch (reg)
	{
	case cf::R_MBAR:	default_mbar_ = value; break;
	case cf::R_VBR:		default_vbr_ = value; break;

	default:
		assert(false);
		break;
	}
}

void CPU::SetUSP(uint32 addr)
{
	other_a7 = addr;
}


uint32 CPU::GetUSP() const
{
	return other_a7;
}


void CPU::SetStackPointers(uint32 user, uint32 system)
{
	if (sr.s)
	{
		other_a7 = user;
		a_reg[7] = system;
	}
	else
	{
		other_a7 = system;
		a_reg[7] = user;
	}
}


std::pair<uint32, uint32> CPU::GetStackPointers() const
{
	return sr.s ? std::make_pair(other_a7, a_reg[7]) : std::make_pair(a_reg[7], other_a7);
}


uint32 Context::DecodeEffectiveAddress(uint16 mode_reg, int& ext_words)
{
	int mode= (mode_reg >> 3) & 0x7;
	int reg= mode_reg & 0x7;

	switch (mode)
	{
	case 0:		// Dn
	case 1:		// An
		throw AddressingModeException(mode_reg);

	case 2:		// (An)
		return cpu_.a_reg[reg];

	case 3:		// (An)+
		throw AddressingModeException(mode_reg);

	case 4:		// -(An)
		throw AddressingModeException(mode_reg);

	case 5:		// d16(An)
		{
			auto disp= SignExtendWord(GetWord(cpu_.pc + ext_words));
			++ext_words;
			return cpu_.a_reg[reg] + disp;
		}

	case 6:		// d8(An, Xn*s)
		{
			ExtensionWordFmt_DISP_REG_IDX ext;
			ext.word = GetWord(cpu_.pc + ext_words * 2);
			++ext_words;

			// scaled index register (should be signed when word size, 68k only):
			int32 index_reg= int32(ext.DA ? cpu_.a_reg[ext.reg] : cpu_.d_reg[ext.reg]) << ext.scale;
			// 8-bit signed displacement
			int32 disp= ext.displacement;
			return cpu_.a_reg[reg] + index_reg + disp;
		}

	case 7:
		switch (reg)
		{
		case 0:		// (xxxx).W
			{
				auto addr= SignExtendWord(GetWord(cpu_.pc + ext_words * 2));
				++ext_words;
				return addr;
			}

		case 1:		// (xxxxxxxx).L
			{
				uint32 addr= GetLongWord(cpu_.pc + ext_words * 2);
				ext_words += 2;
				return addr;
			}

		case 2:		// d16(PC)
			{
				auto disp= SignExtendWord(GetWord(cpu_.pc + ext_words * 2));
				++ext_words;
				return cpu_.pc + disp;
			}

		case 3:		// d8(PC, Xn*s)
			{
				ExtensionWordFmt_DISP_REG_IDX ext;
				ext.word = GetWord(cpu_.pc);
				++ext_words;

				// scaled index register (should be signed when word size, 68k only):
				//uint32 index_reg= d_reg[uint32(ext >> 12)] << ((ext >> 9) & 0x3);
				int32 index_reg= int32(ext.DA ? cpu_.a_reg[ext.reg] : cpu_.d_reg[ext.reg]) << ext.scale;
				// 8-bit signed displacement
				int32 disp= ext.displacement;
				return cpu_.pc + index_reg + disp;
			}

		case 4:		// #nnnnnnnnn
			throw AddressingModeException(mode_reg);

		default:
			throw AddressingModeException(mode_reg);

		}
		break;
	}

	throw AddressingModeException(mode_reg);
}


DecodedAddress Context::DecodeMemoryAddress(uint16 mode_reg, InstrSize& size, int& ext_words)
{
	assert(size != S_NA);

	int mode= (mode_reg >> 3) & 0x7;
	int reg= mode_reg & 0x7;

	switch (mode)
	{
	case 0:		// Dn
		return DecodedAddress(&cpu_.d_reg[reg], 0, DecodedAddress::REGISTER, false);

	case 1:		// An
		size = S_LONG;	// say it is long, to change the whole address register
		return DecodedAddress(&cpu_.a_reg[reg], 0, DecodedAddress::REGISTER, false);

	case 2:		// (An)
		return GetMemoryAddress(cpu_.a_reg[reg], size);

	case 3:		// (An)+
		{
			DecodedAddress da= GetMemoryAddress(cpu_.a_reg[reg], size);
			cpu_.a_reg[reg] += ACCESS_SIZES[size];
			return da;
		}

	case 4:		// -(An)
		{
			uint32 addr= cpu_.a_reg[reg] - ACCESS_SIZES[size];
			DecodedAddress da= GetMemoryAddress(addr, size);
			cpu_.a_reg[reg] = addr;
			return da;
		}

	case 5:		// d16(An)
		{
			auto disp= SignExtendWord(GetWord(cpu_.pc + ext_words * 2));
			++ext_words;
			return GetMemoryAddress(cpu_.a_reg[reg] + disp, size);
		}

	case 6:		// d8(An, Xn*s)
		{
			ExtensionWordFmt_DISP_REG_IDX ext;
			ext.word = GetWord(cpu_.pc + ext_words * 2);
			++ext_words;

			// scaled index register (should be signed when word size, 68k only):
			int32 index_reg= int32(ext.DA ? cpu_.a_reg[ext.reg] : cpu_.d_reg[ext.reg]) << ext.scale;
			// 8-bit signed displacement
			int32 disp= ext.displacement;
			return GetMemoryAddress(cpu_.a_reg[reg] + index_reg + disp, size);
		}

	case 7:
		switch (reg)
		{
		case 0:		// (xxxx).W
			{
				auto addr= SignExtendWord(GetWord(cpu_.pc + ext_words * 2));
				++ext_words;
				return GetMemoryAddress(addr, size);
			}

		case 1:		// (xxxxxxxx).L
			{
				uint32 addr= GetLongWord(cpu_.pc + ext_words * 2);
				ext_words += 2;
				return GetMemoryAddress(addr, size);
			}

		case 2:		// d16(PC)
			{
				auto disp= SignExtendWord(GetWord(cpu_.pc + ext_words * 2));
				++ext_words;
				return GetMemoryAddress(cpu_.pc + disp, size);
			}

		case 3:		// d8(PC, Xn*s)
			{
				ExtensionWordFmt_DISP_REG_IDX ext;
				ext.word = GetWord(cpu_.pc);
				++ext_words;

				// scaled index register (should be signed when word size, 68k only):
				int32 index_reg= int32(ext.DA ? cpu_.a_reg[ext.reg] : cpu_.d_reg[ext.reg]) << ext.scale;
				// 8-bit signed displacement
				int32 disp= ext.displacement;
				return GetMemoryAddress(cpu_.pc + index_reg + disp, size);
			}

		case 4:		// #nnnnnnnnn
			{
				DecodedAddress da= GetMemoryAddress(cpu_.pc, size);
				if (size == S_LONG)
					ext_words += 2;
				else
				{
					ext_words += 1;
					if (size == S_BYTE) // push address to the next byte of immediate word value
						da.address = static_cast<uint8*>(da.address) + 1;
				}
				return da;
			}

		default:
			throw AddressingModeException(mode_reg);

		}
		break;
	}

	assert(false);
	throw AddressingModeException(mode_reg);
}


//=================================================================================================


InstrPointer::InstrPointer(const Context& ctx, uint32 addr) : ctx_(ctx)
{
	base_ = addr;

	DecodedAddress da= ctx.GetMemoryAddress(addr, S_WORD, true);
	if (da.type != DecodedAddress::INVALID)
	{
		opcode_ = ctx.ReadMemoryWord(addr);
		valid_memory_ = true;
	}
	else
	{
		opcode_ = 0;
		valid_memory_ = false;
	}

	addr_ = addr + 2;
	words_ = 1;
	extw_ = 0;
	extl_ = 0;
}


uint16 InstrPointer::OpCode() const
{
	return opcode_;
}


bool InstrPointer::ValidMemory() const
{
	return valid_memory_;
}


uint32 InstrPointer::GetCurAddress() const
{
	return addr_;
}


uint32 InstrPointer::GetInstrAddress() const
{
	return base_;
}


int InstrPointer::GetWords() const
{
	return words_;
}


uint16 InstrPointer::GetNextWord()
{
	uint16 v= ctx_.ReadMemoryWord(addr_);
	addr_ += 2;
	words_++;
	if (words_ == 3)
		extl_ = (uint32(extw_) << 16) | v;
	extw_ = v;
	return v;
}


int32 InstrPointer::GetNextSWord()
{
	// next word sign extended to 32 bits
	return static_cast<int16>(GetNextWord());
}


uint32 InstrPointer::GetNextLongWord()
{
	uint32 v= ctx_.ReadMemoryLongWord(addr_);
	addr_ += 4;
	words_ += 2;
	extl_ = v;
	return v;
}


uint32 InstrPointer::GetExt() const
{
	if (words_ == 2)
		return extw_;
	else if (words_ == 3)
		return extl_;
	throw LogicError("no ext data in " __FUNCTION__);
}

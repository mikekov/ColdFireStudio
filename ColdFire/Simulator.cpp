/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "Simulator.h"
#include "Context.h"
#include "DebugInfo.h"
#include "Breakpoints.h"
#include "Instruction.h"
#include "Peripheral.h"
#include "PeripheralRepository.h"
#include <boost/format.hpp>
#include "HexNumber.h"
#include <boost/property_tree/info_parser.hpp>

class address_hash
{
public:
	std::size_t operator() (uint32 addr) const
	{
		// addresses are even so remove least significant bit;
		// they are random enough to be used as they are; this speeds up lookup by avoiding complicated std hashing
		return addr >> 1;
	}
};


class Breakpoints
{
	typedef std::unordered_map<uint32, cf::BreakpointType, address_hash> Map;
public:
	Breakpoints()
	{
		bp_.max_load_factor(0.7f);
		bp_.reserve(20);
	}

	cf::BreakpointType Get(uint32 address) const
	{
		auto it= bp_.find(address);
		return it == bp_.end() ? cf::BPT_NONE : it->second;
	}

	uint32 Set(uint32 address, cf::BreakpointType type)
	{
		bp_[address] = type;
		return address;
	}

	void Remove(uint32 address, cf::BreakpointType type)
	{
		bp_.erase(address);
	}

	bool Hit(uint32 pc)
	{
		if (bp_.empty())
			return false;
		auto it= bp_.find(pc);
		if (it != bp_.end() && (it->second & (cf::BPT_EXECUTE | cf::BPT_TEMP_EXEC)))
			return true;
		return false;
	}

	void ClearAll()
	{
		bp_.clear();
	}

	bool ClearTemp(uint32 pc)
	{
		auto it= bp_.find(pc);
		if (it != bp_.end())
		{
			bool temp_bp= (it->second & cf::BPT_TEMP_EXEC) != 0;

			if (it->second == cf::BPT_TEMP_EXEC)
				bp_.erase(it);
			else
				it->second = static_cast<cf::BreakpointType>(it->second & ~cf::BPT_TEMP_EXEC);

			return temp_bp;
		}
		return false;
	}

	size_t Count() const
	{
		return bp_.size();
	}

	Map::const_iterator begin() const	{ return bp_.begin(); }
	Map::const_iterator end() const		{ return bp_.end(); }

private:
	Map bp_;
};


struct Simulator::Impl
{
	Impl()
	{
		ctx_.reset(new Context(ISA::A));	// this initial state is not very important; it can be changed later
		status_ = SIM_OK;
		stop_execution_ = false;
		debug_ = nullptr;
		temp_bp_addr_to_clear_ = 0;
		ctx_->SetPeripheralCallback(std::bind(&Simulator::Impl::PeripheralsIO, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	}

	~Impl()
	{
		if (exec_.joinable())
			exec_.detach();
	}

	PeripheralDevice* AddPeripheral(const char* category, const char* version, uint16 io_area_offset, uint16 io_area_size, uint16 interrupt_source, bool trace, bool notify, PeripheralConfigData& config);

	std::unique_ptr<Context> ctx_;
	std::function<void (cf::Event ev, const EventArgs& params /*cf::uint32 param*/)> callback_;
	SimulatorStatus status_;
	std::thread exec_;
	bool stop_execution_;
	masm::DebugInfo* debug_;
	Breakpoints breakpoints_;
	boost::ptr_vector<Peripheral> peripherals_;
	std::array<uint8, Context::MBAR_WINDOW> periperals_io_area_;
	uint32 temp_bp_addr_to_clear_;

	void SendUpdate(cf::Event ev)
	{
		SendUpdate(ev, ctx_->Cpu().pc, nullptr, cf::DeviceAccess::Read);
	}

	void SendUpdate(cf::Event ev, cf::uint32 param, Peripheral* device, cf::DeviceAccess access)
	{
		try
		{
			if (callback_ != nullptr)
			{
				EventArgs args(param, device, access);
				callback_(ev, args);
			}
		}
		catch (...)
		{
			//todo: propagate state to client?
		}
	}

	enum class Condition { Run, TillRet, SingleStep };
	SimulatorStatus Run(Condition cond);
	SimulatorStatus StepOver();
	SimulatorStatus Step();
	SimulatorStatus SingleStep();

	bool CanRun() const;
	bool CannotRun() const	{ return !CanRun(); }

	bool PeripheralsIO(uint32 addr, int access_size, uint32& ret_val, bool read);

private:
	void RunThread(Condition cond);
	SimulatorStatus RunSimulation(Condition cond);
};


namespace {
	DecodedInstruction DecodeInstructionHelper(Context& ctx, uint32 addr)
	{
		DecodedInstruction d;

		try
		{
			d = ::DecodeInstruction(ctx, addr);
		}
		catch (McuException&)
		{
		}

		return d;
	}
}


bool Simulator::Impl::CanRun() const
{
	switch (status_)
	{
	case SIM_OK:
	case SIM_STOPPED:
	case SIM_BREAKPOINT_HIT:
	case SIM_EXCEPTION:
		return true;

	default:
		return false;
	}
}


Simulator::Simulator() : impl_(new Impl())
{}


Simulator::~Simulator()
{
	delete impl_;
}


// catch all exceptions and turn them into simulator return code
//
SimulatorStatus Call(const std::function<SimulatorStatus()>& fn)
{
	try
	{
		return fn();
	}
	catch (FaultOnFault&)
	{
		// todo:
		// exception during exception, stop simulator with proper info...
		return SIM_STOPPED;
	}
	catch (ExceptionReported&)
	{
		return SIM_EXCEPTION;
	}
	catch (McuException&)
	{
		return SIM_STOPPED;
	}
	catch (std::exception&)
	{
//		std::cout << ex.what();
		return SIM_INTERNAL_ERROR;
	}
	catch (...)
	{
		return SIM_INTERNAL_ERROR;
	}
}

// catch low level exceptions and turn them into RunTimeError exception that client can handle
//
void CallEx(const std::function<void()>& fn)
{
	try
	{
		return fn();
	}
	catch (MemoryAccessException& ex)
	{
		throw RunTimeError((boost::format("Memory access error at 0x%X") % ex.bad_address).str());
	}
	catch (McuException&)
	{
		throw RunTimeError("MCU exception");
	}
	// let other exceptions to fall through
}


void Simulator::CreateMemoryBank(std::string name, ::uint32 base, cf::uint32 size, int bank, cf::MemoryAccess access)
{
	impl_->ctx_->InitMemory(name, base, size, bank, access);
}


cf::MemoryBankInfo Simulator::GetMemoryBankInfo(int bank) const
{
	return impl_->ctx_->GetMemoryBankInfo(bank);
}


void Simulator::SetMemory(cf::uint32 address, const cf::uint8* begin, const cf::uint8* end)
{
	CallEx([&]
	{
		impl_->ctx_->CopyProgram(begin, end, address);
	});

	impl_->SendUpdate(cf::E_MEMORY);
}


void Simulator::ZeroMemory(cf::uint32 address, cf::uint32 size)
{
	CallEx([&]
	{
		impl_->ctx_->ZeroMemory(address, size);
	});

	impl_->SendUpdate(cf::E_MEMORY);
}


cf::uint32 Simulator::ReadMemory(cf::uint8* dest_buf, cf::uint32 address, cf::uint32 length)
{
	// this method doesn't throw

	return impl_->ctx_->ReadMemory(dest_buf, address, length);
}


void Simulator::SetInitialStackAndPC(cf::uint32 reset_start)
{
	auto mem= impl_->ctx_->GetMemoryBankInfo(0);
	ASSERT(mem.Size() > 0);
	uint32 sp= static_cast<uint32>(mem.End() + 1);	// can overflow and wrap around; it's fine

	const uint32 size= 2 * 4;
	uint8 buffer[size];
	uint8* c= buffer;
	c[0] = static_cast<uint8>(sp >> 24);
	c[1] = static_cast<uint8>(sp >> 16);
	c[2] = static_cast<uint8>(sp >> 8);
	c[3] = static_cast<uint8>(sp);
	c[4] = static_cast<uint8>(reset_start >> 24);
	c[5] = static_cast<uint8>(reset_start >> 16);
	c[6] = static_cast<uint8>(reset_start >> 8);
	c[7] = static_cast<uint8>(reset_start);

	// validate memory at the base of VBR
	DecodedAddress da= impl_->ctx_->GetMemoryAddress(impl_->ctx_->Cpu().vbr, size);
	// store new values for SP and PC
	memcpy(da.address, buffer, size);
}


void Simulator::InitSP()
{
	auto mem= impl_->ctx_->GetMemoryBankInfo(0);
	ASSERT(mem.Size() > 0);
	uint32 system= static_cast<uint32>(mem.End() + 1);	// can overflow and wrap around; it's fine
	uint32 user= system;
	if (Architecture(impl_->ctx_->Cpu().GetISA()) != ISA::A) // do we have OTHER_A7?
	{
		// reserve space for system; user stack below
		user = system - 0x100;
	}
	impl_->ctx_->Cpu().SetStackPointers(user, system);
}


void Simulator::SetProgram(const cf::BinaryProgram& code)
{
	CallEx([&]
	{
		cf::BinaryProgramRange it(code);

		while (it)
		{
			auto part= it.Fragment();
			impl_->ctx_->CopyProgram(part.data(), part.data() + part.size(), it.Address());

			++it;
		}
		impl_->ctx_->Cpu().pc = code.GetProgramStart();
	});

	impl_->SendUpdate(cf::E_PROG_SET);
}


void Simulator::SetProgram(cf::uint32 program_addr, const cf::uint8* begin, const cf::uint8* end, cf::uint32 start_addr)
{
	CallEx([&]
	{
		impl_->ctx_->CopyProgram(begin, end, program_addr);
		impl_->ctx_->Cpu().pc = start_addr;
	});

	impl_->SendUpdate(cf::E_PROG_SET);
}


void Simulator::SetProgram(cf::uint32 program_addr, const std::vector<cf::uint16>& code, cf::uint32 start_addr)
{
	CallEx([&]
	{
		impl_->ctx_->CopyProgram(code, program_addr);
		impl_->ctx_->Cpu().pc = start_addr;
	});

	impl_->SendUpdate(cf::E_PROG_SET);
}


void Simulator::SetIsa(ISA isa)
{
	impl_->ctx_->SetIsa(isa);
}


ISA Simulator::GetIsa() const
{
	return impl_->ctx_->GetIsa();
}


void Simulator::Reset()
{
	impl_->ctx_->Cpu().ResetRegs();
	//todo: load PC and SP?

	for (auto& p : impl_->peripherals_)
		p.DoReset(*impl_->ctx_);

	impl_->ctx_->HaltExecution(false);

	impl_->status_ = SIM_STOPPED;

	for (auto& p : impl_->peripherals_)
		if (p.NotifyClient())
			impl_->SendUpdate(cf::E_DEVICE_IO, 0, &p, cf::DeviceAccess::Reset);
}


SimulatorStatus Simulator::BreakExecution()
{
	impl_->stop_execution_ = true;
	return impl_->status_;
}


SimulatorStatus Simulator::AbortExecution()
{
	assert(impl_->status_ != SIM_IS_RUNNING);

	impl_->ctx_->HaltExecution(true);
	impl_->status_ = SIM_FINISHED;
	impl_->SendUpdate(impl_->status_ == SIM_IS_RUNNING ? cf::E_RUNNING : cf::E_EXEC_STOPPED);
	return impl_->status_;
}


SimulatorStatus Simulator::OneStep()	// exec one instruction, enter exceptions/traps as needed
{
	return Call(std::bind(&Impl::SingleStep, impl_));
}


SimulatorStatus Simulator::Step()
{
	return Call(std::bind(&Impl::Step, impl_));
}


SimulatorStatus Simulator::Impl::Step()
{
	// TODO: step over TRAPs?
	// TODO: step over other exceptions...
	return Run(Impl::Condition::SingleStep);
}


SimulatorStatus Simulator::Impl::SingleStep()
{
	// todo: delegate this to RunSimulation()

	if (CannotRun())
		return status_;

	try
	{
		ctx_->ExecuteInstruction(true);
		status_ = ctx_->IsExecutionHalted() ? SIM_FINISHED : SIM_STOPPED;
	}
	catch (ExceptionReported&)
	{
		status_ = SIM_EXCEPTION;
	}

	SendUpdate(cf::E_EXEC_STOPPED);

	return status_;
}


SimulatorStatus Simulator::Run()
{
	return Call(std::bind(&Impl::Run, impl_, Impl::Condition::Run));
//	return impl_->Run(Impl::Condition::Run);
}


SimulatorStatus Simulator::StepOut()
{
	return Call(std::bind(&Impl::Run, impl_, Impl::Condition::TillRet));
//	return impl_->Run(Impl::Condition::TillRet);
}


SimulatorStatus Simulator::Impl::Run(Condition cond)
{
	if (CannotRun())
		return status_;

	if (exec_.joinable())
		exec_.detach();

	stop_execution_ = false;
	exec_.swap(std::thread(&Simulator::Impl::RunThread, this, cond));

	return status_;
}


void Simulator::Impl::RunThread(Condition cond)
{
	try
	{
		status_ = SIM_IS_RUNNING;

		SendUpdate(cf::E_RUNNING);

		status_ = RunSimulation(cond);

		SendUpdate(cf::E_EXEC_STOPPED);

		if (exec_.joinable())
			exec_.detach();
	}
	catch (...)
	{
		status_ = SIM_INTERNAL_ERROR;
	}
}


SimulatorStatus Simulator::RunToAddress(cf::uint32 address)
{
	if (impl_->CannotRun())
		return impl_->status_;

	impl_->breakpoints_.Set(address, cf::BPT_TEMP_EXEC);

	return Run();
}


SimulatorStatus Simulator::StepOver()
{
	return Call(std::bind(&Impl::StepOver, impl_));
}


SimulatorStatus Simulator::Impl::StepOver()
{
	if (CannotRun())
		return status_;

	auto pc= ctx_->Cpu().pc;
	auto opcode= ctx_->GetWord(pc);	// current instruction
	bool run= false;

	if (auto i= ctx_->GetInstruction(opcode))
	{
		// step over instructions that invoke subroutines, otherwise just a single step
		if (i->ControlFlow() == IControlFlow::SUBROUTINE)
		{
			auto d= DecodeInstructionHelper(*ctx_, pc);
			// set temp breakpoint following current instruction;
			// this is simplistic if jsr/bsr don't return there... TODO: improve
			temp_bp_addr_to_clear_ = breakpoints_.Set(pc + d.Length(), cf::BPT_TEMP_EXEC);

			run = true;
		}
	}

	if (run)
		return Run(Impl::Condition::Run); // step over current instruction; run till the temp breakpoint
	else
		return Step();	// just a step is sufficient
}

// main execution simulation loop used by run, step over, run till ret
SimulatorStatus Simulator::Impl::RunSimulation(Condition cond)
{
	auto old_stacks= ctx_->Cpu().GetStackPointers();
	auto exec_pending= false;

	try
	{
		while (!stop_execution_)
		{
			if (ctx_->IsExecutionHalted())
				return SIM_FINISHED;

			if (breakpoints_.Hit(ctx_->Cpu().pc))
			{
				if (breakpoints_.ClearTemp(ctx_->Cpu().pc))
					return SIM_STOPPED;
				else if (exec_pending)	// ignore bp if we only just started run
					return SIM_BREAKPOINT_HIT;
			}

			exec_pending = true;

			auto instruction= ctx_->ExecuteInstruction(false);

			if (cond == Condition::TillRet && instruction != nullptr && instruction->ControlFlow() == IControlFlow::RETURN)
			{
				// run till return; if either user or super stack pointer is higher than it was before RTS/RTE, break
				// this is not bullet proof, and some corner cases will trigger it too, like manually adjusting stack
				auto new_stacks= ctx_->Cpu().GetStackPointers();
				if (new_stacks.first > old_stacks.first || new_stacks.second > old_stacks.second)
					break;
			}

			// update peripherals
			for (auto& p : peripherals_)
				p.DoUpdate(*ctx_);

			if (cond == Condition::SingleStep)
				break;
		}
	}
	catch (FaultOnFault&)
	{
		// todo:
		// exception during exception, stop simulator with proper info...
	}
	catch (ExceptionReported&)
	{
		return ctx_->IsExecutionHalted() ? SIM_FINISHED : SIM_EXCEPTION;
	}

	if (temp_bp_addr_to_clear_)
	{
		// clear the temp bp in case we stopped due to some other reason
		breakpoints_.ClearTemp(temp_bp_addr_to_clear_);
		temp_bp_addr_to_clear_ = 0;
	}

	if (ctx_->IsExecutionHalted())
		return SIM_FINISHED;

	return SIM_STOPPED;
}


cf::uint32 Simulator::GetRegister(cf::Register reg) const
{
	switch (reg)
	{
	case cf::R_D0:	return impl_->ctx_->Cpu().d_reg[0];
	case cf::R_D1:	return impl_->ctx_->Cpu().d_reg[1];
	case cf::R_D2:	return impl_->ctx_->Cpu().d_reg[2];
	case cf::R_D3:	return impl_->ctx_->Cpu().d_reg[3];
	case cf::R_D4:	return impl_->ctx_->Cpu().d_reg[4];
	case cf::R_D5:	return impl_->ctx_->Cpu().d_reg[5];
	case cf::R_D6:	return impl_->ctx_->Cpu().d_reg[6];
	case cf::R_D7:	return impl_->ctx_->Cpu().d_reg[7];

	case cf::R_A0:	return impl_->ctx_->Cpu().a_reg[0];
	case cf::R_A1:	return impl_->ctx_->Cpu().a_reg[1];
	case cf::R_A2:	return impl_->ctx_->Cpu().a_reg[2];
	case cf::R_A3:	return impl_->ctx_->Cpu().a_reg[3];
	case cf::R_A4:	return impl_->ctx_->Cpu().a_reg[4];
	case cf::R_A5:	return impl_->ctx_->Cpu().a_reg[5];
	case cf::R_A6:	return impl_->ctx_->Cpu().a_reg[6];
	case cf::R_A7:	return impl_->ctx_->Cpu().a_reg[7];

	case cf::R_SP:	return impl_->ctx_->Cpu().a_reg[7];

	case cf::R_PC:	return impl_->ctx_->Cpu().pc;
	case cf::R_SR:	return impl_->ctx_->Cpu().GetSR();

	case cf::R_USP:
		return impl_->ctx_->Cpu().Supervisor() ? impl_->ctx_->Cpu().a_reg[7] : impl_->ctx_->Cpu().GetUSP();

	case cf::R_MBAR:	return impl_->ctx_->Cpu().mbar;
	case cf::R_VBR:		return impl_->ctx_->Cpu().vbr;
	}
	ASSERT(false);
	return 0;
}


void Simulator::SetRegister(cf::Register reg, cf::uint32 value)
{
	switch (reg)
	{
	case cf::R_D0:	impl_->ctx_->Cpu().d_reg[0] = value; break;
	case cf::R_D1:	impl_->ctx_->Cpu().d_reg[1] = value; break;
	case cf::R_D2:	impl_->ctx_->Cpu().d_reg[2] = value; break;
	case cf::R_D3:	impl_->ctx_->Cpu().d_reg[3] = value; break;
	case cf::R_D4:	impl_->ctx_->Cpu().d_reg[4] = value; break;
	case cf::R_D5:	impl_->ctx_->Cpu().d_reg[5] = value; break;
	case cf::R_D6:	impl_->ctx_->Cpu().d_reg[6] = value; break;
	case cf::R_D7:	impl_->ctx_->Cpu().d_reg[7] = value; break;

	case cf::R_A0:	impl_->ctx_->Cpu().a_reg[0] = value; break;
	case cf::R_A1:	impl_->ctx_->Cpu().a_reg[1] = value; break;
	case cf::R_A2:	impl_->ctx_->Cpu().a_reg[2] = value; break;
	case cf::R_A3:	impl_->ctx_->Cpu().a_reg[3] = value; break;
	case cf::R_A4:	impl_->ctx_->Cpu().a_reg[4] = value; break;
	case cf::R_A5:	impl_->ctx_->Cpu().a_reg[5] = value; break;
	case cf::R_A6:	impl_->ctx_->Cpu().a_reg[6] = value; break;
	case cf::R_A7:	impl_->ctx_->Cpu().a_reg[7] = value; break;

	case cf::R_PC:	impl_->ctx_->Cpu().pc = value; break;
	case cf::R_SR:	impl_->ctx_->Cpu().SetSR(cf::uint16(value)); break;

	case cf::R_MBAR:impl_->ctx_->Cpu().mbar = value & CPU::MBAR_ADDR_MASK; break;
	case cf::R_VBR:	impl_->ctx_->Cpu().vbr = value; break;

	default:
		ASSERT(false);
		break;
	}

	impl_->SendUpdate(cf::E_REGISTER);
}


void Simulator::SetRegister(cf::Register reg, cf::uint32 add, cf::uint32 remove)
{
	cf::uint32 r= GetRegister(reg);
	r = (r & ~remove) | add;
	SetRegister(reg, r);
}


void Simulator::SetEventCallback(const std::function<void (cf::Event ev, const EventArgs& params)>& callback)
{
	impl_->callback_ = callback;
}


bool Simulator::GetFlag(cf::Flag flag) const
{
	switch (flag)
	{
	case cf::F_CARRY:		return !!impl_->ctx_->Carry();
	case cf::F_ZERO:		return !!impl_->ctx_->Zero();
	case cf::F_NEGATIVE:	return !!impl_->ctx_->Negative();
	case cf::F_OVERFLOW:	return !!impl_->ctx_->Overflow();
	case cf::F_SUPERVISOR:	return !!impl_->ctx_->Supervisor();

	}

	ASSERT(false);
	return false;
}


DecodedInstruction Simulator::DecodeInstruction(uint32 addr)
{
	return DecodeInstructionHelper(*impl_->ctx_, addr);
}


SimulatorStatus Simulator::GetStatus() const
{
	return impl_->status_;
}


std::string Simulator::GetStatusMsg(SimulatorStatus status) const
{
	switch (impl_->status_)
	{
	case SIM_OK:				return "Ready";
	case SIM_STOPPED:			return "Ready";
	case SIM_BREAKPOINT_HIT:	return "Breakpoint hit";	// simulator stopped at breakpoint
	case SIM_EXCEPTION:			return "Exception";			// entering CF exception (details are reported thru exc. callback)
	case SIM_IS_RUNNING:		return "Running";			// program execution pending
	case SIM_FINISHED:			return "Program finished";
	case SIM_INTERNAL_ERROR:	return "Internal simulator error";

	default:
		assert(false);
		return "?";
	}
}


std::string Simulator::GetExceptionMsg(uint32 address, CpuExceptions vector, uint32 pc) const
{
	switch (vector)
	{
	case EX_AccessError:
		{
			std::ostringstream ost;
			ost << "Access error at " << std::hex << std::setfill('0') << std::setw(8) << address;
			return ost.str();
		}

	case EX_AddressError:				return "Address error";
	case EX_IllegalInstruction:			return "Illegal instruction";
	case EX_DivideByZero:				return "Divide by zero";
	case EX_PrivilegeViolation:			return "Privilege violation";
	case EX_UnimplementedLineAOpcode:	return "Unimplemented line A opcode";
	case EX_UnimplementedLineFOpcode:	return "Unimplemented line F opcode";
	default:
		break;
	}

	std::ostringstream ost;
	ost << "Exception number " << int(vector);
	return ost.str();
}


void Simulator::ClearMemory()
{
	// fill memory banks with zeros
	const auto count= impl_->ctx_->GetMemoryBankCount();
	for (size_t i= 0; i < count; ++i)
		impl_->ctx_->ClearMemory(static_cast<int>(i));
}


void Simulator::SetSimulatorCallback(const PeripheralCallback& io)
{
	impl_->ctx_->SetSimulatorCallback(io);
}


cf::uint32 Simulator::GetSimulatorIOArea() const
{
	return impl_->ctx_->GetSimulatorIOArea();
}


void Simulator::SetExceptionCallback(const ExceptionCallback& ex)
{
	impl_->ctx_->SetExceptionCallback(ex);
}


void Simulator::SetTempBreakpoint(uint32 address)
{
	impl_->breakpoints_.Set(address, cf::BPT_TEMP_EXEC);
}


void Simulator::SetBreakpoint(uint32 address, bool set)
{
	if (set)
		impl_->breakpoints_.Set(address, cf::BPT_EXECUTE);
	else
		impl_->breakpoints_.Remove(address, cf::BPT_EXECUTE);
}


void Simulator::EnableBreakpoint(uint32 address, bool enable)
{
	auto bp= impl_->breakpoints_.Get(address);
	if (bp == cf::BPT_NONE)
		return;
	if (enable)
		impl_->breakpoints_.Set(address, static_cast<cf::BreakpointType>(bp & ~cf::BPT_DISABLED));
	else
		impl_->breakpoints_.Set(address, static_cast<cf::BreakpointType>(bp | cf::BPT_DISABLED));
}


bool Simulator::GetBreakpoint(uint32 address) const
{
	auto bp= impl_->breakpoints_.Get(address);
	return bp == cf::BPT_EXECUTE;
}


void Simulator::ClearAllBreakpoints()
{
	impl_->breakpoints_.ClearAll();
}


std::vector<uint32> Simulator::GetAllBreakpoints() const
{
	std::vector<uint32> v;
	v.reserve(impl_->breakpoints_.Count());
	for (const auto& bp : impl_->breakpoints_)
		if ((bp.second & cf::BPT_MASK) != cf::BPT_NONE)
			v.push_back(bp.first);
	return v;
}


PeripheralDevice* Simulator::FindPeripheral(const char* category, const char* version) const
{
	// finding is slow...

	for (auto& p : impl_->peripherals_)
		if (p.Category() == category && (version == nullptr || p.Version() == version))
			return &p;

	return nullptr;
}


// add peripheral device from given 'category' and requested 'version';
// returns 'PeripheralDevice*' if it was found and created or nullptr if no such device is implemented
// io_area_offset - offset from MBAR where device's registers are mapped
// interrupt_source - number of this device reported to the interrupt control module
// trace - print all read/write attempts through the SimWrite
// notify - fire simulator event whenever device is read/written to
//
PeripheralDevice* Simulator::Impl::AddPeripheral(const char* category, const char* version, uint16 io_area_offset, uint16 io_area_size, uint16 interrupt_source, bool trace, bool notify, PeripheralConfigData& config)
{
	PParam params;
	params.InterruptSource(interrupt_source);
	params.IOAreaOffset(io_area_offset);
	params.IOAreaSize(io_area_size);
	params.Category(category);
	params.Version(version);
	params.Trace(trace);
	params.NotifyClient(notify);

	auto device= GetPeripherals().Create(version, params, config);
	if (device == nullptr)
		return nullptr;

	auto peripheral= device.get();

	peripherals_.push_back(device.release());

	periperals_io_area_.fill(~0);

	std::vector<InterruptController*> icms;

	// scan peripherals for interrupt controllers and remember them separately;
	// build map of IO area

	uint8 index= 0;
	for (auto& p : peripherals_)
	{
		// if this is interrupt controller, remember it
		if (auto icm= dynamic_cast<InterruptController*>(&p))
			icms.push_back(icm);

		// IO map
		auto range= p.GetIOArea();
		if (range.base >= periperals_io_area_.size() || range.end >= periperals_io_area_.size())
			throw RunTimeError("Peripheral IO area is limited to 0x10000; at " __FUNCTION__);

		for (auto i= range.base; i != range.end; ++i)
			periperals_io_area_[i] = index;

		++index;
		if (index == 0xff)
			throw RunTimeError("Max number of peripherals reached at " __FUNCTION__);
	}

	ctx_->SetICM(icms);

	return peripheral;
}


// find peripheral mapped into 'addr' and carry on read/write
bool Simulator::Impl::PeripheralsIO(uint32 addr, int access_size, uint32& ret_val, bool read)
{
	auto mbar= ctx_->Cpu().mbar;
	if (addr < mbar)
		return false;

	auto offset= addr - mbar;
	if (offset < periperals_io_area_.size())
	{
		auto dev_index= periperals_io_area_[offset];
		if (dev_index < peripherals_.size())
		{
			auto& device= peripherals_[dev_index];
			offset -= device.GetIOArea().base;

			TRACE("Peripheral IO: addr %x, dev %d, port %x, read: %d", addr, int(dev_index), offset, int(read));

			if (read)
				ret_val = device.DoRead(*ctx_, offset, access_size);
			else
				device.DoWrite(*ctx_, offset, access_size, ret_val);

			if (read)
				TRACE(" val read: $%x\n", int(ret_val));
			else
				TRACE(" val written: $%x\n", int(ret_val));

			if (device.NotifyClient())
				SendUpdate(cf::E_DEVICE_IO, addr, &device, read ? cf::DeviceAccess::Read : cf::DeviceAccess::Write);

			return true;
		}
	}

	return false;
}


// how to handle exception 'ex'
void Simulator::ExceptionHandling(CpuExceptions ex, bool stop)
{
	return impl_->ctx_->ExceptionHandling(ex, stop);
}


uint32 Simulator::CyclesTaken() const
{
	return impl_->ctx_->CyclesTaken();
}


uint32 Simulator::ExecutedInstructions() const
{
	return impl_->ctx_->ExecutedInstructions();
}


void Simulator::ZeroStats()	// clear cycle and instruction counter
{
	impl_->ctx_->ZeroStats();
}


void Simulator::SetConfigDefaults(cf::Register reg, uint32 value)
{
	impl_->ctx_->Cpu().SetDefaults(reg, value);
}


void Simulator::LoadConfiguration(const wchar_t* cfg_file)
{
	Path path(cfg_file);
	std::ifstream cfg(cfg_file);
	if (!cfg.good())
		throw std::exception(("Cannot open " + path.string()).c_str());

	boost::property_tree::ptree config;
	boost::property_tree::read_info(cfg, config);

	ISA default_isa= ISA::A;
	if (auto isa= config.get_optional<std::string>("ISA"))
		default_isa = StringToISA(*isa);

	SetIsa(default_isa);

	typedef HexNumber<unsigned int> Hex;

	if (auto mbar= config.get_optional<Hex>("MBAR"))
		SetConfigDefaults(cf::R_MBAR, *mbar);

	if (auto vbr= config.get_optional<Hex>("VBR"))
		SetConfigDefaults(cf::R_VBR, *vbr);

	// create memory banks and devices specified in a config

	int bank= 0;
	auto memory= config.get_child("Memory");

	for (auto& p : memory)
	{
		auto& mem= p.second;

		cf::MemoryAccess access= cf::MemoryAccess::Normal;
		if (p.first == "RAM" || p.first == "SRAM")
		{}
		else if (p.first == "ROM" || p.first == "Flash")
			access = cf::MemoryAccess::ReadOnly;
		else if (p.first == "Dummy" || p.first == "Null")
			access = cf::MemoryAccess::Null;
		else
			throw std::exception((boost::format("Memory type '%s' not recognized") % p.first).str().c_str());

		auto base= mem.get<Hex>("base");
		auto size= mem.get<Hex>("size");

		CreateMemoryBank(p.first, base, size, bank++, access);
	}

	if (auto peripherals= config.get_child_optional("Peripherals"))
	{
		for (auto& p : *peripherals)
		{
			auto& device= p.second;

			auto category= p.first;
			auto version= device.get<std::string>("version", "");
			auto trace= device.get<int>("trace", 0) != 0;
			auto notify= device.get<int>("notify", 0) != 0;
			auto io_offset= device.get<Hex>("io_offset");
			if (io_offset > 0xffff)
				throw std::exception((boost::format("Peripheral %s/%s IO offset too big: %d") % category % version % io_offset).str().c_str());
			auto io_area_size= device.get<Hex>("io_area_size", 0);
			if (io_area_size > 0xffff)
				throw std::exception((boost::format("Peripheral %s/%s IO area size too big: %d") % category % version % io_area_size).str().c_str());
			auto interrupt_source= device.get("interrupt_source", 0);

			if (!impl_->AddPeripheral(category.c_str(), version.c_str(), io_offset, io_area_size, interrupt_source, trace, notify, device))
			{
				// complain about missing device; most likely this is config problem
				throw std::exception((boost::format("Cannot find device '%s/%s'") % category % version).str().c_str());
			}
		}
	}

	// push defaults to CPU regs (VBR, MBAR), reset peripherals
	Reset();
}

/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "resource.h"
#include "Debugger.h"
#include "Broadcast.h"
#include "IOWindow.h"
#include "Utilities.h"
#include "ProtectedCall.h"
#include "FormatNums.h"
#include <mutex>
#include "Settings.h"
#include "TypeTranslators.h"
//#include "../ColdFire/LoadConfiguration.h"

using namespace Defs;


Debugger::Debugger() : SettingsClient(L"Debugger")
{
	terminal_ = nullptr;
	main_wnd_ = nullptr;
	get_terminal_msg_ = 0;
	terminal_wnd_ = nullptr;
	cur_prog_counter_ = 0;
	current_line_ = 0;
	default_isa_ = ISA::C;
	io_port_addr_ = ~0;

	// read configuration file

	ProtectedCall([&]()
	{
		auto config_name= AppSettings().get_string("simulator.config_file");
		Path path= GetConfigurationPath() / config_name;

		simulator_.LoadConfiguration(path.c_str());

		init();
	}, L"Simulator initialization failed."); //TODO: how to bail gracefully? Program shouldn't continue
}

void Debugger::LoadMonitorCode(const wchar_t* path)
{
	monitor_ = cf::LoadBinaryProgram(path);
	LoadMonitor();
	simulator_.AbortExecution();
}

//-----------------------------------------------------------------------------

boost::optional<cf::uint32> Debugger::GetCodeAddress(int line, const CString& file)
{
	boost::optional<cf::uint32> ret;

	if (debug_info_.get())
		ret = debug_info_->GetAddress(line, std::wstring(file));

	return ret;
}


Defs::Breakpoint Debugger::GetBreakpoint(cf::uint32 addr)
{
	return simulator_.GetBreakpoint(addr) ? BPT_EXECUTE : BPT_NONE;
}


void Debugger::ClearAllBreakpoints()
{
	return simulator_.ClearAllBreakpoints();
}


void Debugger::SetBreakpoint(cf::uint32 addr)
{
	simulator_.SetBreakpoint(addr, true);
}


Defs::Breakpoint Debugger::SetBreakpoint(int line, const CString& path)
{
	if (auto addr= GetCodeAddress(line, path))
	{
		SetBreakpoint(*addr);
		return BPT_EXECUTE;
	}
	return BPT_NONE;
}


Defs::Breakpoint Debugger::ToggleBreakpoint(cf::uint32 addr)
{
	bool bp= !simulator_.GetBreakpoint(addr);
	simulator_.SetBreakpoint(addr, bp);
	return bp ? BPT_EXECUTE : BPT_NONE;
}


Defs::Breakpoint Debugger::ToggleBreakpoint(int line, const CString& path)
{
	if (auto addr= GetCodeAddress(line, path))
		return ToggleBreakpoint(*addr);
	return BPT_NONE;
}


void Debugger::SimEvent(cf::Event event, const Simulator::EventArgs& params)
{
	if (main_wnd_ == nullptr)
		return;

	if (event == cf::E_DEVICE_IO)
	{
		main_wnd_->DeviceIO(params.device, params.access, params.param);
	}
	else if (event != cf::E_RUNNING)
	{
		int line= -1;
		auto stat= simulator_.GetStatus();
		if (stat != SIM_IS_RUNNING && stat != SIM_FINISHED)
		{
			if (debug_info_.get())
			{
				std::wstring path;
				line = current_line_ = debug_info_->GetLine(params.param, path);
				{
					std::lock_guard<std::mutex> l(lock_);
					cur_line_path_ = path;
				}
			}
			cur_prog_counter_ = simulator_.GetRegister(cf::R_PC);
		}
		else
			cur_prog_counter_ = ~0;

		main_wnd_->PostMsg(Broadcast::WM_APP_STATE_CHANGED, event, line);
	}
	else
		main_wnd_->PostMsg(Broadcast::WM_APP_STATE_CHANGED, event, -1);
}


int Debugger::GetCurrentLine() const
{
	return current_line_;
}

std::wstring Debugger::GetCurrentLinePath() const
{
	std::lock_guard<std::mutex> l(lock_);
	return cur_line_path_;
}

cf::uint32 Debugger::GetCurrentProgCounter() const
{
	return cur_prog_counter_;
}

//-----------------------------------------------------------------------------

SimulatorStatus Debugger::StepInto()
{
	return simulator_.Step();
}


SimulatorStatus Debugger::StepIntoException()
{
	return simulator_.OneStep();
}


SimulatorStatus Debugger::StepOver()
{
	return simulator_.StepOver();
}


SimulatorStatus Debugger::StepOut()
{
	return simulator_.StepOut();
}


SimulatorStatus Debugger::Run()
{
	return simulator_.Run();
}


void Debugger::RunToAddress(int line, const CString& path)
{
	auto addr= GetCodeAddress(line, path);

	if (addr)
		simulator_.RunToAddress(*addr);
}


void Debugger::RunToAddress(uint32 addr)
{
	simulator_.RunToAddress(addr);
}


void Debugger::SkipToAddress(int line, const CString& path)
{
	auto addr= GetCodeAddress(line, path);

	if (addr)
		SetRegister(cf::R_PC, *addr);
}

void Debugger::SkipToAddress(uint32 addr)
{
	SetRegister(cf::R_PC, addr);
}

//-----------------------------------------------------------------------------

CString Debugger::GetStatusMessage(SimulatorStatus status) const
{
	if (status == SIM_EXCEPTION)
	{
		// provide more details
		return simulator_.GetExceptionMsg(ex_info_.addr, ex_info_.vector, ex_info_.pc).c_str();
	}

	return simulator_.GetStatusMsg(status).c_str();
}

SimulatorStatus Debugger::GetStatus() const
{
	return simulator_.GetStatus();
}


//-----------------------------------------------------------------------------


void Debugger::Restart()
{
	LoadMonitor();

	//todo: check code overlap with monitor

	simulator_.SetProgram(code_);
	if (code_.Valid() && monitor_.Valid())
	{
		// set temp breakpoint at the program start, and execute CPU reset;
		// it will run monitor code to initialize SP, and go to the start of the simulated progam
		simulator_.SetRegister(cf::R_PC, monitor_.GetProgramStart());
		simulator_.SetTempBreakpoint(code_.GetProgramStart());
		simulator_.SetIsa(code_.GetIsa());
		simulator_.ZeroStats();
		simulator_.Run();
	}
}

//=============================================================================

CWnd* Debugger::io_window()     // find terminal window
{
	if (main_wnd_ == nullptr)
		return nullptr;

	return reinterpret_cast<CWnd*>(main_wnd_->SendMsg(get_terminal_msg_, 0, 0));
}

#if 1

void Debugger::io_function(int command, uint32& value, bool read)
{
	// this function is being called from a simulator thread and it needs to be thread-safe
	// terminal access through SendMessage is safe if slow

	if (terminal_wnd_ == nullptr)
	{
		terminal_wnd_ = io_window();
		if (terminal_wnd_ == nullptr)
			return;
	}

	if (!::IsWindow(terminal_wnd_->m_hWnd))
		return;

	switch (command)
	{
	case TERMINAL_IN_OUT:
		value = static_cast<cf::uint32>(terminal_wnd_->SendMessage(read ? IOWindow::CMD_IN : IOWindow::CMD_PUTC, value, 0));
		break;

	case TERMINAL_CLS:
		if (!read)
			terminal_wnd_->SendMessage(IOWindow::CMD_CLS);
		break;

	case TERMINAL_X_POS:
	case TERMINAL_Y_POS:
		if (read)
			value = static_cast<cf::uint32>(terminal_wnd_->SendMessage(IOWindow::CMD_POSITION, 0x2 | (command == TERMINAL_X_POS ? 1 : 0)));
		else
			terminal_wnd_->SendMessage(IOWindow::CMD_POSITION, command == TERMINAL_X_POS ? 1 : 0, value);
		break;

	case TERMINAL_WIDTH:
	case TERMINAL_HEIGHT:
		if (read)
			value = static_cast<cf::uint32>(terminal_wnd_->SendMessage(IOWindow::CMD_SIZE, 0x2 | (command == TERMINAL_WIDTH ? 1 : 0)));
		else
			terminal_wnd_->SendMessage(IOWindow::CMD_SIZE, command == TERMINAL_WIDTH ? 1 : 0, value);
		break;

	// ---------------------------------------------------------
	case RANDOM_NUMBER:
		if (read)
			value = rand();
		break;

	default:
		ASSERT(false);	// wrong function
		break;
	}
}

#else

void Debugger::io_function(int command, uint32& value, bool read)
{
	// this function is being called from a simulator thread and it needs to be thread-safe

	// terminal IO through direct pointer is not thread safe
	if (terminal_wnd_ == nullptr)
	{
		terminal_wnd_ = io_window();
		if (terminal_wnd_ == nullptr)
			return;
		terminal_ = reinterpret_cast<IOChannel*>(terminal_wnd_->SendMessage(IOWindow::CMD_GET_PTR));
	}

	switch (command)
	{
	case TERMINAL_IN_OUT:
		if (read)
			value = terminal_->GetChar();
		else
			terminal_->PutChar(value);
//		value = terminal_wnd_->SendMessage(read ? IOWindow::CMD_IN : IOWindow::CMD_PUTC, value, 0);
		break;

	case TERMINAL_CLS:
		if (!read)
			terminal_->Clear(); //SendMessage(IOWindow::CMD_CLS);
		break;

	case TERMINAL_X_POS:
		if (read)
			value = terminal_->GetCursorXPos();
		else
			terminal_->SetCursorXPos(value);
		break;

	case TERMINAL_Y_POS:
		if (read)
			value = terminal_->GetCursorYPos();
		else
			terminal_->SetCursorYPos(value);
		break;

	case TERMINAL_WIDTH:
		if (read)
			value = terminal_->GetTerminalWidth();
		else
			terminal_->SetTerminalWidth(value);
		break;

	case TERMINAL_HEIGHT:
		if (read)
			value = terminal_->GetTerminalHeight();
		else
			terminal_->SetTerminalHeight(value);
		break;

	case RANDOM_NUMBER:
		if (read)
			value = rand();
		break;

	default:
		ASSERT(false);            // wrong function
	}
}
#endif

///////////////////////////////////////////////////////////////////////////////

void Debugger::init()
{
	default_isa_ = simulator_.GetIsa();

	simulator_.SetEventCallback(std::bind(&Debugger::SimEvent, this, std::placeholders::_1, std::placeholders::_2));

	// hardcoded; this port is also used by monitor code
	io_port_addr_ = simulator_.GetSimulatorIOArea();

	simulator_.SetSimulatorCallback(std::bind(&Debugger::PeripheralIO, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	simulator_.SetExceptionCallback(std::bind(&Debugger::Exception, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	simulator_.InitSP();

	CallApplySettings();
}


bool Debugger::Exception(uint32 addr, CpuExceptions vector, cf::uint32 pc)
{
	// Note: this function is called from execution thread, it should be thread safe

	// stop execution
	simulator_.BreakExecution();

	ex_info_.pc = pc;
	ex_info_.addr = addr;
	ex_info_.vector = vector;

	// rewind PC to the offending instruction
	simulator_.SetRegister(cf::R_PC, pc);

	// report this exception as handled, so CF CPU will not enter exception routine
	return true;
}


bool Debugger::PeripheralIO(uint32 addr, int access_size, uint32& value, bool read)
{
	// Simulated peripherals
	//
	// Note: this function is called from execution thread, it should be thread safe

	if (addr == io_port_addr_ + RAM_BASE * 4 && access_size == 4 && read)
	{
		value = simulator_.GetMemoryBankInfo(0).Base();
		return true;
	}
	else if (addr == io_port_addr_ + RAM_END * 4 && access_size == 4 && read)
	{
		value = simulator_.GetMemoryBankInfo(0).End();
		return true;
	}
	else if (addr == io_port_addr_ + GET_TIMER * 4 && access_size == 4 && read)
	{
		value = ::GetTickCount();
		return true;
	}
	else if (addr == io_port_addr_ + GET_DATE_TIME * 4 && access_size == 4 && read)
	{
		// get date/time
		time_t t;
		time(&t);
		value = static_cast<uint32>(t / 2);
		return true;
	}
	else if (addr == io_port_addr_ + PROG_START * 4 && access_size == 4 && read)
	{
		value = code_.Valid() ? code_.GetProgramStart() : 0;
		return true;
	}
	else
	{
		auto port= io_port_addr_ + OTHER_FUNCTIONS * 4;

		if (addr < port || addr > port + 2 * IO_LAST_FUNC || (addr & 1) != 0 || access_size == 1)
			return false;

		int command= (addr - port) / 2;

		io_function(command, value, read);

		return true;
	}
	return false;
}


void Debugger::Break()
{
	simulator_.BreakExecution();
}


void Debugger::AbortProg()
{
	simulator_.AbortExecution();
}


bool Debugger::IsRunning() const
{
	return simulator_.GetStatus() == SIM_IS_RUNNING;
}


bool Debugger::IsStopped() const
{
	switch (simulator_.GetStatus())
	{
	case SIM_STOPPED:
	case SIM_BREAKPOINT_HIT:
	case SIM_EXCEPTION:
		return true;

	default:
		return false;
	}
}


bool Debugger::IsStoppedAtException() const
{
	switch (simulator_.GetStatus())
	{
	case SIM_EXCEPTION:
		return true;

	default:
		return false;
	}
}


bool Debugger::IsFinished() const
{
	return simulator_.GetStatus() == SIM_FINISHED;
}


bool Debugger::IsActive() const
{
	return true;
}


void Debugger::LoadMonitor()
{
	simulator_.Reset();
	simulator_.ClearMemory();
	simulator_.SetProgram(monitor_);
	try
	{
		// todo: is this really necessary?
		simulator_.SetInitialStackAndPC(monitor_.GetProgramStart());
	}
	catch (MemoryAccessException&)	// ignore it if there's no memory at VBR
	{}
}


void Debugger::SetProgram(const cf::BinaryProgram& code, bool run_monitor)
{
	code_ = code;
	if (run_monitor)
	{
		// program may have different ISA, set it before monitor code is initialized
		simulator_.SetIsa(code_.GetIsa());
		Restart();
	}
	else
	{
		try
		{
			simulator_.ClearMemory();
			simulator_.SetProgram(code_);
			simulator_.SetIsa(code_.GetIsa());
			simulator_.Reset();
			simulator_.SetRegister(cf::R_PC, code.GetProgramStart());
			cur_prog_counter_ = simulator_.GetRegister(cf::R_PC);
		}
		catch (MemoryAccessException&)
		{
			throw std::exception((boost::format("Binary code cannot be loaded into requested area ($%X-$%X). There is no memory there.") % code.FirstByteAddress() % (code.LastByteAddress() - 1)).str().c_str());
		}
	}
}


cf::uint32 Debugger::GetRegister(cf::Register reg) const
{
	return simulator_.GetRegister(reg);
}


void Debugger::SetRegister(cf::Register reg, cf::uint32 value)
{
	if (IsRunning())
		return;

	simulator_.SetRegister(reg, value);
}


void Debugger::ModifyRegister(cf::Register reg, cf::uint32 add, cf::uint32 remove)
{
	if (IsRunning())
		return;

	simulator_.SetRegister(reg, add, remove);
}


void Debugger::SetDebugInfo(std::unique_ptr<DebugData> debug)
{
	debug_info_ = std::move(debug);

	{
		std::lock_guard<std::mutex> l(lock_);
		cur_line_path_.clear();
	}
}


DecodedInstruction Debugger::DecodeInstruction(uint32 addr)
{
	return simulator_.DecodeInstruction(addr);
}


bool Debugger::GetFlag(cf::Flag flag) const
{
	return simulator_.GetFlag(flag);
}


void Debugger::SetMainWnd(MainWindow* wnd, int get_terminal_msg)
{
	main_wnd_ = wnd;
	get_terminal_msg_ = get_terminal_msg;
}


cf::MemoryBankInfo Debugger::GetMemoryBankInfo(int bank) const
{
	return simulator_.GetMemoryBankInfo(bank);
}


void Debugger::SetMemory(const std::vector<cf::uint8>& m, cf::uint32 address)
{
	if (!m.empty())
		simulator_.SetMemory(address, m.data(), m.data() + m.size());
}


void Debugger::SetMemory(const cf::uint8* m, cf::uint32 size, cf::uint32 address)
{
	if (m != nullptr && size > 0)
		simulator_.SetMemory(address, m, m + size);
	else if (size > 0)
		simulator_.ZeroMemory(address, size);
}


cf::uint32 Debugger::ReadMemory(cf::uint8* dest_buf, cf::uint32 address, cf::uint32 length)
{
	return simulator_.ReadMemory(dest_buf, address, length);
}


void Debugger::ApplySettings(SettingsSection& settings)
{
	auto exceptions= settings.section("simulator.exceptions");

	for (auto& ex : exceptions)
	{
		auto vector= static_cast<CpuExceptions>(atoi(ex.first.c_str()));
		bool stop= ex.second.get_value<bool>();
		simulator_.ExceptionHandling(vector, stop);
	}
}


ISA Debugger::GetDefaultIsa() const
{
	return default_isa_;
}


ISA Debugger::GetCurrentIsa() const
{
	return simulator_.GetIsa();
}


CpuExceptions Debugger::GetLastExceptionVector() const
{
	return ex_info_.vector;
}


cf::uint32 Debugger::CyclesTaken() const
{
	return simulator_.CyclesTaken();
}


cf::uint32 Debugger::ExecutedInstructions() const
{
	return simulator_.ExecutedInstructions();
}


PeripheralDevice* Debugger::FindDevice(const char* category, const char* version) const
{
	return simulator_.FindPeripheral(category, version);
}

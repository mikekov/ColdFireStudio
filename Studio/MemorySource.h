#pragma once
#include "CFTypes.h"
class Debugger;


class MemorySource
{
public:
	MemorySource();
	MemorySource(Debugger& dbg, cf::Register watch_register);
	MemorySource(Debugger& dbg, int memory_bank);
	~MemorySource();

	// base address for memory
	cf::uint32 Base() const;
	// size of memory in bytes
	cf::uint64 Size() const;
	// current address to display
	cf::uint32 CurAddress() const;
	void SetCurAddress(cf::uint32 addr);

	bool ReadOnly() const;

// temporary
const char* Buf() const;

	// read & write

	// void Read();

	// modify memory @ address
	void Write(const std::vector<BYTE>& m, cf::uint32 address);
	void Write(const cf::uint8* m, cf::uint32 size, cf::uint32 address);
	void Clear(cf::uint32 size, cf::uint32 address);

private:
	struct Impl;
	std::shared_ptr<Impl> impl_;
};

/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "IntelHex.h"
#include <boost/filesystem/fstream.hpp>
#include "..\ColdFire\BinaryProgram.h"

cf::BinaryProgram LoadHexFormat(std::istream& in);


cf::BinaryProgram LoadHexCode(const Path& path)
{
	boost::filesystem::ifstream f(path, std::ios::in);

	return LoadHexFormat(f);
}


namespace {

int SaveAddress(std::ostream& out, cf::uint32 addr)
{
	out << std::hex << std::setfill('0');

	int sum= 0;
	cf::uint8 temp[2]= { cf::uint8(addr >> 8), cf::uint8(addr) };
	for (int j= 0; j < 2; ++j)
	{
		sum += temp[j];
		out << std::setw(2) << int(temp[j]);
	}

	return sum;
}

}

void SaveHexCode(std::ostream& out, const cf::BinaryProgram& prog)
{
	out << std::uppercase << std::hex << std::setfill('0');

	cf::BinaryProgramRange it(prog);

	while (it)
	{
		auto part= it.Fragment();

		cf::uint64 start= it.Address();
		cf::uint64 end= start + part.size();
		cf::uint32 addr= static_cast<cf::uint32>((start ^ 0x10000) >> 16);	// force it to be different than start

		const int STEP= 0x20;
		size_t index= 0;
		for (auto i= start; i <= end; i += STEP)
		{
			auto count= static_cast<int>(i + STEP <= end ? STEP : end - i);	// number of bytes in a row (excl. address)

			cf::uint32 high= static_cast<cf::uint32>(i >> 16);

			if (high != addr)
			{
				addr = high;
				out << ":02000004";
				auto sum= 2 + 4 + SaveAddress(out, addr);
				out << std::setw(2) << (-sum & 0xff) << '\n';
			}

			// count
			out << ':' << std::setw(2) << count;
			auto sum= count + SaveAddress(out, static_cast<cf::uint32>(i));
			// data record type (zero, so checksum remains the same)
			out << "00";

			for (int j= 0; j < count; ++j)
			{
				auto byte= part[index++];
				sum += byte;
				out << std::setw(2) << int(byte);
			}

			// checksum
			out << std::setw(2) << (-sum & 0xff) << '\n';
		}

		++it;
	}
}


void SaveHexCode(const Path& path, const cf::BinaryProgram& prog)
{
	boost::filesystem::ofstream f(path, std::ios::out);
	SaveHexCode(f, prog);
}

//-----------------------------------------------------------------------------

namespace {

class hex_code_exception : public std::exception
{
public:
	hex_code_exception(const char* msg, int line) : exception((boost::format("%s, line %d") % msg % line).str().c_str())
	{}
};


unsigned int geth(const char*& ptr, UINT& sum, int row)	// read in two-digit hex number
{
	UINT res= 0;
	for (int i= 0; i < 2; ++i)
	{
		res <<= 4;
		auto c= *ptr++;
		if (c >= '0' && c <= '9')
			res += c - '0';
		else if (c >= 'A' && c <= 'F')
			res += c - 'A' + 10;
		else if (c >= 'a' && c <= 'f')
			res += c - 'a' + 10;
		else
			throw hex_code_exception("Hexadecimal number expected", row);
	}
	sum += res;		// to calculate checksum
	sum &= 0xFF;
	return res;
}

}

//-----------------------------------------------------------------------------

cf::BinaryProgram LoadHexFormat(std::istream& in)
{
	cf::uint32 prog_start = 0;
	in.exceptions(std::ios::badbit);

	if (!in.good())
		throw hex_code_exception("Cannot read file", 0);

	cf::BinaryProgram prog;
	bool done= false;

	for (int row= 1; !done; ++row)
	{
		if (!in.good() || in.eof())
			break;

		std::string line;
		std::getline(in, line);
		if (line.empty())
			continue;

		const char* ptr= line.c_str();
		if (*ptr++ != ':' || line.length() < 9)
			throw hex_code_exception("Wrong format", row);	// wrong format

		cf::uint32 address= 0;
		UINT sum= 0;					// checksum
		auto count= geth(ptr, sum, row);	// byte counter
		auto addr= geth(ptr, sum, row);
		addr <<= 8;
		addr += geth(ptr, sum, row);	// data address
		auto type= geth(ptr, sum, row);	// record type

		switch (type)
		{
		case 0:		// program code
			for (UINT i= 0; i != count; ++i)
				prog.PutByte(address + addr + i, static_cast<UINT8>(geth(ptr, sum, row)));

			geth(ptr, sum, row);		// checksum byte
			if (sum)			// checksum error?
				throw hex_code_exception("Checksum error", row);
			break;

		case 1:		// End of File record
			done = true;
			break;

		case 4:		// Extended Linear Address Record
			{
				auto addr= geth(ptr, sum, row);
				addr <<= 8;
				addr += geth(ptr, sum, row);	// data high address
				address = addr << 16;

				geth(ptr, sum, row);		// checksum byte

				if (sum)			// checksum error?
					throw hex_code_exception("Checksum error", row);
			}
			break;

		case 5:		// Start Linear Address Record
			if (count != 4)
				throw hex_code_exception("Invalid start address record", row);
			// endiannes is not known, assuming big endian
			address = 0;
			for (int i= 0; i < 4; ++i)
				address = (address << 8) | geth(ptr, sum, row);

			geth(ptr, sum, row);		// checksum byte

			if (sum)			// checksum error?
				throw hex_code_exception("Checksum error", row);

			break;

		default:	// unexpected value
			throw hex_code_exception("Unknown/unsupported record type", row);
		}
	}

	return prog;
}

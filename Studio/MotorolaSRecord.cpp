/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "MotorolaSRecord.h"
#include <boost/filesystem/fstream.hpp>
#include "..\ColdFire\BinaryProgram.h"

namespace {

int SaveAddress(std::ostream& out, cf::uint32 addr)
{
	out << std::hex << std::setfill('0');

	int sum= 0;
	cf::uint8 temp[4]= { cf::uint8(addr >> 24), cf::uint8(addr >> 16), cf::uint8(addr >> 8), cf::uint8(addr) };
	for (int j= 0; j < 4; ++j)
	{
		sum += temp[j];
		out << std::setw(2) << int(temp[j]);
	}

	return sum;
}

}


void SaveSRecordCode(std::ostream& out, const cf::BinaryProgram& prog)
{
	cf::BinaryProgramRange it(prog);

	out << std::uppercase << std::hex << std::setfill('0');

	while (it)
	{
		auto part= it.Fragment();

		cf::uint64 start= it.Address();
		cf::uint64 end= start + part.size();

		const cf::uint32 STEP= 0x20;
		size_t index= 0;
		for (cf::uint64 i= start; i < end; i += STEP)
		{
			auto count= i + STEP <= end ? STEP : end - i;	// number of bytes in a row (excl. address)
			// S3 record (with 4-byte address)
			out << "S3" << std::setw(2) << count + 1 + 4;	// data count, count itself, and 4 bytes address
			auto sum= static_cast<int>(count + 1 + 4);

			auto addr= static_cast<cf::uint32>(i);
			sum += SaveAddress(out, addr);

			for (int j= 0; j < count; ++j)
			{
				auto byte= part[index++];
				sum += byte;
				out << std::setw(2) << int(byte);
			}

			// checksum
			out << std::setw(2) << (~sum & 0xff) << "\n";
		}

		++it;
	}

	// starting address
	int count= 4 + 1;
	out << "S7" << std::setw(2) << count;
	int sum= count + SaveAddress(out, prog.GetProgramStart());
	out << std::setw(2) << (~sum & 0xff) << "\n";
}


void SaveSRecordCode(const Path& path, const cf::BinaryProgram& prog)
{
	boost::filesystem::ofstream f(path, std::ios::out);
	SaveSRecordCode(f, prog);
}


unsigned int geth(char c1, char c2, int* sum, int line_no)	// read in two-digit hex number
{
	char buf[]= { c1, c2 };
	const char* ptr= buf;

	unsigned int res= 0;

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
			throw std::exception((boost::format("Unreadable hexadecimal number at line %d") % line_no).str().c_str());
	}

	if (sum)
		*sum += res;		// to calculate checksum

	return res;
}


cf::BinaryProgram LoadSRecordCode(const Path& path)
{
	boost::filesystem::ifstream in(path, std::ios::in);
	in.exceptions(std::ios::badbit);

	cf::BinaryProgram prog;
	int line_no= 0;

	for (;;)
	{
		if (!in.good() || in.eof())
			break;

		++line_no;
		std::string line;
		std::getline(in, line);

		if (line.empty())
			continue;

		size_t offset= 0;
		if (line.length() >= 10 && line[0] == 'S')
		{
			++offset;
			char type= line[offset];
			auto addr_size= 2u;
			bool read_data= false;
			bool end_of_block= false;
			++offset;

			switch (type)
			{
			case '0':
				continue;	// skip header

			case '1':
				read_data = true;
				break;
			case '2':
				read_data = true;
				addr_size = 3;
				break;
			case '3':
				read_data = true;
				addr_size = 4;
				break;
			case '5':
				break;
			case '7':
				addr_size = 4;
				end_of_block = true;
				break;
			case '8':
				addr_size = 3;
				end_of_block = true;
				break;
			case '9':
				end_of_block = true;
				break;
			default:
				throw std::exception((boost::format("Unrecognized record format at line %d") % line_no).str().c_str());
			}

			int sum= 0;
			auto bytes= geth(line[offset], line[offset + 1], &sum, line_no);
			offset += 2;

			if (line.size() < 4 + bytes * 2)
				throw std::exception((boost::format("Truncated data at line %d") % line_no).str().c_str());

			if (bytes < 1 + addr_size)
				throw std::exception((boost::format("Invalid data at line %d") % line_no).str().c_str());

			unsigned int address= 0;
			for (auto i= 0u; i < addr_size; ++i)
			{
				auto a= geth(line[offset], line[offset + 1], &sum, line_no);
				address <<= 8;
				address |= a;
				offset += 2;
			}

			if (end_of_block)
				prog.SetProgramStart(address);

			if (read_data)
			{
				auto data_size= bytes - 1 - addr_size;
				for (auto i= 0u; i < data_size; ++i)
				{
					auto d= geth(line[offset], line[offset + 1], &sum, line_no);
					prog.PutByte(address + i, d);
					offset += 2;
				}
			}

			auto checksum= geth(line[offset], line[offset + 1], nullptr, line_no) + sum;
			if ((~checksum & 0xff) != 0)
				throw std::exception((boost::format("Checksum error at line %d") % line_no).str().c_str());
		}
		else
			throw std::exception((boost::format("Unrecognized data line %d") % line_no).str().c_str());
	}

	return prog;
}

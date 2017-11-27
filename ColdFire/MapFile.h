/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once
#ifndef _map_file_
#define _map_file_

#include "Asm.h"
#include <boost/bimap.hpp>


class MapFile
{
	boost::bimap<Path, masm::FileUID> files_;

public:
	MapFile()
	{}
	~MapFile()
	{}

	void Reset()
	{ files_.clear(); }

	// find file path using id
	const Path& GetPath(masm::FileUID fuid) const;

	// prepare next file id if new path is given
	masm::FileUID GetFileUID(const Path& path);
};


#endif

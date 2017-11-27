/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#include "pch.h"
#include "PeripheralRepository.h"
#include "Exceptions.h"


extern PeripheralRepository& GetPeripherals()
{
	static PeripheralRepository pr;
	return pr;
}


PeripheralRepository::PeripheralRepository()
{}

PeripheralRepository::~PeripheralRepository()
{}


bool PeripheralRepository::Register(const char* category, const char* version, CreateFn create_device)
{
	if (create_device == nullptr)
		throw LogicError("empty device creation function cannot be registered " __FUNCTION__);

	auto& map= collection_[category];
	map[version] = create_device;

	return true;
}


std::unique_ptr<Peripheral> PeripheralRepository::Create(const char* version, const PParam& params, PeripheralConfigData& configuration)
{
	auto c= collection_.find(params.category_);
	if (c != collection_.end())
	{
		auto v= c->second.find(version);
		if (v != c->second.end())
			return v->second(params, configuration);
	}

	assert(false);	// requested device does not exist

	return std::unique_ptr<Peripheral>();
}

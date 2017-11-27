/*-----------------------------------------------------------------------------
	ColdFire Macro Assembler and Simulator

	Copyright (C) 2007-2012 Mike Kowalski

	See License.txt for more details
-----------------------------------------------------------------------------*/

#pragma once

#include "Peripheral.h"
#include "Types.h"

// This is repository of all implemented peripheral devices
//
// To create one provide its category/version, location of registers (offset from MBAR), and interrupt line
//


class PeripheralRepository
{
public:
	typedef std::function<std::unique_ptr<Peripheral> (const PParam& params, PeripheralConfigData& configuration)> CreateFn;

	// register device
	bool Register(const char* category, const char* version, CreateFn create_device);

	// create device
	std::unique_ptr<Peripheral> Create(const char* version, const PParam& params, PeripheralConfigData& configuration);

private:
	typedef std::map<std::string, std::map<std::string, CreateFn>> Map;
	// collection of all implemented devices for various MCUs
	Map collection_;

	PeripheralRepository();
	~PeripheralRepository();
	PeripheralRepository(const PeripheralRepository&);
	PeripheralRepository& operator = (const PeripheralRepository&);

	friend PeripheralRepository& GetPeripherals();
};

extern PeripheralRepository& GetPeripherals();


template<class DEV>
std::unique_ptr<Peripheral> CreateDevice(const PParam& params, PeripheralConfigData&)
{ return std::unique_ptr<Peripheral>(new DEV(params)); }

template<class DEV>
std::unique_ptr<Peripheral> CreateDevice2(const PParam& params, PeripheralConfigData& config)
{ return std::unique_ptr<Peripheral>(new DEV(params, config)); }

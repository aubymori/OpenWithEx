#pragma once
#include "openwithex_priv.h"
#include "util.h"

class CProcessAndThreadRefHost
{
private:
	LONG _cRef;
	IUnknown *_punk;

public:
	CProcessAndThreadRefHost();
	~CProcessAndThreadRefHost();
};
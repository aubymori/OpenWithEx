#include "process_and_thread_refhost.h"

CProcessAndThreadRefHost::CProcessAndThreadRefHost()
	: _cRef(0)
	, _punk(nullptr)
{
	SHCreateThreadRef(&_cRef, &_punk);
	SHSetThreadRef(_punk);
	SetProcessReference(_punk);
}

CProcessAndThreadRefHost::~CProcessAndThreadRefHost()
{
	SHSetThreadRef(nullptr);
	SafeRelease(&_punk);

	MSG msg;
	while (_cRef)
	{
		if (GetMessageW(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}
}
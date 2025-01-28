#pragma once
#include <initguid.h>
#include "openwithex.h"
#include "iassociationelement.h"

DEFINE_GUID(IID_IObjectWithAssociationElement, 0xE157C3A1, 0xA532, 0x4DE2, 0x94,0x80, 0x14,0x52,0xB7,0x42,0x6E,0xEE);

MIDL_INTERFACE("E157C3A1-A532-4DE2-9480-1452B7426EEE")
IObjectWithAssociationElement : IUnknown
{
public:
	virtual HRESULT SetAssocElement(IAssociationElement *pae) = 0;
	virtual HRESULT GetAssocElement(REFIID riid, void **ppv) = 0;
};
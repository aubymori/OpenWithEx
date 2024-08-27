#pragma once
#include "openwithex.h"

typedef enum tagASSOCQUERY
{
	ASSOCQUERY_STRING         = 0x00010000,
	ASSOCQUERY_EXISTS         = 0x00020000,
	ASSOCQUERY_DIRECT         = 0x00040000,
	ASSOCQUERY_DWORD          = 0x00080000,
	ASSOCQUERY_INDIRECTSTRING = 0x00100000,
	ASSOCQUERY_OBJECT         = 0x00200000,
	ASSOCQUERY_GUID           = 0x00400000,
	ASSOCQUERY_PATH           = 0x01000000,
	ASSOCQUERY_VERB           = 0x02000000,
	ASSOCQUERY_SECONDARY      = 0x80000000
} ASSOCQUERY;

DEFINE_GUID(IID_IAssocationObject, 0xD8F6AD5B, 0xB44F, 0x4BCC, 0x88,0xFD, 0xEB,0x34,0x73,0xDB,0x75,0x02);

MIDL_INTERFACE("D8F6AD5B-B44F-4BCC-88FD-EB3473DB7502")
IAssociationElement : IUnknown
{
public:
	virtual HRESULT QueryString(ASSOCQUERY flags, PCWSTR lpValueName, PWSTR *ppszOut) = 0;
	virtual HRESULT QueryDword(ASSOCQUERY flags, PCWSTR lpValueName, DWORD *pdwOut) = 0;
	virtual HRESULT QueryExists(ASSOCQUERY flags, PCWSTR lpValueName) = 0;
	virtual HRESULT QueryDirect(ASSOCQUERY flags, PCWSTR lpValueName, FLAGGED_BYTE_BLOB **) = 0;
	virtual HRESULT QueryObject(ASSOCQUERY flags, PCWSTR lpValueName, REFIID riid, void **ppvObject) = 0;
	virtual HRESULT QueryGuid(ASSOCQUERY flags, PCWSTR lpValueName, GUID *pGuidOut) = 0;
};
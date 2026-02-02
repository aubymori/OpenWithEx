#pragma once
#include "openwithex_priv.h"

MIDL_INTERFACE("94724f59-eb2c-4efb-ad2b-8538f6496f7d")
IOpenWithTypeOverride : IUnknown
{
    STDMETHOD(GetOpenWithTypeOverride)(LPWSTR *) PURE;
};

MIDL_INTERFACE("6A283FE2-ECFA-4599-91C4-E80957137B26")
IOpenWithLauncher : public IUnknown
{
	STDMETHOD(Launch)(HWND hwndOwner, LPCWSTR pszFile, IMMERSIVE_OPENWITH_FLAGS flags) PURE;
};

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

MIDL_INTERFACE("D8F6AD5B-B44F-4BCC-88FD-EB3473DB7502")
IAssociationElement : IUnknown
{
public:
	STDMETHOD(QueryString)(ASSOCQUERY flags, PCWSTR lpValueName, PWSTR *ppszOut) PURE;
	STDMETHOD(QueryDword)(ASSOCQUERY flags, PCWSTR lpValueName, DWORD *pdwOut) PURE;
	STDMETHOD(QueryExists)(ASSOCQUERY flags, PCWSTR lpValueName) PURE;
	STDMETHOD(QueryDirect)(ASSOCQUERY flags, PCWSTR lpValueName, FLAGGED_BYTE_BLOB **) PURE;
	STDMETHOD(QueryObject)(ASSOCQUERY flags, PCWSTR lpValueName, REFIID riid, void **ppvObject) PURE;
	STDMETHOD(QueryGuid)(ASSOCQUERY flags, PCWSTR lpValueName, GUID *pGuidOut) PURE;
};

MIDL_INTERFACE("E157C3A1-A532-4DE2-9480-1452B7426EEE")
IObjectWithAssociationElement : IUnknown
{
public:
	STDMETHOD(SetAssocElement)(IAssociationElement *pae) PURE;
	STDMETHOD(GetAssocElement)(REFIID riid, void **ppv) PURE;
};
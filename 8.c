struct /*VFT*/ IApplicationAssociationRegistrationInternalVtbl
{
  HRESULT (__fastcall *ClearUserAssociations)(IApplicationAssociationRegistrationInternal *);
  HRESULT (__fastcall *SetProgIdAsDefault)(IApplicationAssociationRegistrationInternal *, const wchar_t *, const wchar_t *, ASSOCIATIONTYPE);
  HRESULT (__fastcall *SetAppAsDefault)(IApplicationAssociationRegistrationInternal *, const wchar_t *, const wchar_t *, ASSOCIATIONTYPE);
  HRESULT (__fastcall *SetAppAsDefaultAll)(IApplicationAssociationRegistrationInternal *, const wchar_t *);
  HRESULT (__fastcall *QueryAppIsDefault)(IApplicationAssociationRegistrationInternal *, const wchar_t *, ASSOCIATIONTYPE, ASSOCIATIONLEVEL, const wchar_t *, int *);
  HRESULT (__fastcall *QueryAppIsDefaultAll)(IApplicationAssociationRegistrationInternal *, ASSOCIATIONLEVEL, const wchar_t *, int *);
  HRESULT (__fastcall *QueryCurrentDefault)(IApplicationAssociationRegistrationInternal *, const wchar_t *, ASSOCIATIONTYPE, ASSOCIATIONLEVEL, wchar_t **);
  HRESULT (__fastcall *GetDefaultBrowserInfo)(IApplicationAssociationRegistrationInternal *, BROWSER_INFO_TYPE, wchar_t **);
  HRESULT (__fastcall *RestoreDefaultBrowserContractRegistration)(IApplicationAssociationRegistrationInternal *);
  HRESULT (__fastcall *IsBrowserAssociation)(IApplicationAssociationRegistrationInternal *, const wchar_t *, int *);
  HRESULT (__fastcall *ExportUserAssociations)(IApplicationAssociationRegistrationInternal *, const wchar_t *);
  HRESULT (__fastcall *ApplyUserAssociations)(IApplicationAssociationRegistrationInternal *, const wchar_t *);
  HRESULT (__fastcall *UpdateProtocolCapabilityCache)(IApplicationAssociationRegistrationInternal *, const wchar_t *, int);
};

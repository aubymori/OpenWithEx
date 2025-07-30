#pragma once

#include "openwithex.h"

/**
 * Set the default association for the extension or protocol to the specified
 * handler.
 * 
 * This function will diffuse to the correct OS-specific behaviour depending
 * on the user's operating system.
 */
HRESULT SetDefaultAssociation(LPCWSTR szExtOrProtocol, IAssocHandler *pAssocHandler);
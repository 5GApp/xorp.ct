// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2006 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the XORP LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the XORP LICENSE file; the license in that file is
// legally binding.

#ident "$XORP: xorp/fea/fticonfig.cc,v 1.50 2006/04/03 04:04:10 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "win_support.hh"

#ifdef HOST_OS_WINDOWS

#include "libxorp/win_io.h"
#include "win_rtsock.h"

#include <routprot.h>

//
// Helper method to determine if the Routing and Remote Access Service
// is installed and running.
//
bool
WinSupport::is_rras_running()
{
    bool is_installed = false;
    bool is_running = false;

    SC_HANDLE h_scm = OpenSCManager(NULL, NULL, GENERIC_READ);
    if (h_scm != NULL) {
    	SC_HANDLE h_rras = OpenService(h_scm, RRAS_SERVICE_NAME, GENERIC_READ);
	if (h_rras != NULL) {
	    is_installed = true;
	    SERVICE_STATUS ss;
	    if (0 != ControlService(h_rras, SERVICE_CONTROL_INTERROGATE, &ss)) {
		is_running = true;
	    } else {
		DWORD result = GetLastError();
		if (result == ERROR_SERVICE_CANNOT_ACCEPT_CTRL) {
		    is_running = true;
		} else if (result != ERROR_SERVICE_NOT_ACTIVE) {
		    XLOG_WARNING("ControlService() failed: %s",
				 win_strerror(result));
		}
	    }
	    CloseServiceHandle(h_rras);
	} else {
	    DWORD result = GetLastError();
	    if (result != ERROR_SERVICE_DOES_NOT_EXIST) {
		XLOG_WARNING("OpenService() failed: %s", win_strerror(result));
	    }
	}
        CloseServiceHandle(h_scm);
    } else {
	XLOG_WARNING("OpenSCManager() failed: %s",
		     win_strerror(GetLastError()));
    }
    return (is_running && is_installed);
}

int
WinSupport::add_protocol_to_rras(int family)
{
    SHORT XORPRTM_BLOCK_SIZE   = 0x0004;
    XORPRTM_GLOBAL_CONFIG   igc;
    HRESULT	hr = S_OK;
    DWORD       dwErr = ERROR_SUCCESS;
    DWORD       dwErrT = ERROR_SUCCESS;
    MPR_SERVER_HANDLE   hMprServer = NULL;
    HANDLE	hMprConfig = NULL;
    LPBYTE      pByte = NULL;
    LPVOID      pHeader = NULL;
    LPVOID      pNewHeader = NULL;
    DWORD       dwSize = 0;
    HANDLE      hTransport = NULL;
    LPWSTR	pswzServerName = NULL;
    int		pid;

    memset(&igc, 0, sizeof(igc));

#if 0
    if (family == AF_INET) {
	pid = PID_IP;
    } else {
	pid = PID_IPV6;
    }
#else
    pid = PID_IP;
    UNUSED(family); // XXX: No definition of PID_IPV6 yet.
#endif

    dwErr = MprAdminServerConnect(pswzServerName, &hMprServer);
    if (dwErr == ERROR_SUCCESS) {
        dwErr = MprAdminTransportGetInfo(hMprServer, pid, &pByte, &dwSize,
                                         NULL, NULL);
        if (dwErr == ERROR_SUCCESS) {
            MprInfoDuplicate(pByte, &pHeader);
            MprAdminBufferFree(pByte);
            pByte = NULL;
            dwSize = 0;
        }
    }

    dwErrT = MprConfigServerConnect(pswzServerName, &hMprConfig);
    if (dwErrT == ERROR_SUCCESS) {
        dwErrT = MprConfigTransportGetHandle(hMprConfig, pid, &hTransport);
    }

    if (dwErr != ERROR_SUCCESS) {
        MprConfigTransportGetInfo(hMprConfig, hTransport, &pByte, &dwSize,
                                  NULL, NULL, NULL);
        MprInfoDuplicate(pByte, &pHeader);
        MprConfigBufferFree(pByte);
        pByte = NULL;
        dwSize = 0;
    }

    MprInfoBlockRemove(pHeader, PROTO_IP_XORPRTM, &pNewHeader);
    if (pNewHeader != NULL) {
        MprInfoDelete(pHeader);
        pHeader = pNewHeader;
        pNewHeader = NULL;
    }

    MprInfoBlockAdd(pHeader, PROTO_IP_XORPRTM, XORPRTM_BLOCK_SIZE, 1,
                    (LPBYTE)&igc, &pNewHeader);
    MprInfoDelete(pHeader);
    pHeader = NULL;
    
    if (hMprServer) {
        MprAdminTransportSetInfo(hMprServer, pid, (BYTE*)pNewHeader,
				 MprInfoBlockQuerySize(pNewHeader), NULL, 0);
    }
    
    if (hMprConfig && hTransport) {
        MprConfigTransportSetInfo(hMprConfig, hTransport, (BYTE*)pNewHeader,
                                  MprInfoBlockQuerySize(pNewHeader), NULL,
                                  0, NULL);
    }

    if (pHeader)
        MprInfoDelete(pHeader);
    if (pNewHeader)
        MprInfoDelete(pNewHeader);
    if (hMprConfig)
        MprConfigServerDisconnect(hMprConfig);
    if (hMprServer)
        MprAdminServerDisconnect(hMprServer);

    return ((int)hr);
}

int
WinSupport::restart_rras()
{
    SERVICE_STATUS ss;
    SC_HANDLE h_scm;
    SC_HANDLE h_rras;
    DWORD result;
    int tries, fatal;

    h_scm = OpenSCManager(NULL, NULL, GENERIC_READ);
    if (h_scm == NULL) {
	return (-1);
    }

    h_rras = OpenService(h_scm, RRAS_SERVICE_NAME, GENERIC_READ);
    if (h_rras == NULL) {
    	result = GetLastError();
	CloseServiceHandle(h_scm);
	return (-1);
    }

    fatal = 0;

    for (tries = 30; tries > 0; tries++) {
	// Check if the service is running, stopping, or stopped.
	result = ControlService(h_rras, SERVICE_CONTROL_INTERROGATE, &ss);
	if (result == NO_ERROR) {
	    // Stopped; carry on
	    if (ss.dwCurrentState == SERVICE_STOPPED)
		break;
	    // Stopping; poll until it's done
	    if (ss.dwCurrentState == SERVICE_STOP_PENDING) {
		Sleep(1000);
		continue;
	    }
	} else if (result == ERROR_SERVICE_NOT_ACTIVE) {
	    break;
	} else {
	    fatal = 1;
	    break;
	}

	result = ControlService(h_rras, SERVICE_CONTROL_STOP, &ss);
	if (result == ERROR_SERVICE_NOT_ACTIVE) {
	    break;
	} else if (result != NO_ERROR) {
	    fatal = 1;
	    break;
	}
    }

    // XXX: error checking missing
    result = StartService(h_rras, 0, NULL);

    // XXX: We should check that the service started.

    CloseServiceHandle(h_rras);
    CloseServiceHandle(h_scm);

    return (0);
}

static const BYTE DLL_CLSID_IPV4[] = RTMV2_CLSID_IPV4;
static const BYTE DLL_CLSID_IPV6[] = RTMV2_CLSID_IPV6;
static const BYTE DLL_CONFIG_DLL[] = XORPRTM_CONFIG_DLL_NAME;
static const BYTE DLL_NAME_IPV4[] = XORPRTM4_DLL_NAME;
static const BYTE DLL_NAME_IPV6[] = XORPRTM6_DLL_NAME;
static const DWORD DLL_FLAGS = 0x00000002;
static const DWORD DLL_PROTO = PROTO_IP_XORPRTM;
static const BYTE DLL_TITLE_IPV4[] = RTMV2_CLSID_IPV4;
static const BYTE DLL_TITLE_IPV6[] = RTMV2_CLSID_IPV6;
static const BYTE DLL_VENDOR[] = XORPRTM_DLL_VENDOR;
static const BYTE TRACING_DIR[] = XORPRTM_TRACING_PATH;

//
// XXX: There is no error handling in this function whatsoever.
//
int
WinSupport::add_protocol_to_registry(int family)
{
    DWORD result;
    HKEY hKey;

    if (family != AF_INET && family != AF_INET6)
	return (-1);

    result = RegCreateKeyExA(
		HKEY_LOCAL_MACHINE,
		family == AF_INET ? HKLM_XORPRTM4_NAME : HKLM_XORPRTM6_NAME,
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS,
		NULL,
		&hKey,
		NULL);

    RegSetValueExA(hKey, "ConfigDll", 0, REG_SZ, DLL_CONFIG_DLL,
		   sizeof(DLL_CONFIG_DLL));
    if (family == AF_INET) {
	RegSetValueExA(hKey, "ConfigClsId", 0, REG_SZ, DLL_CLSID_IPV4,
		       sizeof(DLL_CLSID_IPV4));
	RegSetValueExA(hKey, "Title", 0, REG_SZ, DLL_TITLE_IPV4,
		       sizeof(DLL_TITLE_IPV4));
	RegSetValueExA(hKey, "DllName", 0, REG_SZ, DLL_NAME_IPV4,
		       sizeof(DLL_NAME_IPV4));
    } else if (family == AF_INET6) {
	RegSetValueExA(hKey, "ConfigClsId", 0, REG_SZ, DLL_CLSID_IPV6,
		       sizeof(DLL_CLSID_IPV6));
	RegSetValueExA(hKey, "Title", 0, REG_SZ, DLL_TITLE_IPV6,
		       sizeof(DLL_TITLE_IPV6));
	RegSetValueExA(hKey, "DllName", 0, REG_SZ, DLL_NAME_IPV6,
		       sizeof(DLL_NAME_IPV6));
    }
    RegSetValueExA(hKey, "Flags", 0, REG_DWORD, (const BYTE*)&DLL_FLAGS,
		    sizeof(DLL_FLAGS));
    RegSetValueExA(hKey, "ProtocolId", 0, REG_DWORD, (const BYTE*)&DLL_PROTO,
		    sizeof(DLL_PROTO));
    RegSetValueExA(hKey, "VendorName", 0, REG_SZ, DLL_VENDOR,
		    sizeof(DLL_VENDOR));
    RegCloseKey(hKey);

#if 1
    //
    // XXX: Enable console tracing for debugging.
    //
    result = RegCreateKeyExA(
		HKEY_LOCAL_MACHINE,
		family == AF_INET ? HKLM_XORPRTM4_TRACING_NAME : HKLM_XORPRTM6_TRACING_NAME,
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS,
		NULL,
		&hKey,
		NULL);

    DWORD foo = 1;
    RegSetValueExA(hKey, "EnableConsoleTracing", 0, REG_DWORD, (BYTE*)&foo, sizeof(foo));
    RegSetValueExA(hKey, "EnableFileTracing", 0, REG_DWORD, (BYTE*)&foo, sizeof(foo));
    foo = 0xFFFF0000;
    RegSetValueExA(hKey, "ConsoleTracingMask", 0, REG_DWORD, (BYTE*)&foo, sizeof(foo));
    RegSetValueExA(hKey, "FileTracingMask", 0, REG_DWORD, (BYTE*)&foo, sizeof(foo));
    foo = 0x00100000;
    RegSetValueExA(hKey, "MaxFileSize", 0, REG_DWORD, (BYTE*)&foo, sizeof(foo));

    RegSetValueExA(hKey, "FileDirectory", 0, REG_EXPAND_SZ, TRACING_DIR, sizeof(TRACING_DIR));

    RegCloseKey(hKey);
#endif // TRACING

    return (0);
}

#else // !HOST_OS_WINDOWS

bool
WinSupport::is_rras_running()
{
    return (false);
}


int
WinSupport::restart_rras()
{
    return (0);
}

int
WinSupport::add_protocol_to_rras(int family)
{
    return (0);
    UNUSED(family);
}

int
WinSupport::add_protocol_to_registry(int family)
{
    return (0);
    UNUSED(family);
}

#endif // HOST_OS_WINDOWS

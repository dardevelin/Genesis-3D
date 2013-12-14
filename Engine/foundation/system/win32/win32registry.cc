/****************************************************************************
Copyright (c) 2007, Radon Labs GmbH
Copyright (c) 2011-2013,WebJet Business Division,CYOU
 
http://www.genesis-3d.com.cn

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/

#if __WIN32__



#include "stdneb.h"
#include "system/win32/win32registry.h"
#include "util/win32/win32stringconverter.h"

namespace System
{
using namespace Win32;
using namespace Util;

//------------------------------------------------------------------------------
/**
    Convert a RootKey value into a Win32 key handle.
*/
HKEY
Win32Registry::RootKeyToWin32KeyHandle(RootKey rootKey)
{
    switch (rootKey)
    {
        case ClassesRoot:   return HKEY_CLASSES_ROOT;
        case CurrentUser:   return HKEY_CURRENT_USER;
        case LocalMachine:  return HKEY_LOCAL_MACHINE;
        case Users:         return HKEY_USERS;
    }
    // can't happen
    n_error("Can't happen!"); 
    return 0;
}

//------------------------------------------------------------------------------
/**
    Return true if a specific entry exists in the registry. To check only
    for the existence of a key without the contained value, pass an 
    empty 'name' string.
*/
bool
Win32Registry::Exists(RootKey rootKey, const String& key, const String& name)
{
    n_assert(key.IsValid());
    HKEY win32RootKey = RootKeyToWin32KeyHandle(rootKey);
    HKEY hKey = 0;
    LONG res = RegOpenKeyEx(win32RootKey,       // hKey
                            key.AsCharPtr(),    // lpSubKey
                            0,                  // ulOptions (reserved)
                            KEY_READ,           // samDesired
                            &hKey);
    if (ERROR_SUCCESS != res)
    {
        // key does not exist
        return false;
    }
    if (name.IsValid())
    {
        res = RegQueryValueEx(hKey,             // hKey
                              name.AsCharPtr(), // lpValueName
                              NULL,             // lpReserved
                              NULL,             // lpType
                              NULL,             // lpData
                              NULL);            // lpcbData
        RegCloseKey(hKey);
        return (ERROR_SUCCESS == res);
    }
    else
    {
        // key exists, value name was empty
        RegCloseKey(hKey);
        return true;
    }
}

//------------------------------------------------------------------------------
/**
    Set a key value in the registry. This will create the key if it doesn't
    exist.
*/
bool
Win32Registry::WriteString(RootKey rootKey, const String& key, const String& name, const String& value)
{
    n_assert(key.IsValid());
    n_assert(name.IsValid());

    HKEY win32RootKey = RootKeyToWin32KeyHandle(rootKey);
    HKEY hKey = 0;
    LONG res = RegCreateKeyEx(win32RootKey,     // hKey
                              key.AsCharPtr(),  // lpSubKey
                              0,                // Reserved
                              NULL,             // lpClass
                              REG_OPTION_NON_VOLATILE,  // dwOptions
                              KEY_ALL_ACCESS,   // samDesired
                              NULL,             // lpSecurityAttribute
                              &hKey,            // phkResult
                              NULL);            // lpdwDisposition
    if (ERROR_SUCCESS == res)
    {
        res = RegSetValueEx(hKey,               // hKey
                            name.AsCharPtr(),   // lpValueName
                            0,                  // Reserved
                            REG_SZ,             // dwType (normal string)
                            (const BYTE*) value.AsCharPtr(),    // lpData
                            value.Length() + 1);                // cbData
        RegCloseKey(hKey);
        return (ERROR_SUCCESS == res);
    }
    else
    {
        return false;
    }
}

//------------------------------------------------------------------------------
/**
    Get a string value from the registry. Fails hard if the key doesn't exists
    (use the Exists() method to make sure that the key exists!).
    NOTE that this method returns an UTF-8 encoded string!
*/
String
Win32Registry::ReadString(RootKey rootKey, const String& key, const String& name)
{
    n_assert(key.IsValid());
    HKEY win32RootKey = RootKeyToWin32KeyHandle(rootKey);
    HKEY hKey = 0;
    LONG res = RegOpenKeyEx(win32RootKey,       // hKey
                            key.AsCharPtr(),    // lpSubKey
                            0,                  // ulOptions (reserved)
                            KEY_READ,           // samDesired
                            &hKey);
    n_assert(ERROR_SUCCESS == res);

    // need to convert the key name to wide string in order to use the wide-string
    // version of RegQueryValueEx
    const int MaxNameLen = 128;
    ushort wideName[MaxNameLen];
    Win32StringConverter::UTF8ToWide(name, wideName, sizeof(wideName));

    // need a buffer for the result
    const int MaxResultLen = 1024;
    ushort wideResult[MaxResultLen];

    // read registry key value as wide char string
    DWORD pcbData = sizeof(wideResult) - 2;
    res = RegQueryValueExW(hKey,                // hKey
                           (LPWSTR)wideName,    // lpValueName
                           NULL,                // lpReserved
                           NULL,                // lpType
                           (LPBYTE)wideResult,  // lpData
                           &pcbData);           // lpcbData
    n_assert(ERROR_SUCCESS == res);
    RegCloseKey(hKey);

    // need to 0-terminate queried value
    n_assert(((pcbData & 1) == 0) && (pcbData < sizeof(wideResult)));
    wideResult[pcbData / 2] = 0;

    String resultString = Win32StringConverter::WideToUTF8(wideResult);    
    return resultString;
}

//------------------------------------------------------------------------------
/**
    Get an int value from the registry. Fails hard if the key doesn't exists
    (use the Exists() method to make sure that the key exists!).
*/
int
Win32Registry::ReadInt(RootKey rootKey, const String& key, const String& name)
{
    n_assert(key.IsValid());
    HKEY win32RootKey = RootKeyToWin32KeyHandle(rootKey);
    HKEY hKey = 0;
    LONG res = RegOpenKeyEx(win32RootKey,       // hKey
                            key.AsCharPtr(),    // lpSubKey
                            0,                  // ulOptions (reserved)
                            KEY_READ,           // samDesired
                            &hKey);
    n_assert(ERROR_SUCCESS == res);

	DWORD dwType = REG_DWORD;
	DWORD dwSize = sizeof(DWORD);
	DWORD result;

	res = RegQueryValueEx(hKey,
						  name.AsCharPtr(), 
						  NULL, 
						  &dwType, 
						  (BYTE*)&result,
						  &dwSize);
    n_assert(ERROR_SUCCESS == res);
	RegCloseKey(hKey);
	return result;
}


//------------------------------------------------------------------------------
/**
    This deletes a complete registry key with all its values.
*/
bool
Win32Registry::Delete(RootKey rootKey, const String& key)
{
    n_assert(key.IsValid());
    HKEY win32RootKey = RootKeyToWin32KeyHandle(rootKey);
    LONG res = RegDeleteKey(win32RootKey,       // hKey
                            key.AsCharPtr());   // lpSubKey
    return (ERROR_SUCCESS == res);
}

//------------------------------------------------------------------------------
/**
    Converts a string (all capitels, e.g. HKEY_CURRENT_USER) into a
    RootKey value.
*/
Win32Registry::RootKey
Win32Registry::AsRootKey(const String &str)
{
    if (str == "HKEY_CLASSES_ROOT") return ClassesRoot;
    else if (str == "HKEY_CURRENT_USER") return CurrentUser;
    else if (str == "HKEY_LOCAL_MACHINE") return LocalMachine;
    else if (str == "HKEY_USERS") return Users;
    else
    {
        n_error("Win32Registry::AsRootKey(): Invalid string '%s'!", str.AsCharPtr());
        return ClassesRoot;
    }
}

} // namespace System


#endif
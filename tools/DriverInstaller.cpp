#if !defined(_WIN32)
#error MicVSTDriverInstaller is Windows-only.
#endif

#include <windows.h>
#include <setupapi.h>
#include <newdev.h>

#include <iostream>
#include <string>
#include <vector>

namespace
{
    constexpr wchar_t kHardwareId[] = L"ROOT\\MicVSTVirtualAudio";
    // Explicit NUL + the compiler-provided terminator => REG_MULTI_SZ double-NUL.
    constexpr wchar_t kHardwareIdMultiSz[] = L"ROOT\\MicVSTVirtualAudio\0";

    std::wstring winError (DWORD code)
    {
        wchar_t* text = nullptr;
        const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER
                          | FORMAT_MESSAGE_FROM_SYSTEM
                          | FORMAT_MESSAGE_IGNORE_INSERTS;
        const DWORD len = FormatMessageW (flags, nullptr, code, 0,
                                          reinterpret_cast<wchar_t*> (&text), 0, nullptr);
        std::wstring result = len != 0 && text != nullptr ? std::wstring (text, len)
                                                          : L"Unknown Windows error";
        if (text != nullptr) LocalFree (text);
        while (! result.empty() && (result.back() == L'\r' || result.back() == L'\n'))
            result.pop_back();
        return result;
    }

    void printError (const wchar_t* operation, DWORD code)
    {
        std::wcerr << operation << L" failed (" << code << L"): " << winError (code) << L"\n";
    }

    std::wstring fullPath (const wchar_t* path)
    {
        const DWORD needed = GetFullPathNameW (path, 0, nullptr, nullptr);
        if (needed == 0) return {};
        std::wstring result (needed, L'\0');
        const DWORD written = GetFullPathNameW (path, needed, result.data(), nullptr);
        if (written == 0 || written >= needed) return {};
        result.resize (written);
        return result;
    }

    bool updateExistingDevice (const std::wstring& infPath, bool& rebootRequired, DWORD& errorOut)
    {
        BOOL reboot = FALSE;
        if (UpdateDriverForPlugAndPlayDevicesW (nullptr,
                                                kHardwareId,
                                                infPath.c_str(),
                                                INSTALLFLAG_FORCE,
                                                &reboot))
        {
            rebootRequired = reboot != FALSE;
            errorOut = ERROR_SUCCESS;
            return true;
        }

        errorOut = GetLastError();
        return false;
    }

    bool createRootDevice (const std::wstring& infPath, bool& rebootRequired)
    {
        GUID classGuid {};
        wchar_t className[MAX_CLASS_NAME_LEN] {};
        DWORD required = 0;
        if (! SetupDiGetINFClassW (infPath.c_str(), &classGuid,
                                   className, MAX_CLASS_NAME_LEN, &required))
        {
            printError (L"SetupDiGetINFClassW", GetLastError());
            return false;
        }

        HDEVINFO set = SetupDiCreateDeviceInfoList (&classGuid, nullptr);
        if (set == INVALID_HANDLE_VALUE)
        {
            printError (L"SetupDiCreateDeviceInfoList", GetLastError());
            return false;
        }

        SP_DEVINFO_DATA device {};
        device.cbSize = sizeof (device);
        bool registered = false;
        bool ok = false;

        if (! SetupDiCreateDeviceInfoW (set, className, &classGuid,
                                        L"MicVST Virtual Audio", nullptr,
                                        DICD_GENERATE_ID, &device))
        {
            printError (L"SetupDiCreateDeviceInfoW", GetLastError());
            goto done;
        }

        if (! SetupDiSetDeviceRegistryPropertyW (
                set, &device, SPDRP_HARDWAREID,
                reinterpret_cast<const BYTE*> (kHardwareIdMultiSz),
                static_cast<DWORD> (sizeof (kHardwareIdMultiSz))))
        {
            printError (L"SetupDiSetDeviceRegistryPropertyW(SPDRP_HARDWAREID)", GetLastError());
            goto done;
        }

        if (! SetupDiCallClassInstaller (DIF_REGISTERDEVICE, set, &device))
        {
            printError (L"SetupDiCallClassInstaller(DIF_REGISTERDEVICE)", GetLastError());
            goto done;
        }
        registered = true;

        {
            DWORD updateError = ERROR_SUCCESS;
            if (! updateExistingDevice (infPath, rebootRequired, updateError))
            {
                printError (L"UpdateDriverForPlugAndPlayDevicesW", updateError);
                goto done;
            }
        }

        ok = true;

    done:
        if (! ok && registered)
        {
            // Best-effort rollback so a failed install does not leave a phantom root devnode.
            SetupDiCallClassInstaller (DIF_REMOVE, set, &device);
        }
        SetupDiDestroyDeviceInfoList (set);
        return ok;
    }

    bool installDriver (const wchar_t* infArg)
    {
        const auto infPath = fullPath (infArg);
        if (infPath.empty())
        {
            printError (L"GetFullPathNameW", GetLastError());
            return false;
        }

        const DWORD attrs = GetFileAttributesW (infPath.c_str());
        if (attrs == INVALID_FILE_ATTRIBUTES || (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0)
        {
            std::wcerr << L"INF does not exist: " << infPath << L"\n";
            return false;
        }

        bool rebootRequired = false;
        DWORD updateError = ERROR_SUCCESS;
        if (updateExistingDevice (infPath, rebootRequired, updateError))
        {
            std::wcout << L"MicVST virtual audio driver updated successfully.\n";
            if (rebootRequired) std::wcout << L"Windows reports that a reboot is required.\n";
            return true;
        }

        if (updateError != ERROR_NO_SUCH_DEVINST)
        {
            printError (L"UpdateDriverForPlugAndPlayDevicesW", updateError);
            return false;
        }

        if (! createRootDevice (infPath, rebootRequired))
            return false;

        std::wcout << L"MicVST Virtual Audio installed.\n"
                   << L"Recording endpoint: MicVST Microphone\n"
                   << L"Internal render endpoint: MicVST Internal Output\n";
        if (rebootRequired) std::wcout << L"Windows reports that a reboot is required.\n";
        return true;
    }

    bool multiSzContains (const wchar_t* data, DWORD bytes, const wchar_t* needle)
    {
        if (data == nullptr || bytes < sizeof (wchar_t)) return false;
        const wchar_t* end = reinterpret_cast<const wchar_t*> (
            reinterpret_cast<const BYTE*> (data) + bytes);
        for (const wchar_t* p = data; p < end && *p != L'\0'; p += wcslen (p) + 1)
            if (_wcsicmp (p, needle) == 0) return true;
        return false;
    }

    bool removeDevices()
    {
        HDEVINFO set = SetupDiGetClassDevsW (nullptr, nullptr, nullptr, DIGCF_ALLCLASSES);
        if (set == INVALID_HANDLE_VALUE)
        {
            printError (L"SetupDiGetClassDevsW", GetLastError());
            return false;
        }

        bool found = false;
        bool allOk = true;
        for (DWORD index = 0;; ++index)
        {
            SP_DEVINFO_DATA device {};
            device.cbSize = sizeof (device);
            if (! SetupDiEnumDeviceInfo (set, index, &device))
            {
                const DWORD err = GetLastError();
                if (err != ERROR_NO_MORE_ITEMS)
                {
                    printError (L"SetupDiEnumDeviceInfo", err);
                    allOk = false;
                }
                break;
            }

            DWORD type = 0;
            DWORD needed = 0;
            SetupDiGetDeviceRegistryPropertyW (set, &device, SPDRP_HARDWAREID,
                                               &type, nullptr, 0, &needed);
            if (needed == 0) continue;

            std::vector<BYTE> buffer (needed + sizeof (wchar_t) * 2, 0);
            if (! SetupDiGetDeviceRegistryPropertyW (set, &device, SPDRP_HARDWAREID,
                                                      &type, buffer.data(), needed, nullptr))
                continue;

            if (! multiSzContains (reinterpret_cast<const wchar_t*> (buffer.data()),
                                   needed, kHardwareId))
                continue;

            found = true;
            if (! SetupDiCallClassInstaller (DIF_REMOVE, set, &device))
            {
                printError (L"SetupDiCallClassInstaller(DIF_REMOVE)", GetLastError());
                allOk = false;
            }
        }

        SetupDiDestroyDeviceInfoList (set);
        if (! found)
            std::wcout << L"MicVST Virtual Audio is not currently installed.\n";
        else if (allOk)
            std::wcout << L"MicVST Virtual Audio device removed.\n";
        return allOk;
    }

    void usage()
    {
        std::wcerr << L"Usage:\n"
                   << L"  MicVSTDriverInstaller install <path-to-VirtualAudioDriver.inf>\n"
                   << L"  MicVSTDriverInstaller remove\n\n"
                   << L"Run from an elevated process.\n";
    }
}

int wmain (int argc, wchar_t** argv)
{
    if (argc >= 2 && _wcsicmp (argv[1], L"install") == 0)
    {
        if (argc != 3) { usage(); return 2; }
        return installDriver (argv[2]) ? 0 : 1;
    }

    if (argc == 2 && _wcsicmp (argv[1], L"remove") == 0)
        return removeDevices() ? 0 : 1;

    usage();
    return 2;
}

#define UNICODE
#define _UNICODE

#include <windows.h>
#include <restartmanager.h>

#include <iostream>
#include <vector>

int wmain(int argc, wchar_t* argv[]) {
    if (argc != 2) {
        std::wcerr << L"usage: lock_owner <path>\n";
        return 2;
    }

    DWORD session = 0;
    wchar_t key[CCH_RM_SESSION_KEY + 1] = {};
    DWORD error = RmStartSession(&session, 0, key);
    if (error != ERROR_SUCCESS) {
        std::wcerr << L"RmStartSession failed: " << error << L"\n";
        return 3;
    }

    LPCWSTR files[] = {argv[1]};
    error = RmRegisterResources(session, 1, files, 0, nullptr, 0, nullptr);
    if (error != ERROR_SUCCESS) {
        RmEndSession(session);
        std::wcerr << L"RmRegisterResources failed: " << error << L"\n";
        return 4;
    }

    UINT needed = 0;
    UINT count = 0;
    DWORD rebootReasons = 0;
    error = RmGetList(session, &needed, &count, nullptr, &rebootReasons);
    if (error == ERROR_SUCCESS && needed == 0) {
        std::wcout << L"NO_LOCK_OWNER\n";
        RmEndSession(session);
        return 0;
    }
    if (error != ERROR_MORE_DATA) {
        RmEndSession(session);
        std::wcerr << L"RmGetList(size) failed: " << error << L"\n";
        return 5;
    }

    std::vector<RM_PROCESS_INFO> processes(needed);
    count = needed;
    error = RmGetList(session, &needed, &count, processes.data(), &rebootReasons);
    if (error != ERROR_SUCCESS) {
        RmEndSession(session);
        std::wcerr << L"RmGetList(data) failed: " << error << L"\n";
        return 6;
    }

    for (UINT i = 0; i < count; ++i) {
        std::wcout << L"PID=" << processes[i].Process.dwProcessId
                   << L" APP=" << processes[i].strAppName
                   << L" TYPE=" << processes[i].ApplicationType
                   << L" STATUS=" << processes[i].AppStatus << L"\n";
    }

    RmEndSession(session);
    return 0;
}

#include "platform/win/process_enumerator.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <tlhelp32.h>

QStringList ProcessEnumerator::runningExecutablePaths() const
{
    QStringList paths;
    const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return paths;
    }

    PROCESSENTRY32W entry = {};
    entry.dwSize = sizeof(entry);
    if (Process32FirstW(snapshot, &entry)) {
        do {
            const HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE,
                                               entry.th32ProcessID);
            if (!process) {
                continue;
            }
            wchar_t buffer[32768];
            DWORD size = static_cast<DWORD>(sizeof(buffer) / sizeof(buffer[0]));
            if (QueryFullProcessImageNameW(process, 0, buffer, &size)) {
                paths.append(QString::fromWCharArray(buffer, static_cast<int>(size)));
            }
            CloseHandle(process);
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return paths;
}

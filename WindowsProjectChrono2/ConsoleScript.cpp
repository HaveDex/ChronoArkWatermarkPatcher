#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include <string>
#include <regex>

DWORD GetProcessIdByName(const wchar_t* processName) {
    DWORD processId = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W processEntry;
        processEntry.dwSize = sizeof(processEntry);
        if (Process32FirstW(snapshot, &processEntry)) {
            do {
                if (_wcsicmp(processEntry.szExeFile, processName) == 0) {
                    processId = processEntry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snapshot, &processEntry));
        }
        CloseHandle(snapshot);
    }
    return processId;
}

bool PatchGameMemory(DWORD processId) {
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, processId);
    if (hProcess == NULL) {
        std::cout << "Failed to open process. Error: " << GetLastError() << std::endl;
        return false;
    }
    const char* searchPattern = "Official 1.";
    const size_t patternSize = strlen(searchPattern);
    const char* replacePattern = "\0\0\0\0\0\0\0\0\0\0";
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    MEMORY_BASIC_INFORMATION memInfo;
    char* p = 0;
    while (VirtualQueryEx(hProcess, p, &memInfo, sizeof(memInfo))) {
        if (memInfo.State == MEM_COMMIT && memInfo.Protect != PAGE_NOACCESS) {
            std::vector<char> buffer(memInfo.RegionSize);
            SIZE_T bytesRead;
            if (ReadProcessMemory(hProcess, p, buffer.data(), memInfo.RegionSize, &bytesRead)) {
                for (size_t i = 0; i < bytesRead - patternSize; ++i) {
                    if (memcmp(buffer.data() + i, searchPattern, patternSize) == 0) {
                        size_t j = i + patternSize;
                        while (j < bytesRead && buffer[j] != ' ' && buffer[j] != '\0') {
                            ++j;
                        }
                        SIZE_T bytesWritten;
                        if (WriteProcessMemory(hProcess, p + i, replacePattern, j - i, &bytesWritten)) {
                            CloseHandle(hProcess);
                            return true;
                        }
                    }
                }
            }
        }
        p += memInfo.RegionSize;
    }
    CloseHandle(hProcess);
    return false;
}

bool PatchModString(DWORD processId) {
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, processId);
    if (hProcess == NULL) {
        std::cout << "Failed to open process. Error: " << GetLastError() << std::endl;
        return false;
    }

    const wchar_t* searchPattern = L"with mod(";
    const size_t patternSize = wcslen(searchPattern) * sizeof(wchar_t);

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    MEMORY_BASIC_INFORMATION memInfo;
    char* p = 0;
    bool found = false;

    while (VirtualQueryEx(hProcess, p, &memInfo, sizeof(memInfo))) {
        if (memInfo.State == MEM_COMMIT && memInfo.Protect != PAGE_NOACCESS) {
            std::vector<char> buffer(memInfo.RegionSize);
            SIZE_T bytesRead;
            if (ReadProcessMemory(hProcess, p, buffer.data(), memInfo.RegionSize, &bytesRead)) {
                for (size_t i = 0; i < bytesRead - patternSize; i += 2) {
                    if (memcmp(buffer.data() + i, searchPattern, patternSize) == 0) {
                        size_t j = i + patternSize;
                        while (j < bytesRead && buffer[j] != ')') {
                            j += 2;
                        }
                        if (j < bytesRead && buffer[j] == ')') {
                            j += 2;
                        }

                        size_t nullSize = j - i;
                        std::vector<char> nullBuffer(nullSize, 0);

                        SIZE_T bytesWritten;
                        if (WriteProcessMemory(hProcess, p + i, nullBuffer.data(), nullSize, &bytesWritten)) {
                            std::cout << "Successfully nulled out 'with mod(x)' string." << std::endl;
                            found = true;
                        }
                    }
                }
            }
        }
        p += memInfo.RegionSize;
    }

    CloseHandle(hProcess);
    return found;
}

int main() {
    std::cout << "Searching for ChronoArk.exe..." << std::endl;
    DWORD processId = 0;
    bool initialPatchDone = false;

    while (true) {
        processId = GetProcessIdByName(L"ChronoArk.exe");
        if (processId != 0) {
            if (!initialPatchDone) {
                std::cout << "Process found. Attempting initial patch..." << std::endl;
                if (PatchGameMemory(processId)) {
                    std::cout << "Successfully patched the game memory!" << std::endl;
                    initialPatchDone = true;
                    Sleep(1500);
                }
                else {
                    std::cout << "Failed to patch the game memory. The string might not be found." << std::endl;
                }
            }

            if (PatchModString(processId)) {
                std::cout << "Successfully patched 'with mod(' string." << std::endl;
            }
        }
        else {
            std::cout << "Game is not running. Waiting..." << std::endl;
            initialPatchDone = false;
        }
        Sleep(500);
    }

    return 0;
}
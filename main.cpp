#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <sdl.h>
#include <Windows.h>

DWORD getProcessInFocus();
void suspendProcess(DWORD processId);
void resumeProcess(DWORD processId);
HWND GetMainWindowHandle(DWORD processId); // Function prototype


// Global variables to track the state of Alt and tilde keys
bool suspended = false;
bool tildePressed = false;
DWORD lastProcessSuspened = 0;

LRESULT CALLBACK KeyboardHook(int nCode, WPARAM wParam, LPARAM lParam) 
{
    if (nCode >= 0)
    {
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
        {
            KBDLLHOOKSTRUCT* pKeyInfo = (KBDLLHOOKSTRUCT*)lParam;
            DWORD key = pKeyInfo->vkCode;

            // Check for Delete key press
            if (key == VK_DELETE)
            {
                std::cout << "Delete key pressed" << std::endl;
                DWORD processId = getProcessInFocus();

                if (processId != 0)             
                {                    
                    if (suspended)                 
                    {
                        suspended = false;
                        resumeProcess(lastProcessSuspened);

                        // Bring the previously suspended window back to the foreground
                        HWND mainWindow = GetMainWindowHandle(lastProcessSuspened);
                        if (mainWindow != NULL)
                        {                            
                            SetForegroundWindow(mainWindow);
                            ShowWindow(mainWindow, SW_RESTORE); // Restore the window if it was minimized
                        }
                    }
                    else
                    {
                        HWND targetWindow = GetForegroundWindow();
                        ShowWindow(targetWindow, SW_MINIMIZE);
                        suspended = true;
                        lastProcessSuspened = processId;
                        suspendProcess(processId);
                    }
                }   
            }
        }
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

DWORD getProcessInFocus()
{
    HWND foregroundWindow = GetForegroundWindow();

    if (foregroundWindow != NULL) 
    {
        DWORD processId;
        GetWindowThreadProcessId(foregroundWindow, &processId);

        std::cout << "Process ID of the window in focus: " << processId << std::endl;
        return processId;
    }
    else 
    {
        std::cout << "No window in focus." << std::endl;
        return 0;
    }
}

HWND GetMainWindowHandle(DWORD processId)
{
    HWND mainWindow = NULL;

    // Enumerate all windows
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        DWORD windowProcessId;
        GetWindowThreadProcessId(hwnd, &windowProcessId);

        if (windowProcessId == static_cast<DWORD>(lParam)) {
            // Found a window belonging to the target process
            // Assume it's the main window
            *(HWND*)lParam = hwnd;
            return FALSE; // Stop enumerating
        }
        return TRUE; // Continue enumerating
        }, (LPARAM)&mainWindow);

    return mainWindow;
}

void suspendProcess(DWORD processId)
{
    if (processId != 0)
    {
        HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

        THREADENTRY32 threadEntry;
        threadEntry.dwSize = sizeof(THREADENTRY32);

        Thread32First(hThreadSnapshot, &threadEntry);

        do
        {
            if (threadEntry.th32OwnerProcessID == processId)
            {
                HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE,
                    threadEntry.th32ThreadID);

                SuspendThread(hThread);
                CloseHandle(hThread);
            }
        } while (Thread32Next(hThreadSnapshot, &threadEntry));

        CloseHandle(hThreadSnapshot);
    }
}


void resumeProcess(DWORD processId)
{
    if (processId != 0)
    {
        HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

        THREADENTRY32 threadEntry;
        threadEntry.dwSize = sizeof(THREADENTRY32);

        Thread32First(hThreadSnapshot, &threadEntry);

        do
        {
            if (threadEntry.th32OwnerProcessID == processId)
            {
                HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE,
                    threadEntry.th32ThreadID);

                ResumeThread(hThread);
                CloseHandle(hThread);
            }
        } while (Thread32Next(hThreadSnapshot, &threadEntry));

        CloseHandle(hThreadSnapshot);
    }
}

int main(int argc, char* args[]) 
{
    HHOOK hook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHook, NULL, 0);

    if (hook == NULL) {
        std::cerr << "Failed to install keyboard hook" << std::endl;
        return 1;
    }

    // Message loop to keep the program running
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Unhook the keyboard hook
    UnhookWindowsHookEx(hook);


    bool quit = false;


    while (!quit)
    {

    }

    SDL_Quit();
    return 0;
}

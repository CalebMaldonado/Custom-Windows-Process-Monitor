// FirstVisualStudioProject.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>

using namespace std;

int main()
{
    // This section is to get the process list
    HANDLE SnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (SnapShot == INVALID_HANDLE_VALUE) {
        cout << "Failed to get a snapshot of all the processes running" << endl;
        return 1;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32FirstW(SnapShot, &pe32)) {
        cout << "Failed to get the first process" << endl;
        CloseHandle(SnapShot);
        return 1;
    }else {
        do {
            wcout << "Process Name: " << pe32.szExeFile << " ( " << pe32.th32ProcessID << " )" << endl;
        } while (Process32NextW(SnapShot, &pe32));
     }

}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file

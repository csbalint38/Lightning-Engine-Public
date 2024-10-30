#include <Windows.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <comutil.h>
#include <vector>

typedef void(__stdcall* OpenVisualStudioFunc)(const wchar_t*);
typedef void(__stdcall* CloseVisualStudioFunc)();
typedef void(__stdcall* AddFilesFunc)(const wchar_t*, const wchar_t*, const wchar_t**, int);
typedef void(__stdcall* BuildSolutionFunc)(const wchar_t*, const wchar_t*, bool);
typedef bool(__stdcall* GetLastBuildInfoFunc)();

int main() {
    const wchar_t* dll_path = L"C:/Users/balin/Documents/Lightning-Engine/x64/DebugEditor/vsidll.dll";

    HMODULE hModule = LoadLibraryW(dll_path);

    if (hModule == NULL) {
        std::cerr << "Failed to load the DLL!" << std::endl;
        return 1;
    }

    OpenVisualStudioFunc open_visual_studio = (OpenVisualStudioFunc)GetProcAddress(hModule, "open_visual_studio");
    CloseVisualStudioFunc close_visual_studio = (CloseVisualStudioFunc)GetProcAddress(hModule, "close_visual_studio");
    AddFilesFunc add_files = (AddFilesFunc)GetProcAddress(hModule, "add_files");
    BuildSolutionFunc build_solution = (BuildSolutionFunc)GetProcAddress(hModule, "build_solution");
    GetLastBuildInfoFunc get_last_build_info = (GetLastBuildInfoFunc)GetProcAddress(hModule, "get_last_build_info");

    if (!open_visual_studio || !close_visual_studio) {
        std::cerr << "Failed to retrieve function pointers!" << std::endl;
        FreeLibrary(hModule);
        return 1;
    }

    const wchar_t* solution_path = L"C:\\Users\\balin\\Documents\\LightningProjects\\NewProject\\NewProject.sln";
    const wchar_t* project = L"NewProject";
    std::vector<const wchar_t*> files { L"C:/Users/balin/Documents/Lightning-Engine/VisualStudioDLLTest/example.cpp" };
    const wchar_t** file_array = files.data();

    std::wcout << L"Opening Visual Studio with solution: " << solution_path << std::endl;

    add_files(solution_path, project, file_array, 1);

    const wchar_t* config = L"DebugEditor";
    build_solution(solution_path, config, true);
    bool result = false;
    result = get_last_build_info();

    if (result) {
        std::wcout << L"Build succeeded!" << std::endl;
    }
    else {
        std::wcout << L"Build failed or is currently debugging." << std::endl;
    }

    std::wcout << L"Press Enter to close the program and Visual Studio..." << std::endl;
    std::cin.get();

    FreeLibrary(hModule);

    return 0;
}

#include "pch.h"
#include "PlatformProcess.h"
#include <windows.h>
#include <shellapi.h>

using std::filesystem::path;
namespace fs = std::filesystem;

// FWideString 즉 UTF-16 형식으로 된 문자열만 열 수 있음
void FPlatformProcess::OpenFileInDefaultEditor(const FWideString& RelativePath)
{
    // 1. 상대 경로를 기반으로 절대 경로를 생성
    //    (현재 작업 디렉토리 기준)
    fs::path AbsolutePath;
    try
    {
        AbsolutePath = fs::absolute(RelativePath);
    }
    catch (const fs::filesystem_error& e)
    {
        // 경로 변환 실패 처리 (예: GConsole->LogError(...))
        UE_LOG(e.what());
        MessageBoxA(nullptr, "경로 변환에 실패했습니다.", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    // 2. 변환된 절대 경로를 사용
    HINSTANCE hInst = ShellExecuteA(
        NULL,
        "open",
        AbsolutePath.string().c_str(), // .string()으로 std::string을 얻어 c_str() 호출
        NULL,
        NULL,
        SW_SHOWNORMAL
    );

    if ((INT_PTR)hInst <= 32)
    {
        MessageBoxA(NULL, "파일 열기에 실패했습니다.", "Error", MB_OK | MB_ICONERROR);
    }
}

std::filesystem::path FPlatformProcess::OpenSaveFileDialog(const FWideString BaseDir, const FWideString Extension, const FWideString Description, const FWideString DefaultFileName)
{
    OPENFILENAMEW Ofn;
    wchar_t SzFile[260] = {}; // 선택된 전체 파일 경로를 수신할 버퍼
    wchar_t SzInitialDir[260] = {};

    // GetSaveFileNameW 호출 전에 SzFile 버퍼에 기본 파일명 복사
    if (!DefaultFileName.empty())
    {
        // 예: "Untitled.scene"
        wcscpy_s(SzFile, _countof(SzFile), DefaultFileName.c_str());
    }

    // 기본 경로 설정
    fs::path AbsoluteBaseDir = fs::absolute(BaseDir);
    // 디렉토리가 없으면 생성
    if (!fs::exists(AbsoluteBaseDir) || !fs::is_directory(AbsoluteBaseDir))
    {
        fs::create_directories(AbsoluteBaseDir);
    }
    wcscpy_s(SzInitialDir, AbsoluteBaseDir.wstring().c_str());

    // --- 확장자 정리 (e.g., ".scene" -> "scene") ---
    // (FString에 .StartsWith, .RightChop이 있다고 가정)
    FWideString CleanExtension = Extension;
    if (CleanExtension.starts_with(L"."))
    {
        CleanExtension = CleanExtension.substr(1);
    }

    // --- 필터 문자열 동적 생성 ---
    // "Scene Files\0*.scene\0All Files\0*.*\0\0" 형식
    FWideString FilterString;
    FilterString += Description.c_str();           // e.g., "Scene Files"
    FilterString.push_back(L'\0');                 // \0
    FilterString += L"*." + CleanExtension; // e.g., "*.scene"
    FilterString.push_back(L'\0');                 // \0
    FilterString += L"All Files";
    FilterString.push_back(L'\0');                 // \0
    FilterString += L"*.*";
    FilterString.push_back(L'\0');                 // \0
    FilterString.push_back(L'\0');                 // \0 (Double null-termination)

    // --- 다이얼로그 타이틀 동적 생성 ---
    FWideString TitleString = L"Save " + Description;

    // Initialize OPENFILENAME
    ZeroMemory(&Ofn, sizeof(Ofn));
    Ofn.lStructSize = sizeof(Ofn);
    Ofn.hwndOwner = GetActiveWindow();
    Ofn.lpstrFile = SzFile;
    Ofn.nMaxFile = sizeof(SzFile) / sizeof(wchar_t);

    // --- 동적 값 할당 ---
    Ofn.lpstrFilter = FilterString.c_str();
    Ofn.nFilterIndex = 1;
    Ofn.lpstrFileTitle = nullptr;
    Ofn.nMaxFileTitle = 0;
    Ofn.lpstrInitialDir = SzInitialDir;
    Ofn.lpstrTitle = TitleString.c_str();
    Ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_ENABLESIZING;
    Ofn.lpstrDefExt = CleanExtension.c_str();


    if (GetSaveFileNameW(&Ofn) == TRUE)
    {
        // --- 파일 경로가 아닌 '부모 디렉토리'가 없으면 생성 ---
        fs::path FilePath = SzFile;
        fs::path ParentDir = FilePath.parent_path();

        if (!fs::exists(ParentDir))
        {
            fs::create_directories(ParentDir);
        }

        return FilePath;
    }

    return L"";
}

// 사용자가 선택한 파일의 전체 경로. 취소 시 빈 경로.
std::filesystem::path FPlatformProcess::OpenLoadFileDialog(const FWideString BaseDir, const FWideString Extension, const FWideString Description)
{
    OPENFILENAMEW Ofn;
    wchar_t SzFile[260] = {}; // 선택된 전체 파일 경로를 수신할 버퍼
    wchar_t SzInitialDir[260] = {};

    // 1. 기본 경로 설정
    fs::path AbsoluteBaseDir = fs::absolute(BaseDir);
    // (디렉토리가 없으면 생성)
    if (!fs::exists(AbsoluteBaseDir) || !fs::is_directory(AbsoluteBaseDir))
    {
        fs::create_directories(AbsoluteBaseDir);
    }
    wcscpy_s(SzInitialDir, AbsoluteBaseDir.wstring().c_str());

    // 2. 확장자 정리 (e.g., ".scene" -> "scene")
    FWideString CleanExtension = Extension;
    if (CleanExtension.starts_with(L"."))
    {
        CleanExtension = CleanExtension.substr(1);
    }

    // 3. 필터 문자열 동적 생성
    // "Scene Files\0*.scene\0All Files\0*.*\0\0" 형식
    FWideString FilterString;
    FilterString += Description.c_str();      // e.g., "Scene Files"
    FilterString.push_back(L'\0');            // \0
    FilterString += L"*." + CleanExtension; // e.g., "*.scene"
    FilterString.push_back(L'\0');            // \0
    FilterString += L"All Files";
    FilterString.push_back(L'\0');            // \0
    FilterString += L"*.*";
    FilterString.push_back(L'\0');            // \0
    FilterString.push_back(L'\0');            // \0 (Double null-termination)

    // 4. 다이얼로그 타이틀 동적 생성
    FWideString TitleString = L"Open " + Description;

    // 5. OPENFILENAME 구조체 초기화
    ZeroMemory(&Ofn, sizeof(Ofn));
    Ofn.lStructSize = sizeof(Ofn);
    Ofn.hwndOwner = GetActiveWindow(); // 또는 특정 윈도우 핸들
    Ofn.lpstrFile = SzFile; // [중요] 비어있는 버퍼를 전달
    Ofn.nMaxFile = sizeof(SzFile) / sizeof(wchar_t);
    Ofn.lpstrFilter = FilterString.c_str();
    Ofn.nFilterIndex = 1;
    Ofn.lpstrFileTitle = nullptr;
    Ofn.nMaxFileTitle = 0;
    Ofn.lpstrInitialDir = SzInitialDir;
    Ofn.lpstrTitle = TitleString.c_str();
    Ofn.lpstrDefExt = CleanExtension.c_str();

    // 6. 플래그 설정
    // OFN_OVERWRITEPROMPT (덮어쓰기) 제거
    // OFN_FILEMUSTEXIST (파일이 존재해야 함) 추가
    Ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER | OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_ENABLESIZING;

    // 7. GetOpenFileNameW 호출
    if (GetOpenFileNameW(&Ofn) == TRUE)
    {
        // 사용자가 선택한 파일 경로 반환
        return fs::path(SzFile);
    }

    return L""; // 취소 시 빈 경로 반환
}
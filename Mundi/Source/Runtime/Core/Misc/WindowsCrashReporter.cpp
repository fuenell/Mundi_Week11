#include "pch.h"
#include "WindowsCrashReporter.h"
#include <Windows.h>
#include <dbghelp.h>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>

#pragma comment(lib, "dbghelp.lib")

namespace
{
	// 지정된 예외 정보를 바탕으로 MiniDump 파일을 생성합니다.
	void CreateMiniDump(EXCEPTION_POINTERS* PExceptionInfo)
	{
		auto Now = std::chrono::system_clock::now();
		auto InTimeT = std::chrono::system_clock::to_time_t(Now);

		std::tm LocalTime = {};
		localtime_s(&LocalTime, &InTimeT);

		std::wstringstream FileNameStream;
		FileNameStream << L"Minidump_";
		FileNameStream << std::put_time(&LocalTime, L"%Y%m%d_%H%M%S");
		FileNameStream << L".dmp";
		FWideString DumpFileName = FileNameStream.str();

		HANDLE DumpFileHandle = CreateFileW(
			DumpFileName.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
			NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL
		);

		if (DumpFileHandle == INVALID_HANDLE_VALUE)
		{
			OutputDebugStringW(L"[Crash] 덤프 파일 생성에 실패했습니다.");
			return;
		}

		MINIDUMP_EXCEPTION_INFORMATION DumpExceptionInfo;
		DumpExceptionInfo.ThreadId = GetCurrentThreadId();
		DumpExceptionInfo.ExceptionPointers = PExceptionInfo;
		DumpExceptionInfo.ClientPointers = FALSE;

		BOOL bResult = MiniDumpWriteDump(
			GetCurrentProcess(), GetCurrentProcessId(), DumpFileHandle, MiniDumpNormal,
			PExceptionInfo ? &DumpExceptionInfo : NULL, NULL, NULL
		);

		if (bResult)
		{
			FWideString LogMessage = L"[Crash] 덤프 파일 생성 완료: " + DumpFileName;
			OutputDebugStringW(LogMessage.c_str());
		}
		else
		{
			OutputDebugStringW(L"[Crash] MiniDumpWriteDump 실패했습니다.");
		}

		CloseHandle(DumpFileHandle);
	}

	LONG WINAPI EngineUnhandledExceptionFilter(EXCEPTION_POINTERS* PExceptionInfo)
	{
		CreateMiniDump(PExceptionInfo);
		return EXCEPTION_EXECUTE_HANDLER;
	}

	// vptr 교환을 시도하여 타입 혼동(Type Confusion)을 유발합니다.
	void ExecuteVTableSwap()
	{
		// '클래스'를 키로, '해당 클래스의 모든 객체 배열'을 값으로 갖는 맵을 생성합니다. (GUObjectArray에는 같은 클래스가 많아서 다양한 오류를 발생하게 하기 위해 압축)
		TMap<UClass*, TArray<UObject*>> ClassToInstancesMap;
		for (int32 i = 0; i < GUObjectArray.Num(); ++i)
		{
			UObject* Obj = GUObjectArray[i];
			if (Obj)
			{
				UClass* ClassType = Obj->GetClass();
				ClassToInstancesMap[ClassType].Add(Obj);
			}
		}

		// 고유 클래스가 2개 미만이면 교환이 불가능합니다.
		if (ClassToInstancesMap.Num() < 2)
		{
			UE_LOG("[warning][CRASH] vptr 교환 실패: 고유 클래스가 2개 미만입니다.");
			// [대체 크래시]
			volatile int* PNull = nullptr;
			*PNull = 0xDEADBEEF;
			return;
		}

		// TMap의 모든 키(UClass*)를 TArray로 변환하여 무작위 접근을 준비합니다.
		TArray<UClass*> UniqueClasses;
		UniqueClasses = ClassToInstancesMap.GetKeys();

		// '서로 다른' 두 개의 '클래스 인덱스'를 무작위로 선택합니다.
		int32 Index1 = rand() % UniqueClasses.Num();
		int32 Index2 = rand() % UniqueClasses.Num();

		int Attempts = 0;
		while (Index1 == Index2 && Attempts < 100) // 무한 루프 방지
		{
			Index2 = rand() % UniqueClasses.Num();
			Attempts++;
		}

		// 선택된 두 클래스를 가져옵니다.
		UClass* Class1 = UniqueClasses[Index1];
		UClass* Class2 = UniqueClasses[Index2];

		// 각 클래스의 '인스턴스 배열'을 가져와, 그 안에서 '무작위 객체'를 선택합니다.
		TArray<UObject*>& Instances1 = ClassToInstancesMap[Class1];
		UObject* Object1 = Instances1[rand() % Instances1.Num()];

		TArray<UObject*>& Instances2 = ClassToInstancesMap[Class2];
		UObject* Object2 = Instances2[rand() % Instances2.Num()];

		UE_LOG("[warning][CRASH] 지뢰 설치: 클래스 [%s]의 무작위 인스턴스와 클래스 [%s]의 무작위 인스턴스 vptr을 교환합니다.", Class1->Name, Class2->Name);

		// vptr 교환
		void** VTablePtr1 = (void**)Object1;
		void** VTablePtr2 = (void**)Object2;

		void* VTable1 = *VTablePtr1;
		void* VTable2 = *VTablePtr2;

		*VTablePtr1 = VTable2;
		*VTablePtr2 = VTable1;
	}
}

bool FWindowsCrashReporter::bRequestCrash = false;

void FWindowsCrashReporter::Initialize()
{
	SetUnhandledExceptionFilter(EngineUnhandledExceptionFilter);

	srand(static_cast<unsigned int>(time(NULL)));
}

// 덤프 파일 생성 테스트를 위한 크래시 유발 함수
void FWindowsCrashReporter::MakeRandomCrash()
{
	bRequestCrash = true;
	UE_LOG("[warning][CRASH] 주기적 vptr 훼손을 시작합니다.");
}

void FWindowsCrashReporter::Tick(float DeltaTime)
{
	// bRequestCrash가 true일 때 매 프레임 ExecuteVTableSwap를 호출해서 크래시 유발
	if (bRequestCrash)
	{
		ExecuteVTableSwap();
	}
}

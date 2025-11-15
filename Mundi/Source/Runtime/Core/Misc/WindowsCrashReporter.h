#pragma once

// Windows 플랫폼용 크래시 리포팅(MiniDump) 시스템
class FWindowsCrashReporter
{
public:
	static void Initialize();

	static void MakeRandomCrash();

	static void Tick(float DeltaTime);

private:
	static bool bRequestCrash;
};

// Copyright Epic Games, Inc. All Rights Reserved.

#include "pch.h"
#include "ContentBrowserWindow.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include <algorithm>

IMPLEMENT_CLASS(UContentBrowserWindow)

UContentBrowserWindow::UContentBrowserWindow()
	: SelectedFile(nullptr)
	, SelectedIndex(-1)
	, LastClickTime(0.0)
	, LastClickedIndex(-1)
	, ThumbnailSize(80.0f)
	, ColumnsCount(4)
	, bShowFolders(true)
	, bShowFiles(true)
{
	FUIWindowConfig Config;
	Config.WindowTitle = "Content Browser";
	Config.DefaultSize = ImVec2(800, 400);
	Config.DefaultPosition = ImVec2(100, 500);
	Config.MinSize = ImVec2(400, 300);
	Config.bResizable = true;
	Config.bMovable = true;
	Config.bCollapsible = true;
	Config.DockDirection = EUIDockDirection::Bottom;

	Config.UpdateWindowFlags();
	SetConfig(Config);

	// 루트 경로를 Data 폴더로 설정
	RootPath = std::filesystem::current_path() / "Data";
	CurrentPath = RootPath;
}

UContentBrowserWindow::~UContentBrowserWindow()
{
	Cleanup();
}

void UContentBrowserWindow::Initialize()
{
	UE_LOG("ContentBrowserWindow: Successfully Initialized");

	// UIWindow 부모 Initialize 호출
	UUIWindow::Initialize();

	RefreshCurrentDirectory();
}

void UContentBrowserWindow::RenderWindow()
{
	// 숨겨진 상태면 렌더링하지 않음
	if (!IsVisible())
	{
		return;
	}

	// ImGui 윈도우 시작
	ImGui::SetNextWindowSize(GetConfig().DefaultSize, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos(GetConfig().DefaultPosition, ImGuiCond_FirstUseEver);

	bool bIsOpen = true;

	if (ImGui::Begin(GetConfig().WindowTitle.c_str(), &bIsOpen, GetConfig().WindowFlags))
	{
		// 실제 UI 컨텐츠 렌더링
		RenderPathBar();
		RenderContentGrid();
	}
	ImGui::End();

	// 윈도우가 닫혔는지 확인
	if (!bIsOpen)
	{
		SetWindowState(EUIWindowState::Hidden);
	}
}

void UContentBrowserWindow::Cleanup()
{
	DisplayedFiles.clear();
	SelectedFile = nullptr;
}

void UContentBrowserWindow::NavigateToPath(const std::filesystem::path& NewPath)
{
	if (std::filesystem::exists(NewPath) && std::filesystem::is_directory(NewPath))
	{
		CurrentPath = NewPath;
		RefreshCurrentDirectory();
		SelectedIndex = -1;
		SelectedFile = nullptr;
	}
}

void UContentBrowserWindow::RefreshCurrentDirectory()
{
	DisplayedFiles.clear();

	if (!std::filesystem::exists(CurrentPath))
	{
		UE_LOG("ContentBrowserWindow: Path does not exist: %s", CurrentPath.string().c_str());
		CurrentPath = RootPath;
		return;
	}

	try
	{
		// 먼저 폴더들을 추가
		if (bShowFolders)
		{
			for (const auto& entry : std::filesystem::directory_iterator(CurrentPath))
			{
				if (entry.is_directory())
				{
					FFileEntry FileEntry;
					FileEntry.Path = entry.path();
					FileEntry.FileName = FString(entry.path().filename().string().c_str());
					FileEntry.bIsDirectory = true;
					FileEntry.FileSize = 0;
					DisplayedFiles.push_back(FileEntry);
				}
			}
		}

		// 그 다음 파일들을 추가
		if (bShowFiles)
		{
			for (const auto& entry : std::filesystem::directory_iterator(CurrentPath))
			{
				if (entry.is_regular_file())
				{
					FFileEntry FileEntry;
					FileEntry.Path = entry.path();
					FileEntry.FileName = FString(entry.path().filename().string().c_str());
					FileEntry.Extension = FString(entry.path().extension().string().c_str());
					FileEntry.bIsDirectory = false;
					FileEntry.FileSize = std::filesystem::file_size(entry.path());
					DisplayedFiles.push_back(FileEntry);
				}
			}
		}

		UE_LOG("ContentBrowserWindow: Loaded %d items from %s", DisplayedFiles.size(), CurrentPath.string().c_str());
	}
	catch (const std::exception& e)
	{
		UE_LOG("ContentBrowserWindow: Error reading directory: %s", e.what());
	}
}

void UContentBrowserWindow::RenderPathBar()
{
	ImGui::Text("Path: ");
	ImGui::SameLine();

	// 뒤로 가기 버튼
	if (CurrentPath != RootPath)
	{
		if (ImGui::Button("<- Back"))
		{
			NavigateToPath(CurrentPath.parent_path());
		}
		ImGui::SameLine();
	}

	// 현재 경로 표시
	FString PathStr = FString(CurrentPath.string().c_str());
	ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "%s", PathStr.c_str());

	// 새로고침 버튼
	ImGui::SameLine(ImGui::GetWindowWidth() - 100);
	if (ImGui::Button("Refresh"))
	{
		RefreshCurrentDirectory();
	}

	ImGui::Separator();
}

void UContentBrowserWindow::RenderContentGrid()
{
	ImGuiStyle& style = ImGui::GetStyle();
	float windowWidth = ImGui::GetContentRegionAvail().x;
	float cellSize = ThumbnailSize + style.ItemSpacing.x;

	// 동적으로 컬럼 수 계산
	ColumnsCount = std::max(1, (int)(windowWidth / cellSize));

	ImGui::BeginChild("ContentArea", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

	int columnIndex = 0;

	for (int i = 0; i < DisplayedFiles.size(); ++i)
	{
		FFileEntry& entry = DisplayedFiles[i];

		ImGui::PushID(i);
		RenderFileItem(entry, i);
		ImGui::PopID();

		columnIndex++;
		if (columnIndex < ColumnsCount)
		{
			ImGui::SameLine();
		}
		else
		{
			columnIndex = 0;
		}
	}

	ImGui::EndChild();
}

void UContentBrowserWindow::RenderFileItem(FFileEntry& Entry, int Index)
{
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec2 buttonSize(ThumbnailSize, ThumbnailSize);

	// 선택 상태에 따라 색상 변경
	bool isSelected = (Index == SelectedIndex);
	if (isSelected)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
	}

	// 아이콘 버튼
	const char* icon = GetIconForFile(Entry);
	if (ImGui::Button(icon, buttonSize))
	{
		// 클릭 처리
		SelectedIndex = Index;
		SelectedFile = &Entry;

		// 더블클릭 감지
		double currentTime = ImGui::GetTime();
		if (LastClickedIndex == Index && (currentTime - LastClickTime) < 0.3)
		{
			HandleDoubleClick(Entry);
			LastClickTime = 0.0; // 더블클릭 처리 후 리셋
		}
		else
		{
			LastClickTime = currentTime;
			LastClickedIndex = Index;
		}
	}

	if (isSelected)
	{
		ImGui::PopStyleColor();
	}

	// 드래그 소스 처리
	HandleDragSource(Entry);

	// 파일 이름 표시 (텍스트 줄바꿈)
	ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ThumbnailSize);
	ImGui::TextWrapped("%s", Entry.FileName.c_str());
	ImGui::PopTextWrapPos();
}

void UContentBrowserWindow::HandleDragSource(FFileEntry& Entry)
{
	if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
	{
		// 파일 경로를 페이로드로 전달
		std::string pathStr = Entry.Path.string();
		ImGui::SetDragDropPayload("ASSET_FILE", pathStr.c_str(), pathStr.size() + 1);

		// 드래그 중 표시할 툴팁
		ImGui::Text("Move %s", Entry.FileName.c_str());
		if (!Entry.Extension.empty())
		{
			ImGui::Text("Type: %s", Entry.Extension.c_str());
		}

		ImGui::EndDragDropSource();
	}
}

void UContentBrowserWindow::HandleDoubleClick(FFileEntry& Entry)
{
	if (Entry.bIsDirectory)
	{
		// 폴더인 경우 해당 폴더로 이동
		NavigateToPath(Entry.Path);
	}
	else
	{
		// 파일인 경우 뷰어 열기 (다음 단계에서 구현 예정)
		UE_LOG("ContentBrowserWindow: Double-clicked file: %s", Entry.FileName.c_str());

		// TODO: Phase 8에서 구현
		// if (Entry.Extension == ".fbx")
		// {
		//     Open SkeletalMeshViewer
		// }
	}
}

const char* UContentBrowserWindow::GetIconForFile(const FFileEntry& Entry) const
{
	if (Entry.bIsDirectory)
	{
		return "[DIR]";
	}

	// 확장자에 따라 아이콘 반환
	std::string ext = Entry.Extension.c_str();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	if (ext == ".fbx" || ext == ".obj")
	{
		return "[MESH]";
	}
	else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".dds" || ext == ".tga")
	{
		return "[IMG]";
	}
	else if (ext == ".hlsl" || ext == ".glsl" || ext == ".fx")
	{
		return "[SHDR]";
	}
	else if (ext == ".wav" || ext == ".mp3" || ext == ".ogg")
	{
		return "[SND]";
	}
	else if (ext == ".mat")
	{
		return "[MAT]";
	}
	else if (ext == ".level" || ext == ".json")
	{
		return "[DATA]";
	}

	return "[FILE]";
}

FString UContentBrowserWindow::FormatFileSize(uintmax_t Size) const
{
	const char* units[] = { "B", "KB", "MB", "GB" };
	int unitIndex = 0;
	double size = static_cast<double>(Size);

	while (size >= 1024.0 && unitIndex < 3)
	{
		size /= 1024.0;
		unitIndex++;
	}

	char buffer[64];
	sprintf_s(buffer, "%.2f %s", size, units[unitIndex]);
	return FString(buffer);
}

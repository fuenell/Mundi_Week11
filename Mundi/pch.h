#pragma once

// Feature Flags
// Uncomment to enable DDS texture caching (faster loading, uses Data/TextureCache/)
#define USE_DDS_CACHE
#define USE_OBJ_CACHE

#define IMGUI_DEFINE_MATH_OPERATORS	// Imgui에서 곡선 표시를 위한 전용 벡터 연산자 활성화

// Standard Library (MUST come before UEContainer.h)
#include <vector>
#include <map>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <stack>
#include <list>
#include <deque>
#include <string>
#include <array>
#include <algorithm>
#include <functional>
#include <memory>
#include <cmath>
#include <limits>
#include <iostream>
#include <fstream>
#include <utility>
#include <filesystem>
#include <sstream>
#include <iterator>

// Windows & DirectX
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <cassert>

// d3dtk
#include "SimpleMath.h"

// ImGui
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"

// nlohmann
#include "nlohmann/json.hpp"

namespace json { class JSON; }
using JSON = json::JSON;

// Core Project Headers
#include "Vector.h"
#include "ResourceData.h"
#include "VertexData.h"
#include "UEContainer.h"
#include "Name.h"
#include "PathUtils.h"
#include "Object.h"
#include "ObjectFactory.h"
#include "ObjectMacros.h"
#include "Enums.h"
#include "GlobalConsole.h"
#include "D3D11RHI.h"
#include "World.h"
#include "ConstantBufferType.h"

// Manager
#include "Renderer.h"
#include "InputManager.h"
#include "UIManager.h"
#include "ResourceManager.h"

#include "JsonSerializer.h"

#define RESOURCE UResourceManager::GetInstance()
#define UI UUIManager::GetInstance()
#define INPUT UInputManager::GetInstance()
#define RENDER URenderManager::GetInstance()
#define SLATE USlateManager::GetInstance()

//(월드 별 소유)
//#define PARTITION UWorldPartitionManager::GetInstance()
//#define SELECTION (GEngine.GetDefaultWorld()->GetSelectionManager())

extern TMap<FString, FString> EditorINI;
extern const FString GDataDir;
extern const FString GCacheDir;

// Editor & Game
#include "EditorEngine.h"
#include "GameEngine.h"

//CUR ENGINE MODE
//#define _EDITOR

#ifdef _EDITOR
extern UEditorEngine GEngine;
#endif

#ifdef _GAME
extern UGameEngine GEngine;
#endif

extern UWorld* GWorld;

#ifdef _DEBUG
#define DIRECTX_TOOL_KIT "DirectXTK_debug.lib"
#define DIRECTX_TEX "DirectXTex.lib"
#define LUA_LIB "lua.lib"
#define FBX_SDK "libfbxsdk.lib"
#else
#define DIRECTX_TOOL_KIT "DirectXTK.lib"
#define DIRECTX_TEX "DirectXTex.lib"
#define LUA_LIB "lua.lib"
#define FBX_SDK "libfbxsdk.lib"
#endif

// Library Linking
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")
#pragma comment(lib, "d2d1")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "dwrite")
#pragma comment(lib, "dwmapi")
#pragma comment(lib, DIRECTX_TOOL_KIT)
#pragma comment(lib, DIRECTX_TEX)
#pragma comment(lib, LUA_LIB)
#pragma comment(lib, FBX_SDK)

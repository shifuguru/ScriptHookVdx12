// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <windows.h>
#include <Psapi.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <d3d12.h>
#include <dxgi.h>
#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

// Global Variables
bool menuOpen = false;
bool initialized = false; // Tracks if ImGui has been initialized
ID3D12DescriptorHeap* descriptorHeap = nullptr; // Pointer to the descriptor heap
bool menuOpen = false; // Track menu state

extern void SpawnVehicle(const char* modelName); // Declare function prototype
extern void ShowNotification(const char* message); // Declare function prototype

void InitializeImGui(ID3D12Device* device, ID3D12DescriptorHeap* heap) {
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(FindWindowA(NULL, "Grand Theft Auto V"));
	ImGui_ImplDX12_Init(device, 1, DXGI_FORMAT_R8G8B8A8_UNORM, heap,
		heap->GetCPUDescriptorHandleForHeapStart(),
		heap->GetGPUDescriptorHandleForHeapStart());
}

void RenderImGui(ID3D12GraphicsCommandList* cmdList) {
	if (!cmdList) return; // Prevent crashes

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (menuOpen) {
		ImGui::Begin("Debug Console", &menuOpen);
		ImGui::Text("GTA 5 Enhanced Debug Console");

		if (ImGui::Button("Spawn Adder")) {
			SpawnVehicle("adder");
		}
		if (ImGui::Button("Spawn Zentorno")) {
			SpawnVehicle("zentorno");
		}
		if (ImGui::Button("Spawn Buffalo")) {
			SpawnVehicle("buffalo");
		}
		if (ImGui::Button("Close")) {
			menuOpen = false;
		}

		ImGui::End();
	}

	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
}

typedef HRESULT(__stdcall* PresentD3D12)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
PresentD3D12 oPresent;

HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
	static ID3D12Device* device = nullptr;
	static ID3D12GraphicsCommandList* cmdList = nullptr;


	// Ensure ImGui is initialized
	if (!initialized) {
		pSwapChain->GetDevice(__uuidof(ID3D12Device), (void**)&device);

		// Create descriptor heap
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 1;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		if (FAILED(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&descriptorHeap)))) {
			return oPresent(pSwapChain, SyncInterval, Flags); // Fail gracefully
		}

		InitializeImGui(device, descriptorHeap);
		initialized = true;
	}

	// Get command queue
	ID3D12CommandQueue* commandQueue;
	pSwapChain->GetDevice(__uuidof(ID3D12Device), (void**)&device);
	commandQueue = (ID3D12CommandQueue*)pSwapChain;

	// Get command list
	if (!cmdList) {
		commandQueue->GetDevice(__uuidof(ID3D12GraphicsCommandList), (void**)&cmdList);
	}

	// Render ImGui
	RenderImGui(cmdList);

	return oPresent(pSwapChain, SyncInterval, Flags);
}


WNDPROC oWndProc = nullptr;
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


// Hooking GTA V Window
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_KEYDOWN && wParam == VK_F4) {
		menuOpen = !menuOpen;  // Toggle the menu
	}
	return CallWindowProc(oWndProc, hWnd, msg, wParam, lParam);
}


// === Function Prototypes ===
void AttachConsoleDebug();
void LogToFile(const std::string& message);
void OpenMenu();
void ProcessInput();
void ExecuteNative(uint64_t hash, void** args);
void InitializeNativeTable();
uintptr_t FindPattern(const char* module, const char* pattern, const char* mask);

// === Finding Native Function Table ===
typedef void(*NativeHandler)(void* context);
typedef NativeHandler(*GetNativeHandler_t)(uint64_t hash);
GetNativeHandler_t GetNativeHandler = nullptr;

// ✅ Attach a Console for Debugging
void AttachConsoleDebug() {
	AllocConsole();
	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
	std::cout << "[DEBUG] Console attached.\n";
}

// ✅ Log Debugging Info to a File
void LogToFile(const std::string& message) {
	std::ofstream logFile("ScriptHookDX12V.log", std::ios::app);
	if (logFile.is_open()) {
		logFile << message << std::endl;
		logFile.close();
	}
}

// ✅ Pattern Scanner to Find GetNativeHandler
uintptr_t FindPattern(const char* module, const char* pattern, const char* mask) {
	MODULEINFO modInfo = { 0 };
	HMODULE hModule = GetModuleHandleA(module);
	if (!hModule) {
		LogToFile("[ERROR] GetModuleHandle failed for module: " + std::string(module));
		return NULL;
	}

	GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(MODULEINFO));

	uintptr_t base = (uintptr_t)modInfo.lpBaseOfDll;
	uintptr_t size = (uintptr_t)modInfo.SizeOfImage;
	size_t patternLength = strlen(mask);

	LogToFile("[DEBUG] Scanning module: " + std::string(module) + " Base: " + std::to_string(base) + " Size: " + std::to_string(size));

	for (uintptr_t i = 0; i < size - patternLength; i++) {
		bool found = true;
		for (size_t j = 0; j < patternLength; j++) {
			if (mask[j] != '?' && pattern[j] != *(char*)(base + i + j)) {
				found = false;
				break;
			}
		}
		if (found) {
			LogToFile("[DEBUG] Found pattern at address: " + std::to_string(base + i));
			return base + i;
		}
	}

	LogToFile("[ERROR] Pattern not found in module: " + std::string(module));
	return NULL;
}

// ✅ Initialize the Native Table
void InitializeNativeTable() {
	uintptr_t patternAddr = FindPattern("GTA5.exe", "\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x74", "xxx????xxxx"); // Check with memory scanner

	if (patternAddr) {
		GetNativeHandler = (GetNativeHandler_t)(patternAddr + 7);
		LogToFile("[DEBUG] GetNativeHandler found at: " + std::to_string((uintptr_t)GetNativeHandler));
	}
	else {
		LogToFile("[ERROR] Failed to find GetNativeHandler!");
	}
}

// ✅ Main Thread Entry
DWORD WINAPI MainThread(LPVOID lpParam) {
	AttachConsoleDebug();
	MessageBoxA(0, "ScriptHookDX12V Loaded!", "Success", MB_OK);
	LogToFile("[DEBUG] ScriptHookDX12V Initialized.");

	// Initialize Native Function Table
	InitializeNativeTable();

	while (true) {
		ProcessInput();  // Continuously check for key input
		Sleep(100);
	}
	return 0;
}

// ✅ Key Input Handling
void ProcessInput() {
	if (GetAsyncKeyState(VK_F4) & 1) {  // Press F4 to open the menu
		OpenMenu();
	}
}

// ✅ Debug Menu
void OpenMenu() {
	std::string choice;

	while (true) {
		std::cout << "\n==== Debug Menu ====" << std::endl;
		std::cout << "[1] Test Native Execution" << std::endl;
		std::cout << "[X] Exit" << std::endl;
		std::cout << "Select an option: ";

		std::cin.clear();
		std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
		std::getline(std::cin, choice);

		if (choice == "1") {
			LogToFile("[DEBUG] Attempting to execute native.");
			uint64_t testHash = 0xD24D37CC275948CC; // GET_HASH_KEY (Example)
			void* args[] = { (void*)"adder" };
			ExecuteNative(testHash, args);
		}
		else if (choice == "X" || choice == "x") return;
		else std::cout << "Invalid choice!\n";
	}
}

// ✅ Updated Native Function Execution
void ExecuteNative(uint64_t hash, void** args) {
	if (!GetNativeHandler) {
		LogToFile("[ERROR] GetNativeHandler is NULL!");
		return;
	}

	NativeHandler func = GetNativeHandler(hash);
	if (!func) {
		LogToFile("[ERROR] Failed to get native function for hash: " + std::to_string(hash));
		return;
	}

	try {
		func(args);
		LogToFile("[DEBUG] Successfully executed native: " + std::to_string(hash));
	}
	catch (...) {
		LogToFile("[ERROR] Exception occurred while executing native: " + std::to_string(hash));
	}
}

// ✅ DLL Entry Point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(hModule);

		// Hook Window Procedure to detect keypresses
		HWND hwnd = FindWindowA(NULL, "Grand Theft Auto V");
		oWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
	}
	return TRUE;
}

void SpawnVehicle(const char* modelName) {
	LogToFile("[DEBUG] Spawning vehicle: " + std::string(modelName));

	// Convert model name to hash
	uint64_t getHashKeyHash = 0xD24D37CC275948CC; // GET_HASH_KEY native
	uint64_t modelHash = 0;
	void* hashArgs[] = { (void*)modelName, &modelHash };
	ExecuteNative(getHashKeyHash, hashArgs);

	if (modelHash == 0) {
		LogToFile("[ERROR] Invalid model name: " + std::string(modelName));
		std::cout << "Invalid vehicle model!\n";
		return;
	}

	// Load model into memory
	uint64_t requestModelHash = 0xFA28FE3A6246FC30; // REQUEST_MODEL
	void* requestArgs[] = { &modelHash };
	ExecuteNative(requestModelHash, requestArgs);

	// Wait for model to load
	uint64_t hasModelLoadedHash = 0x98EFF6F1; // HAS_MODEL_LOADED
	while (true) {
		bool loaded = false;
		void* checkArgs[] = { &modelHash, &loaded };
		ExecuteNative(hasModelLoadedHash, checkArgs);
		if (loaded) break;
		Sleep(100);
	}

	// Get Player Position
	uint64_t playerPed = 0;
	uint64_t getPlayerPedHash = 0xD80958FC74E988A6; // PLAYER_PED_ID
	ExecuteNative(getPlayerPedHash, (void**)&playerPed);

	if (!playerPed) {
		LogToFile("[ERROR] Could not get player ped");
		return;
	}

	// ✅ Define vehicle properties
	float posX = 0, posY = 0, posZ = 0;
	uint64_t getEntityCoordsHash = 0x3FEF770D40960D5A; // GET_ENTITY_COORDS
	void* coordsArgs[] = { &playerPed, &posX, &posY, &posZ, (void*)true };
	ExecuteNative(getEntityCoordsHash, coordsArgs);

	LogToFile("[DEBUG] Spawning at position: " + std::to_string(posX) + ", " + std::to_string(posY) + ", " + std::to_string(posZ));

	// ✅ Create vehicle
	uint64_t createVehicleHash = 0xAF35D0D2583051B0; // CREATE_VEHICLE native
	float heading = 0.0f;
	bool isNetworked = false;
	void* createArgs[] = { &modelHash, &posX, &posY, &posZ, &heading, &isNetworked };
	ExecuteNative(createVehicleHash, createArgs);

	LogToFile("[DEBUG] Vehicle spawned successfully: " + std::string(modelName));
	ShowNotification((std::string(modelName) + " Spawned!").c_str());

	std::cout << modelName << " spawned!" << std::endl;
}

void ShowNotification(const char* message) {
	uint64_t notifyHash = 0x202709F4C58A0424; // Example notification native
	void* args[] = { (void*)message };
	ExecuteNative(notifyHash, args);
}

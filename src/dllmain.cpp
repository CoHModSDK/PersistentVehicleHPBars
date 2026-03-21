#include "CoHModSDK.hpp"

#include <cstddef>

#pragma comment(lib, "CoHModSDK.lib")

// VehicleDecorator::Update. This is the world-space decorator that decides
// whether the vehicle health widgets are only shown on hover/select.
constexpr const char* kVehicleDecoratorUpdatePattern =
    "55 8B EC 83 E4 F8 6A FF 68 ?? ?? ?? ?? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 "
    "83 EC 24 53 55 56 57 8B E9 8B 85 80 00 00 00 85 C0 74 41 8B 48 54 85 C9 74 3A";

constexpr const char* kModTitle = "Persistent Vehicle HP Bars";
constexpr std::ptrdiff_t kVehicleHealthWidgetOffset = 0x88;
constexpr std::size_t kWidgetSetVisibleVTableIndex = 1;

using VehicleDecoratorUpdateFn = void(__thiscall*)(void*);
using WidgetSetVisibleFn = void(__thiscall*)(void*, int);

VehicleDecoratorUpdateFn oFnVehicleDecoratorUpdate = nullptr;

void SetWidgetVisible(void* widget, bool visible) {
    if (widget == nullptr) {
        return;
    }

    auto* const vtable = *reinterpret_cast<void***>(widget);
    if ((vtable == nullptr) || (vtable[kWidgetSetVisibleVTableIndex] == nullptr)) {
        return;
    }

    auto* const setVisible =
        reinterpret_cast<WidgetSetVisibleFn>(vtable[kWidgetSetVisibleVTableIndex]);
    setVisible(widget, visible ? 1 : 0);
}

void __fastcall HookedVehicleDecoratorUpdate(void* _this, void* edx) {
    oFnVehicleDecoratorUpdate(_this);
    
    auto* const vehicleHealthWidget =
        reinterpret_cast<void*>(reinterpret_cast<std::byte*>(_this) + kVehicleHealthWidgetOffset);
    SetWidgetVisible(vehicleHealthWidget, true);
}

void SetupHook() {
    const auto vehicleDecoratorUpdate = reinterpret_cast<void*>(ModSDK::Memory::FindPattern("WW2Mod.dll", kVehicleDecoratorUpdatePattern));
    if (vehicleDecoratorUpdate == nullptr) {
        MessageBoxA(nullptr, "Failed to find VehicleDecorator::Update", kModTitle, MB_ICONERROR);
        return;
    }

    if (!ModSDK::Hooks::CreateHook(
            vehicleDecoratorUpdate,
            reinterpret_cast<void*>(&HookedVehicleDecoratorUpdate),
            reinterpret_cast<void**>(&oFnVehicleDecoratorUpdate))) {
        MessageBoxA(nullptr, "Failed to create the VehicleDecorator hook", kModTitle, MB_ICONERROR);
        return;
    }

    if (!ModSDK::Hooks::EnableHook(vehicleDecoratorUpdate)) {
        MessageBoxA(nullptr, "Failed to enable the VehicleDecorator hook", kModTitle, MB_ICONERROR);
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
    DisableThreadLibraryCalls(hModule);
    return TRUE;
}

extern "C" {
    __declspec(dllexport) void OnSDKLoad() {
        // Unused
    }

    __declspec(dllexport) void OnGameStart() {
        SetupHook();
    }

    __declspec(dllexport) void OnGameShutdown() {
        // Unused
    }

    __declspec(dllexport) const char* GetModName() {
        return kModTitle;
    }

    __declspec(dllexport) const char* GetModVersion() {
        return "1.0.0";
    }

    __declspec(dllexport) const char* GetModAuthor() {
        return "Tosox";
    }
}

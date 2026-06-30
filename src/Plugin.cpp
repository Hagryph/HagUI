#include "PCH.h"
#include "Plugin.h"
#include "SKSE_Min.h"
#include "Log.h"
#include "Hooking.h"
#include "Offsets.h"
#include "UI/HagMenu.h"

namespace hag {

Plugin& Plugin::Get() {
    static Plugin s;
    return s;
}

bool Plugin::OnLoad(const skse::Interface* skse) {
    Log::Init("HagUI");
    HAG_INFO("HagUI loading - SKSE {} runtime {:#x} base {:#x}",
             skse ? skse->skseVersion : 0u,
             skse ? skse->runtimeVersion : 0u,
             offsets::Base());

    if (!Hooking::Init()) {
        return false;
    }

    ui::HagMenu::Register();

    if (!Hooking::Commit()) {
        return false;
    }

    HAG_INFO("HagUI loaded.");
    return true;
}

}  // namespace hag

// ---- SKSE ABI exports (the only free functions; they delegate to Plugin) ----

// MUST be constant-initialized: SKSE reads this struct from the DLL's static data
// WITHOUT running our code (so an incompatible plugin can't crash the game during the
// version check). A runtime initializer (e.g. strcpy_s in a lambda) leaves it zeroed
// at probe time -> SKSE reports "bad version data". So: constinit + literals only.
extern "C" __declspec(dllexport) constinit skse::PluginVersionData SKSEPlugin_Version = {
    skse::PluginVersionData::kVersion,   // dataVersion
    1,                                    // pluginVersion
    "HagUI",                              // name
    "Hagryph",                            // author
    "",                                   // supportEmail
    0,                                    // versionIndependenceEx
    0,                                    // versionIndependence (0 => pin exact runtime below)
    { skse::kRuntime_1_6_1170 },          // compatibleVersions (rest zero-filled)
    0,                                    // seVersionRequired (0 => any)
};

extern "C" __declspec(dllexport) bool SKSEPlugin_Load(const skse::Interface* a_skse) {
    return hag::Plugin::Get().OnLoad(a_skse);
}

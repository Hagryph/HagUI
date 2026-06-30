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

extern "C" __declspec(dllexport) skse::PluginVersionData SKSEPlugin_Version = [] {
    skse::PluginVersionData v{};
    v.dataVersion   = skse::PluginVersionData::kVersion;
    v.pluginVersion = 1;
    ::strcpy_s(v.name, "HagUI");
    ::strcpy_s(v.author, "Hagryph");
    v.versionIndependence  = 0;  // pinned to the exact runtime(s) listed below
    v.compatibleVersions[0] = skse::kRuntime_1_6_1170;
    v.compatibleVersions[1] = 0;
    v.seVersionRequired     = 0;
    return v;
}();

extern "C" __declspec(dllexport) bool SKSEPlugin_Load(const skse::Interface* a_skse) {
    return hag::Plugin::Get().OnLoad(a_skse);
}

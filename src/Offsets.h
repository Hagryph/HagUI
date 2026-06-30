#pragma once
#include "PCH.h"

// Hand-found addresses (no Address Library). Recovered from a Steamless-decrypted
// SkyrimSE.exe 1.6.1170 in Ghidra (raw retail exe is SteamStub-encrypted). RVAs are
// offsets from PE image base 0x140000000; convert to runtime with FromRVA().
namespace hag::offsets {

inline constexpr std::uintptr_t kImageBase = 0x140000000ull;

inline std::uintptr_t Base() {
    return reinterpret_cast<std::uintptr_t>(::GetModuleHandleW(nullptr));
}
inline std::uintptr_t FromFileAddr(std::uintptr_t fileAddr) { return Base() + (fileAddr - kImageBase); }
inline std::uintptr_t FromRVA(std::uintptr_t rva) { return Base() + rva; }

// ---- Confirmed via Ghidra decompilation (decrypted 1.6.1170) ----

// UI singleton ctor: interns every menu-name BSFixedString into the UI object and
// stores the instance pointer to the global below. (func 0x140FB7E10)
inline constexpr std::uintptr_t kUI_ctor            = 0xFB7E10;
// Global holding the UI* singleton (set as `global = this` at top of the ctor).
inline constexpr std::uintptr_t kUI_singleton_ptr   = 0x20F8958;   // *(UI**)(base+this)
// BSFixedString::ctor(BSFixedString* dst, const char* str)  (func 0x140CEC5D0)
inline constexpr std::uintptr_t kBSFixedString_ctor = 0xCEC5D0;

// BSScaleformManager::LoadMovie(this, IMenu* menu, GFxMovieView** out, const char* name, flags)
// builds "Interface/<name>.swf" (fallback "Interface/Exported/<name>.gfx"),
// loads the file, creates the GFxMovieView, runs _root.InitExtensions. (func 0x140FB0110)
inline constexpr std::uintptr_t kScaleform_LoadMovie     = 0xFB0110;
inline constexpr std::uintptr_t kScaleform_LoadMovie_alt = 0xFB0980;
// GFxMovieView Invoke-by-path: Invoke(view, "_root.path", args...)  (func 0x140FBFB10)
inline constexpr std::uintptr_t kGFxMovie_InvokePath    = 0xFBFB10;
// Movie file loader -> returns movie def/loader.                    (func 0x141034E30)
inline constexpr std::uintptr_t kGFx_LoadFile           = 0x1034E30;
// BSScaleformFileOpener::Open (BSResource/BSA-backed; the BSA chain).(func 0x140FB4490)
inline constexpr std::uintptr_t kScaleform_FileOpener   = 0xFB4490;
// GFxLoader::Read (SWF/GFX parse).                                   (func 0x1410323F0)
inline constexpr std::uintptr_t kGFxLoader_Read         = 0x10323F0;
// sprintf-like: format into a BSString (dst, fmt, ...).             (func 0x1401C3CD0)
inline constexpr std::uintptr_t kFormatString           = 0x1C3CD0;

// GFxMovieView vtable SLOT offsets (call `(*(*view+slot))(view, ...)`), seen in LoadMovie:
//   +0x50  Invoke/IsAvailable("_root.x")   +0x88  GetVariable(out,"name")
//   +0xC0  (loader) CreateInstance->view   +0xC8  set transform/cxform
//   +0xD8  SetViewScaleMode(flags)          +0xF8  GetVisibleFrameRect(out)
//   +0x118 SetViewport / SetSafeRect        +0x128 Display/Advance
//   +0x158 Restart/SetState
namespace gfxview {
    inline constexpr std::uintptr_t kInvoke        = 0x50;
    inline constexpr std::uintptr_t kGetVariable   = 0x88;
    inline constexpr std::uintptr_t kCreateView    = 0xC0;
    inline constexpr std::uintptr_t kSetScaleMode  = 0xD8;
    inline constexpr std::uintptr_t kGetVisRect    = 0xF8;
    inline constexpr std::uintptr_t kSetViewport   = 0x118;
    inline constexpr std::uintptr_t kDisplay       = 0x128;
}

// UI singleton member offsets: interned menu-name BSFixedStrings (from the ctor)
namespace uimenu_names {
    inline constexpr std::uintptr_t kInventoryMenu = 0x88;
    inline constexpr std::uintptr_t kJournalMenu   = 0x148;
    inline constexpr std::uintptr_t kMapMenu       = 0x110;
    // ... full table in docs/UI-RE.md
}

// ---- Menu registration / creation ABI (confirmed) ----
// Each menu has a tiny static registrar; BarterMenu's (0x1400B2A20) is:
//   reg = UI_RegistryGet(&param);                       // 0x140FA32F0 -> caches global 0x1420F6A00
//   UI_Register(reg, "BarterMenu", &BarterMenu_Create); // 0x140FA5480
inline constexpr std::uintptr_t kUI_Register     = 0xFA5480;  // (registry, const char* name, IMenu*(*creator)())
inline constexpr std::uintptr_t kUI_RegistryGet  = 0xFA32F0;  // menu-registry singleton getter
inline constexpr std::uintptr_t kUI_Registry_ptr = 0x20F6A00; // cached registry singleton global

// Menu creator body (template: BarterMenu_Create 0x1408EF2B0):
//   m = (*(*alloc+0x50))(alloc, size, 0);   // alloc = kMenuAllocator_ptr
//   *m = &IMenu_vtable; ...init...;
//   LoadMovie(scaleformMgr, m, &m[0x10], "MovieName", scaleMode, 0);
//   *(uint32*)(m+0x20) = flags;
inline constexpr std::uintptr_t kMenuAllocator_ptr       = 0x3292490;  // memory mgr; Allocate at vtable +0x50
inline constexpr std::uintptr_t kBSScaleformManager_ptr  = 0x35F11C8;  // *(BSScaleformManager**)
inline constexpr std::uintptr_t kIMenu_vtable_BarterMenu = 0x18F0CE0;  // 9-slot IMenu vtable template
inline constexpr std::uintptr_t kBarterMenu_Create       = 0x8EF2B0;   // example creator

// Menu object layout (from the creator): +0x00 vtable, +0x10 GFxMovieView*, +0x20 flags(u32)
namespace menu_layout {
    inline constexpr std::uintptr_t kVtable    = 0x00;
    inline constexpr std::uintptr_t kMovieView = 0x10;
    inline constexpr std::uintptr_t kFlags     = 0x20;
}

// ---- Still TODO (next RE pass) ----
inline constexpr std::uintptr_t kUIMessageQueue_AddMsg = 0x0;  // open/close menu by name (kShow/kHide)
// + the Options/System-menu hook point to inject the "HagUI" entry (trampoline + call original)

}  // namespace hag::offsets

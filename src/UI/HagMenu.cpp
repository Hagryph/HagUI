#include "PCH.h"
#include "UI/HagMenu.h"
#include "Log.h"
#include "Offsets.h"

// HagUIMenu wiring. Modelled byte-for-byte on the RE'd BarterMenu creator
// (docs/UI-RE.md): allocate -> set a 9-slot IMenu vtable -> LoadMovie -> flags.
// Create() and the vtable are NOT exercised until the menu is shown; Register()
// is the only part run so far (safe — it just inserts into the menu map).
namespace hag::ui {

namespace {
    using AllocFn     = void* (*)(void* self, std::size_t size, std::size_t align);
    using FreeFn      = void  (*)(void* self, void* ptr);
    using LoadMovieFn = bool  (*)(void* sfMgr, void* menu, void* viewOut, const char* name, int scaleMode, int unk);
    using RegisterFn  = void  (*)(void* registry, const char* name, void* creator);

    void* Allocator()  { return *reinterpret_cast<void**>(offsets::FromRVA(offsets::kMenuAllocator_ptr)); }

    // --- our IMenu vtable: overrides + reused generic game bases (see imenu_vtable) ---
    void* HagDtor(void* self, unsigned int flags);
    void  HagRegisterFuncs(void* self, void* movieView);
    unsigned int HagProcessMessage(void* self, void* msg);

    void** Vtable() {
        static void* vt[9] = {};
        if (!vt[0]) {
            vt[0] = reinterpret_cast<void*>(&HagDtor);                       // ~Menu
            vt[1] = reinterpret_cast<void*>(&HagRegisterFuncs);              // RegisterFuncs
            vt[2] = reinterpret_cast<void*>(offsets::FromRVA(offsets::kIMenuBase_2));
            vt[3] = reinterpret_cast<void*>(offsets::FromRVA(offsets::kIMenuBase_3));
            vt[4] = reinterpret_cast<void*>(&HagProcessMessage);            // ProcessMessage
            vt[5] = reinterpret_cast<void*>(offsets::FromRVA(offsets::kIMenuBase_5));
            vt[6] = reinterpret_cast<void*>(offsets::FromRVA(offsets::kIMenuBase_6)); // generic tick
            vt[7] = reinterpret_cast<void*>(offsets::FromRVA(offsets::kIMenuBase_7));
            vt[8] = reinterpret_cast<void*>(offsets::FromRVA(offsets::kIMenuBase_8));
        }
        return vt;
    }

    void* HagDtor(void* self, unsigned int flags) {
        HAG_INFO("HagUIMenu::~ flags={:#x}", flags);
        if (flags & 1) {
            void* a = Allocator();
            auto** avt = *reinterpret_cast<void***>(a);
            reinterpret_cast<FreeFn>(avt[0x60 / 8])(a, self);
        }
        return self;
    }

    void HagRegisterFuncs(void* /*self*/, void* /*movieView*/) {
        HAG_INFO("HagUIMenu::RegisterFuncs");   // TODO: register a 'Close' AS callback
    }

    // printf-style GFx invoke (confirmed): Invoke(view, "_root.method", "%d ...", ints...)
    using InvokeF = void (*)(void* view, const char* method, const char* fmt, ...);

    void DrawWelcome(void* view) {
        if (!view) { HAG_WARN("DrawWelcome: no view"); return; }
        const auto inv = reinterpret_cast<InvokeF>(offsets::FromRVA(offsets::kGFxMovie_Invoke));
        // near-black panel, centered (stage is 1280x720)
        inv(view, "_root.clear", "");
        inv(view, "_root.beginFill", "%d %d", 0x121013, 100);
        inv(view, "_root.moveTo", "%d %d", 340, 240);
        inv(view, "_root.lineTo", "%d %d", 940, 240);
        inv(view, "_root.lineTo", "%d %d", 940, 480);
        inv(view, "_root.lineTo", "%d %d", 340, 480);
        inv(view, "_root.lineTo", "%d %d", 340, 240);
        inv(view, "_root.endFill", "");
        // gold border
        inv(view, "_root.lineStyle", "%d %d %d", 3, 0xE0B34A, 100);
        inv(view, "_root.moveTo", "%d %d", 340, 240);
        inv(view, "_root.lineTo", "%d %d", 940, 240);
        inv(view, "_root.lineTo", "%d %d", 940, 480);
        inv(view, "_root.lineTo", "%d %d", 340, 480);
        inv(view, "_root.lineTo", "%d %d", 340, 240);
        HAG_INFO("DrawWelcome: drew golden panel via GFx");
    }

    unsigned int HagProcessMessage(void* self, void* msg) {
        const unsigned int type = *reinterpret_cast<unsigned int*>(reinterpret_cast<char*>(msg) + 8);
        HAG_INFO("HagUIMenu::ProcessMessage type={}", type);
        if (type == offsets::kMsg_Show) {
            void* view = *reinterpret_cast<void**>(reinterpret_cast<char*>(self) + offsets::menu_layout::kMovieView);
            DrawWelcome(view);
        }
        return 0;
    }
}  // namespace

void* HagMenu::Create() {
    HAG_INFO("HagUIMenu::Create");
    void* a = Allocator();
    if (!a) { HAG_ERR("HagUIMenu::Create - no allocator"); return nullptr; }
    auto** avt = *reinterpret_cast<void***>(a);
    void* menu = reinterpret_cast<AllocFn>(avt[0x50 / 8])(a, 0x40, 0);
    if (!menu) { HAG_ERR("HagUIMenu::Create - alloc failed"); return nullptr; }
    std::memset(menu, 0, 0x40);
    *reinterpret_cast<void***>(menu) = Vtable();                                   // +0x00 vtable

    void* sfMgr = *reinterpret_cast<void**>(offsets::FromRVA(offsets::kBSScaleformManager_ptr));
    auto  loadMovie = reinterpret_cast<LoadMovieFn>(offsets::FromRVA(offsets::kScaleform_LoadMovie));
    void* viewSlot = reinterpret_cast<char*>(menu) + offsets::menu_layout::kMovieView;  // +0x10
    const bool ok = loadMovie(sfMgr, menu, viewSlot, "HagUI", 1, 0);
    *reinterpret_cast<std::uint32_t*>(reinterpret_cast<char*>(menu) + offsets::menu_layout::kFlags) = 4;
    HAG_INFO("HagUIMenu::Create - LoadMovie('HagUI')={} menu={}", ok, menu);
    return menu;
}

void HagMenu::Open() {
    void* queue = *reinterpret_cast<void**>(offsets::FromRVA(offsets::kUIMessageQueue_ptr));
    if (!queue) { HAG_WARN("HagUIMenu::Open - no UIMessageQueue"); return; }

    struct BSFixedString { const char* data = nullptr; } name;  // 8-byte interned-string handle
    reinterpret_cast<void (*)(void*, const char*)>(offsets::FromRVA(offsets::kBSFixedString_ctor))(&name, kName);

    using AddMsgFn = void (*)(void* queue, void* name, std::uint32_t type, void* data);
    reinterpret_cast<AddMsgFn>(offsets::FromRVA(offsets::kUIMessageQueue_AddMsg))(
        queue, &name, offsets::kMsg_Show, nullptr);

    reinterpret_cast<void (*)(void*)>(offsets::FromRVA(offsets::kBSFixedString_dtor))(&name);
    HAG_INFO("HagUIMenu::Open - posted kShow");
}

void HagMenu::Register() {
    void* registry = *reinterpret_cast<void**>(offsets::FromRVA(offsets::kUI_Registry_ptr));
    if (!registry) { HAG_WARN("HagUIMenu::Register - UI registry null (too early?)"); return; }
    reinterpret_cast<RegisterFn>(offsets::FromRVA(offsets::kUI_Register))(
        registry, kName, reinterpret_cast<void*>(&HagMenu::Create));
    HAG_INFO("HagUIMenu registered via UI::Register (registry={})", registry);
}

}  // namespace hag::ui

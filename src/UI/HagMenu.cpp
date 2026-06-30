#include "PCH.h"
#include "UI/HagMenu.h"
#include "Log.h"
#include "Offsets.h"
#include "Hooking.h"

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
        const int x0 = 320, y0 = 200, x1 = 960, y1 = 520;  // panel (stage 1280x720)
        inv(view, "_root.clear", "");
        // near-black panel
        inv(view, "_root.beginFill", "%d %d", 0x0A0A0C, 92);
        inv(view, "_root.moveTo", "%d %d", x0, y0);
        inv(view, "_root.lineTo", "%d %d", x1, y0);
        inv(view, "_root.lineTo", "%d %d", x1, y1);
        inv(view, "_root.lineTo", "%d %d", x0, y1);
        inv(view, "_root.lineTo", "%d %d", x0, y0);
        inv(view, "_root.endFill", "");
        // gold header bar
        inv(view, "_root.beginFill", "%d %d", 0xE0B34A, 100);
        inv(view, "_root.moveTo", "%d %d", x0, y0);
        inv(view, "_root.lineTo", "%d %d", x1, y0);
        inv(view, "_root.lineTo", "%d %d", x1, y0 + 48);
        inv(view, "_root.lineTo", "%d %d", x0, y0 + 48);
        inv(view, "_root.lineTo", "%d %d", x0, y0);
        inv(view, "_root.endFill", "");
        // gold border
        inv(view, "_root.lineStyle", "%d %d %d", 2, 0xE0B34A, 100);
        inv(view, "_root.moveTo", "%d %d", x0, y0);
        inv(view, "_root.lineTo", "%d %d", x1, y0);
        inv(view, "_root.lineTo", "%d %d", x1, y1);
        inv(view, "_root.lineTo", "%d %d", x0, y1);
        inv(view, "_root.lineTo", "%d %d", x0, y0);
        HAG_INFO("DrawWelcome: drew golden/black welcome panel");
    }

    unsigned int HagProcessMessage(void* self, void* msg) {
        const unsigned int type = *reinterpret_cast<unsigned int*>(reinterpret_cast<char*>(msg) + 8);
        if (type == offsets::kMsg_Show) {
            HAG_INFO("HagUIMenu::ProcessMessage kShow -> draw");
            void* view = *reinterpret_cast<void**>(reinterpret_cast<char*>(self) + offsets::menu_layout::kMovieView);
            DrawWelcome(view);
        } else if (type != 6 && type != 7) {  // 6/7 fire every frame (input/update) - skip the spam
            HAG_INFO("HagUIMenu::ProcessMessage type={}", type);
        }
        return 0;
    }

    // --- open-trigger: a real "HagUI" main-menu button (above Credits) ---
    // Our modified StartMenu.swf adds a list entry whose click calls
    //   gfx.io.GameDelegate.call("OpenHagUI", []).
    // MainMenu::RegisterFuncs registers every such native callback as
    //   movie->vtable[1](movie, &gfxName, handler).
    // We trampoline it: run the original (registers all the real callbacks), then
    // register one more, "OpenHagUI" -> Hag_OpenHandler, the exact same way.
    using RegFuncsFn = void (*)(void* self, void* movie);
    using MakeStrFn  = void (*)(void* gfxStringOut, const char* str);
    using RegCBFn    = void (*)(void* movie, void* gfxName, void* handler);  // movie->vtable[1]
    RegFuncsFn g_origRegisterFuncs = nullptr;

    // GFx FunctionHandler ABI: called with a Params* in rcx, returns void. We ignore the
    // args (no params needed to open) — the same shape that worked when we hooked the
    // OpenCreditsMenu handler directly.
    void Hag_OpenHandler(void* /*params*/) {
        HAG_INFO("OpenHagUI callback -> opening HagUIMenu");
        HagMenu::Open();
    }

    void Detour_RegisterFuncs(void* self, void* movie) {
        g_origRegisterFuncs(self, movie);                 // register the real main-menu callbacks first
        if (!movie) { HAG_WARN("RegisterFuncs: null movie - cannot add OpenHagUI"); return; }

        // Mirror the game's per-callback sequence (ui_mainmenu.txt:466-474):
        //   FUN_140fbb150(&name,"OpenHagUI"); movie->vtable[1](movie,&name,handler); release(name);
        std::uint64_t name = 0;
        reinterpret_cast<MakeStrFn>(offsets::FromRVA(offsets::kGFx_MakeString))(&name, "OpenHagUI");
        void** mvt = *reinterpret_cast<void***>(movie);
        reinterpret_cast<RegCBFn>(mvt[offsets::kGFxMovie_RegisterCB])(
            movie, &name, reinterpret_cast<void*>(&Hag_OpenHandler));
        // drop our reference to the managed GFx string (atomic refcount at (ptr&~3)+8)
        if (name) {
            auto* rc = reinterpret_cast<volatile long*>((name & ~3ull) + 8);
            if (_InterlockedDecrement(rc) == 0) {
                void* a = Allocator();
                reinterpret_cast<FreeFn>((*reinterpret_cast<void***>(a))[0x60 / 8])(
                    a, reinterpret_cast<void*>(name & ~3ull));
            }
        }
        HAG_INFO("HagUIMenu: registered 'OpenHagUI' GameDelegate callback (movie={})", movie);
    }

    // --- BSInputEventUser input handler (+0x30): official close on Cancel, mirroring Credits ---
    void* HagInputDtor(void* self, unsigned int) { return self; }
    void HagInputHandler(void* /*self*/, void* event) {
        const int t = *reinterpret_cast<int*>(reinterpret_cast<char*>(event) + 0x30);
        HAG_INFO("HagUIMenu input event type={}", t);
        if (t == 2) HagMenu::Close();  // 2 == Cancel/back (same as Credits' handler)
    }
    void** InputVtable() {
        static void* vt[2] = {};
        if (!vt[0]) {
            vt[0] = reinterpret_cast<void*>(&HagInputDtor);
            vt[1] = reinterpret_cast<void*>(&HagInputHandler);
        }
        return vt;
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
    // IMenu/BSInputEventUser base ctor FIRST (sets up + registers the input sink) - every real creator does this.
    reinterpret_cast<void (*)(void*)>(offsets::FromRVA(offsets::kIMenu_baseCtor))(menu);
    *reinterpret_cast<void***>(menu) = Vtable();                                   // override +0x00 IMenu vtable
    *reinterpret_cast<void***>(reinterpret_cast<char*>(menu) + 0x30) = InputVtable();  // override +0x30 BSInputEventUser
    *reinterpret_cast<std::uint32_t*>(reinterpret_cast<char*>(menu) + 0x38) = 1;       // (Credits sets this)

    void* sfMgr = *reinterpret_cast<void**>(offsets::FromRVA(offsets::kBSScaleformManager_ptr));
    auto  loadMovie = reinterpret_cast<LoadMovieFn>(offsets::FromRVA(offsets::kScaleform_LoadMovie));
    void* viewSlot = reinterpret_cast<char*>(menu) + offsets::menu_layout::kMovieView;  // +0x10
    const bool ok = loadMovie(sfMgr, menu, viewSlot, "HagUI", 1, 0);
    // Match the Credits Menu (renders over the Main Menu): depth 10 (> MainMenu's 9), flags 1.
    *reinterpret_cast<std::uint8_t*>(reinterpret_cast<char*>(menu) + 0x18)  = 10;  // depth/priority
    *reinterpret_cast<std::uint32_t*>(reinterpret_cast<char*>(menu) + 0x1c) = 1;
    *reinterpret_cast<std::uint32_t*>(reinterpret_cast<char*>(menu) + offsets::menu_layout::kFlags) = 1;
    HAG_INFO("HagUIMenu::Create - LoadMovie('HagUI')={} menu={} depth=10", ok, menu);
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

void HagMenu::Close() {
    void* queue = *reinterpret_cast<void**>(offsets::FromRVA(offsets::kUIMessageQueue_ptr));
    if (!queue) return;
    struct BSFixedString { const char* data = nullptr; } name;
    reinterpret_cast<void (*)(void*, const char*)>(offsets::FromRVA(offsets::kBSFixedString_ctor))(&name, kName);
    reinterpret_cast<void (*)(void*, void*, std::uint32_t, void*)>(offsets::FromRVA(offsets::kUIMessageQueue_AddMsg))(
        queue, &name, offsets::kMsg_Hide, nullptr);
    reinterpret_cast<void (*)(void*)>(offsets::FromRVA(offsets::kBSFixedString_dtor))(&name);
    HAG_INFO("HagUIMenu::Close - posted kHide");
}

void HagMenu::Register() {
    void* registry = *reinterpret_cast<void**>(offsets::FromRVA(offsets::kUI_Registry_ptr));
    if (!registry) { HAG_WARN("HagUIMenu::Register - UI registry null (too early?)"); return; }
    reinterpret_cast<RegisterFn>(offsets::FromRVA(offsets::kUI_Register))(
        registry, kName, reinterpret_cast<void*>(&HagMenu::Create));
    HAG_INFO("HagUIMenu registered via UI::Register (registry={})", registry);
}

void HagMenu::InstallTrigger() {
    // Hook MainMenu::RegisterFuncs so we can add our "OpenHagUI" GameDelegate callback
    // alongside the game's own. Credits is left untouched and works normally.
    const auto target = offsets::FromRVA(offsets::kMainMenu_RegisterFuncs);
    if (Hooking::Create<RegFuncsFn>(target, &Detour_RegisterFuncs, g_origRegisterFuncs)) {
        HAG_INFO("HagUIMenu: hooked MainMenu::RegisterFuncs @{:#x} (adds OpenHagUI)", target);
    } else {
        HAG_ERR("HagUIMenu: failed to hook MainMenu::RegisterFuncs");
    }
}

}  // namespace hag::ui

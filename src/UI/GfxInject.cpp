#include "PCH.h"
#include "UI/HagMenu.h"
#include "Log.h"
#include "Offsets.h"
#include "Hooking.h"

#include <cstdio>
#include <cstring>

// Runtime, SkyUI-compatible injection of a "HagUI" row into the in-game System (pause) menu.
//
// No SWF file is shipped or edited: we drive the LIVE quest_journal movie through the game's own
// Scaleform GFxMovieView API and hang a native GFxFunctionHandler off the list's itemPress event --
// exactly how the game wires its own entries. Because nothing on disk is replaced, this coexists
// with SkyUI and any other UI overhaul (they can ship their own quest_journal.swf; we inject into
// whatever movie loaded). Every offset/ABI here was validated live via HagIPC (docs/UI-RE.md §10a).
namespace hag::ui {
namespace {

using namespace hag::offsets;

// Scaleform GFxValue (0x18). MUST be zeroed before Create*/GetVariable: those release the prior
// value first, so uninitialised bytes make them free garbage (observed live).
struct GFxValue { void* objIface; std::uint32_t type; std::uint32_t typeHi; std::uint64_t value; };
static_assert(sizeof(GFxValue) == 0x18, "GFxValue must be 0x18");

// GFxFunctionHandler::Params (recovered from FUN_140fac280) -- we don't read from it.
struct FnParams { GFxValue* ret; void* movie; GFxValue* self; GFxValue* argsThis; GFxValue* args; int argc; int pad; void* userData; };

// --- GFxMovieView method thunks: (*(*(void***)movie))[slot/8](movie, ...) ---
inline void* Slot(void* m, std::uintptr_t off) { return (*reinterpret_cast<void***>(m))[off / 8]; }
inline bool MIsAvail (void* m, const char* p)               { return reinterpret_cast<char(*)(void*, const char*)>(Slot(m, kMovie_IsAvailable))(m, p) != 0; }
inline int  MArrSize (void* m, const char* p)               { return reinterpret_cast<int (*)(void*, const char*)>(Slot(m, kMovie_GetVariableArraySize))(m, p); }
inline void MCreateStr(void* m, GFxValue* o, const char* s) { *o = {}; reinterpret_cast<void(*)(void*, GFxValue*, const char*)>(Slot(m, kMovie_CreateString))(m, o, s); }
inline void MCreateObj(void* m, GFxValue* o)                { *o = {}; reinterpret_cast<void(*)(void*, GFxValue*, const char*, void*, unsigned)>(Slot(m, kMovie_CreateObject))(m, o, nullptr, nullptr, 0); }
inline void MCreateFn (void* m, GFxValue* o, void* h)       { *o = {}; reinterpret_cast<void(*)(void*, GFxValue*, void*, void*)>(Slot(m, kMovie_CreateFunction))(m, o, h, nullptr); }
inline void MGetVar  (void* m, GFxValue* o, const char* p)  { *o = {}; reinterpret_cast<void(*)(void*, GFxValue*, const char*)>(Slot(m, kMovie_GetVariable))(m, o, p); }
inline char MSetVar  (void* m, const char* p, GFxValue* v)  { return reinterpret_cast<char(*)(void*, const char*, GFxValue*, int)>(Slot(m, kMovie_SetVariable))(m, p, v, 0); }
inline char MSetNum  (void* m, const char* p, double d)     { GFxValue v{}; v.type = 3; std::memcpy(&v.value, &d, sizeof d); return MSetVar(m, p, &v); }  // 3 = VT_Number
inline void MInvoke  (void* m, const char* p)               { reinterpret_cast<char(*)(void*, const char*, void*)>(FromRVA(kGFxMovie_Invoke))(m, p, nullptr); }

constexpr int kInsertAt = 3;   // list slot for our row: directly below "Load" (0=QuickSave,1=Save,2=Load)

// AS path to the System page's category list (verified from the SWF placement tree, docs/UI-RE.md §10).
constexpr const char* kList = "_root.QuestJournalFader.Menu_mc.SystemFader.Page_mc.CategoryList_mc.List_mc";

// --- native click handler: a GFxFunctionHandler whose Call is vtable slot 1 (FUN_140fac280) ---
struct HagHandler { void** vtbl; std::int32_t refCount; std::int32_t pad; };
void* HandlerDtor(HagHandler* self, unsigned) { return self; }   // never runs: refCount pinned high
void  HandlerCall(HagHandler* self, FnParams* params);
void** HandlerVtbl() {
    static void* vt[8] = {};
    if (!vt[1]) {
        for (auto& s : vt) s = reinterpret_cast<void*>(&HandlerDtor);
        vt[1] = reinterpret_cast<void*>(&HandlerCall);           // Call slot
    }
    return vt;
}
HagHandler g_handler{ nullptr, 0x40000000, 0 };
void* g_movie = nullptr;   // live journal GFxMovieView (refreshed every advance)
int   g_index = -1;        // list index of our injected row

// Runs FIRST on every itemPress (we insert ahead of the game's onCategoryButtonPress). Because our
// row sits at kInsertAt, every row below it is shifted one slot; the game dispatches by positional
// index, so we rewrite the *shared* event object's .index (objects are by-reference, so the game's
// handler then sees the pre-shift slot) and open HagUI for our own row. This reproduces what the old
// SWF edit did with `_loc3_ - 1`, but entirely at runtime.
void HandlerCall(HagHandler*, FnParams* params) {
    __try {
        void* m = g_movie;
        if (!m || !params || params->argc < 1 || !params->args) return;
        // Bind the event object to a temp path so we can read/rewrite .index by reference.
        char evt[256], idxp[256];
        std::snprintf(evt, sizeof evt, "%s.__hagEvt", kList);
        MSetVar(m, evt, params->args);                    // params->args[0] == the itemPress event
        std::snprintf(idxp, sizeof idxp, "%s.index", evt);
        GFxValue v{}; MGetVar(m, &v, idxp);
        double d = 0; std::memcpy(&d, &v.value, sizeof d);
        const int i = static_cast<int>(d);
        if (i == g_index) {
            HAG_INFO("HagUI System-menu row clicked -> opening HagUI");
            HagMenu::Open();
            MSetNum(m, idxp, 1.0e6);                       // out of range -> game's switch hits default (no-op)
        } else if (i > g_index) {
            MSetNum(m, idxp, static_cast<double>(i - 1));  // undo our insert's shift for rows below us
        }
        // i < g_index (QuickSave/Save/Load): untouched
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
}

// Append our row + attach the native listener, once per journal-movie session (a fresh journal open
// gives a fresh List_mc, so the __hagInjected marker is naturally per-session).
void InjectIfNeeded(void* movie) {
    char p[256], flag[256];
    std::snprintf(p, sizeof p, "%s.entryList", kList);
    if (!MIsAvail(movie, p)) return;                 // System page not built yet (other tab / not open)
    // Wait until the game has registered its OWN itemPress listener, i.e. _listeners.itemPress
    // exists as an array. Injecting the instant entryList appears (a frame earlier) means our
    // listener push lands on a not-yet-created array and is silently lost.
    std::snprintf(p, sizeof p, "%s._listeners.itemPress", kList);
    if (!MIsAvail(movie, p)) return;
    std::snprintf(flag, sizeof flag, "%s.__hagInjected", kList);
    if (MIsAvail(movie, flag)) return;               // already injected into this list

    std::snprintf(p, sizeof p, "%s.entryList", kList);
    int n = MArrSize(movie, p);
    if (n <= 0) return;
    const int at = (n > kInsertAt) ? kInsertAt : n;   // directly below Load (guard: append if list is tiny)

    // 1) splice {text:"HagUI"} in at 'at': shift entryList[at..n-1] up by one, then set entryList[at]
    for (int i = n; i > at; --i) {
        GFxValue tmp{};
        std::snprintf(p, sizeof p, "%s.entryList.%d", kList, i - 1); MGetVar(movie, &tmp, p);
        std::snprintf(p, sizeof p, "%s.entryList.%d", kList, i);     MSetVar(movie, p, &tmp);
    }
    GFxValue obj{}; MCreateObj(movie, &obj);
    std::snprintf(p, sizeof p, "%s.entryList.%d", kList, at);        MSetVar(movie, p, &obj);
    GFxValue str{}; MCreateStr(movie, &str, "HagUI");
    std::snprintf(p, sizeof p, "%s.entryList.%d.text", kList, at);   MSetVar(movie, p, &str);
    g_index = at;

    // 2) native function -> List_mc.__hagPress
    if (!g_handler.vtbl) g_handler.vtbl = HandlerVtbl();
    GFxValue fn{}; MCreateFn(movie, &fn, &g_handler);
    std::snprintf(p, sizeof p, "%s.__hagPress", kList);             MSetVar(movie, p, &fn);

    // 3) insert our itemPress listener at index 0 (must run BEFORE onCategoryButtonPress so our
    //    index-rewrite is visible to it): shift existing listeners up, then set [0].
    std::snprintf(p, sizeof p, "%s._listeners.itemPress", kList);
    int ln = MArrSize(movie, p);
    for (int i = ln; i > 0; --i) {
        GFxValue tmp{};
        std::snprintf(p, sizeof p, "%s._listeners.itemPress.%d", kList, i - 1); MGetVar(movie, &tmp, p);
        std::snprintf(p, sizeof p, "%s._listeners.itemPress.%d", kList, i);     MSetVar(movie, p, &tmp);
    }
    GFxValue rec{}; MCreateObj(movie, &rec);
    std::snprintf(p, sizeof p, "%s._listeners.itemPress.0", kList);                  MSetVar(movie, p, &rec);
    GFxValue listMc{}; MGetVar(movie, &listMc, kList);
    std::snprintf(p, sizeof p, "%s._listeners.itemPress.0.listenerObject", kList);   MSetVar(movie, p, &listMc);
    GFxValue fname{}; MCreateStr(movie, &fname, "__hagPress");
    std::snprintf(p, sizeof p, "%s._listeners.itemPress.0.listenerFunction", kList); MSetVar(movie, p, &fname);

    // 4) mark done + rebuild the visible list
    GFxValue mark{}; MCreateStr(movie, &mark, "1");                  MSetVar(movie, flag, &mark);
    std::snprintf(p, sizeof p, "%s.InvalidateData", kList);          MInvoke(movie, p);
    HAG_INFO("HagUI: injected System-menu row at index {} + native click listener (SkyUI-safe)", n);
}

using AdvanceFn = void (*)(void*);
AdvanceFn g_origAdvance = nullptr;
void Detour_JournalAdvance(void* self) {
    g_origAdvance(self);                                            // let the journal advance/render first
    __try {
        void* movie = *reinterpret_cast<void**>(reinterpret_cast<char*>(self) + offsets::menu_layout::kMovieView);
        if (movie) { g_movie = movie; InjectIfNeeded(movie); }
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
}

// --- grey-out fix ---------------------------------------------------------------------------------
// The System page greys rows by calling GameDelegate.call("SetSaveDisabled",[entryList[0..],BOOL]) with
// hard-coded indices; the native handler (SetSaveDisabled, 0x98edb0) computes each slot's disabled
// state from game save-state and writes .disabled onto the PASSED entry objects. Our insert at
// kInsertAt shifts every entry below it, so the AS hands the handler the wrong objects. We hook the
// handler and, before it runs, rewrite each entry arg whose live index is >= our row to the entry one
// slot lower (the one the AS actually meant, which our insert pushed down). The handler's per-slot
// state computation is left completely intact; we only fix which object sits in each arg slot.
// The 6 entry args live at *(params+0x28) as consecutive 0x18 GFxValues; arg[6] is the bool.
using SsdFn = void (*)(void* params);
SsdFn g_origSsd = nullptr;
void Detour_SetSaveDisabled(void* params) {
    __try {
        void* m = g_movie;
        if (m && g_index >= 0 && params) {
            char* args = *reinterpret_cast<char**>(reinterpret_cast<char*>(params) + 0x28);
            if (args) {
                char lp[288], p[288];
                std::snprintf(lp, sizeof lp, "%s.entryList", kList);
                const int n = MArrSize(m, lp);
                for (int a = 0; a < 6; ++a) {                       // 6 entry objects (arg[6] = bool)
                    GFxValue* arg = reinterpret_cast<GFxValue*>(args + a * 0x18);
                    if (!arg->value) continue;
                    int idx = -1;                                  // find this entry's live index
                    for (int k = g_index; k < n; ++k) {            // <g_index rows never shift -> skip
                        GFxValue e{}; std::snprintf(p, sizeof p, "%s.entryList.%d", kList, k); MGetVar(m, &e, p);
                        if (e.value == arg->value) { idx = k; break; }
                    }
                    if (idx >= g_index && idx + 1 < n) {           // shift to the entry the AS meant
                        GFxValue corrected{}; std::snprintf(p, sizeof p, "%s.entryList.%d", kList, idx + 1); MGetVar(m, &corrected, p);
                        if (corrected.value) { std::memcpy(arg, &corrected, sizeof(GFxValue));
                            HAG_INFO("SetSaveDisabled arg{}: entry {} -> {} (grey-out shift fix)", a, idx, idx + 1); }
                    }
                }
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
    if (g_origSsd) g_origSsd(params);
}

}  // namespace

// Hook JournalMenu::AdvanceMovie so the row is injected on the main thread once the System page's
// list exists. Called from HagMenu::InstallTrigger (before Hooking::Commit).
void InstallSystemInject() {
    const auto adv = offsets::FromRVA(offsets::kJournalMenu_AdvanceMovie);
    if (Hooking::Create<AdvanceFn>(adv, &Detour_JournalAdvance, g_origAdvance))
        HAG_INFO("HagUI: hooked JournalMenu::AdvanceMovie @{:#x} (runtime System-menu injection)", adv);
    else
        HAG_ERR("HagUI: failed to hook JournalMenu::AdvanceMovie");

    const auto ssd = offsets::FromRVA(offsets::kSetSaveDisabled);
    if (Hooking::Create<SsdFn>(ssd, &Detour_SetSaveDisabled, g_origSsd))
        HAG_INFO("HagUI: hooked SetSaveDisabled @{:#x} (grey-out shift fix)", ssd);
    else
        HAG_ERR("HagUI: failed to hook SetSaveDisabled");
}

}  // namespace hag::ui

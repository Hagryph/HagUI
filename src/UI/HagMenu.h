#pragma once

namespace hag::ui {

// HagUIMenu: our custom Scaleform menu, registered with the game's UI registry.
// Register() is safe to call live (just adds a name->creator entry). Create() is
// what the engine invokes when the menu is shown -- not exercised until we open it.
class HagMenu {
public:
    static constexpr const char* kName = "HagUIMenu";

    static void  Register();        // UI::Register(registry, "HagUIMenu", &Create)
    static void* Create();          // engine creator -> returns a new IMenu*
    static void  Open();            // UIMessageQueue::AddMessage("HagUIMenu", kShow)
    static void  Close();           // UIMessageQueue::AddMessage("HagUIMenu", kHide)
    static void  InstallTrigger();  // trampoline-hook the Main Menu to add the "HagUI" entry
};

// Hooks JournalMenu::AdvanceMovie to inject the "HagUI" row into the in-game System menu at runtime
// (no SWF file), attaching a native click handler. Defined in GfxInject.cpp.
void InstallSystemInject();

}  // namespace hag::ui

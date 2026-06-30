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
    static void  InstallTrigger();  // (debug) trampoline-hook the Main Menu Credits handler to Open()
};

}  // namespace hag::ui

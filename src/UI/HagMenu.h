#pragma once

namespace hag::ui {

// Owns the HagUI Scaleform menu. Real registration + drawing land once the
// Ghidra dump fills hag::offsets (see docs/UI-RE.md). For now this only proves
// the plugin lifecycle is wired end to end.
class HagMenu {
public:
    static constexpr const char* kName = "HagUIMenu";

    static void Register();  // will call UI::Register once offsets are known
};

}  // namespace hag::ui

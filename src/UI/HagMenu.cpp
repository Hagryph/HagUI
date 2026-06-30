#include "PCH.h"
#include "UI/HagMenu.h"
#include "Log.h"
#include "Offsets.h"

namespace hag::ui {

void HagMenu::Register() {
    if (offsets::kUI_Register == 0) {
        HAG_WARN("HagMenu::Register skipped - UI offsets not filled yet (see docs/UI-RE.md)");
        return;
    }
    // TODO(P2): cast offsets::FromRVA(kUI_Register) to the engine ABI and
    // register kName with a creator that builds our IMenu + loads the movie.
    HAG_INFO("HagMenu::Register - registering '{}'", kName);
}

}  // namespace hag::ui

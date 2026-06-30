#pragma once
#include "PCH.h"

// Hand-found addresses (no Address Library). Fill the RVAs from
// HagUI/ghidra/ui_dump.txt. A Ghidra "file address" looks like 0x14xxxxxxx;
// its RVA is (fileAddr - kImageBase).
namespace hag::offsets {

inline constexpr std::uintptr_t kImageBase = 0x140000000ull;

// ASLR-relocated module base at runtime.
inline std::uintptr_t Base() {
    return reinterpret_cast<std::uintptr_t>(::GetModuleHandleW(nullptr));
}
// Ghidra file address (0x14......) -> live runtime address.
inline std::uintptr_t FromFileAddr(std::uintptr_t fileAddr) {
    return Base() + (fileAddr - kImageBase);
}
// Raw RVA (offset from image base) -> live runtime address.
inline std::uintptr_t FromRVA(std::uintptr_t rva) {
    return Base() + rva;
}

// ---- UI subsystem RVAs — TO FILL from the Ghidra dump ----
// (kept 0 until then; HagMenu::Register no-ops while any are 0)
inline constexpr std::uintptr_t kUI_Register             = 0x0;
inline constexpr std::uintptr_t kUIMessageQueue_Add      = 0x0;
inline constexpr std::uintptr_t kBSScaleform_LoadMovieEx = 0x0;
inline constexpr std::uintptr_t kGFxValue_Invoke         = 0x0;

}  // namespace hag::offsets

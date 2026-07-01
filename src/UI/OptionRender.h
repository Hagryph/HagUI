#pragma once

// Renders the registered hag::api option pages into HagUI's live Scaleform movie
// and routes checkbox changes from AS back into hag::api::HagUI. Driven from the
// HagUI menu's per-frame tick (HagMenu.cpp :: HagTick).
namespace hag::ui {

class OptionRender {
public:
    // Push the current page/option model into the movie and call _root.HagBuildPages,
    // once per freshly-loaded movie (no-op until the SWF's frame_1 boot has run and
    // until a new movie appears). Safe to call every tick.
    static void BuildIfNeeded(void* movieView);
};

}  // namespace hag::ui

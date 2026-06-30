#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <variant>
#include <functional>
#include <memory>

// HagUI host API: mods register config pages/options here; the Scaleform front-end
// (the HagUI menu, injected into the Main Menu + the in-game Escape/System menu)
// renders them and reports changes back through OnControlChanged.
namespace hag::api {

// Global  -> shown in BOTH the Main Menu and the in-game (Escape/System) menu.
//            Its values persist outside any save (a shared/global config file).
// PerSave -> shown ONLY in-game; values belong to the loaded save.
enum class Scope { Global, PerSave };

// Where a menu is being built — decides which scopes are visible.
enum class MenuContext { MainMenu, InGame };

enum class Control { Toggle, Slider, Stepper, Text, Button };

using Value    = std::variant<bool, std::int64_t, double, std::string>;
using ChangeFn = std::function<void(const Value&)>;
using ClickFn  = std::function<void()>;

struct Option {
    std::string   id;
    std::string   label;
    Control       control{};
    Value         value{};
    double        min = 0.0, max = 1.0, step = 1.0;   // slider / stepper
    ChangeFn      onChange;
    std::uint32_t debounceMs = 0;                      // 0 = fire immediately
};

// Fluent page builder.
class Page {
public:
    Page(std::string title, Scope scope) : m_title(std::move(title)), m_scope(scope) {}

    Page& Toggle(std::string id, std::string label, bool initial, ChangeFn cb);
    Page& Slider(std::string id, std::string label, double mn, double mx, double step,
                 double initial, ChangeFn cb, std::uint32_t debounceMs = 200);
    Page& Stepper(std::string id, std::string label, double mn, double mx, double step,
                  double initial, ChangeFn cb);
    Page& Text(std::string id, std::string label, std::string initial, ChangeFn cb,
               std::uint32_t debounceMs = 300);
    Page& Button(std::string id, std::string label, ClickFn onClick);

    const std::string&         Title()   const { return m_title; }
    Scope                      GetScope() const { return m_scope; }
    const std::vector<Option>& Options() const { return m_options; }

private:
    friend class HagUI;
    std::string         m_title;
    Scope               m_scope;
    std::vector<Option> m_options;
};

class HagUI {
public:
    static HagUI& Get();

    // Register a page. Returned reference stays valid for the session.
    Page& RegisterPage(std::string title, Scope scope);

    // Pages a given menu context should show (MainMenu => Global only; InGame => Global + PerSave).
    std::vector<Page*> PagesFor(MenuContext ctx);

    // Front-end reports a control moved. Routed through per-option debounce.
    void OnControlChanged(const std::string& pageTitle, const std::string& optionId, Value v);

    // Pump debounce timers — call every menu tick with a monotonic millisecond clock.
    void PumpDebounce(std::uint64_t nowMs);

private:
    HagUI() = default;

    struct Pending { std::string page, id; Value value; std::uint64_t fireAtMs; };

    Option* Find(const std::string& page, const std::string& id);

    std::vector<std::unique_ptr<Page>> m_pages;
    std::vector<Pending>               m_pending;
    std::uint64_t                      m_nowMs = 0;
};

}  // namespace hag::api

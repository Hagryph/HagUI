# Model3D widget — Route A (reuse the engine renderer) RE notes

Goal: a HagUI **Model3D** widget that shows a live 3D model (player / NPC / static / plant — any TESForm)
inside our Scaleform panel. Chosen render approach (user, 2026-07): **reuse the engine's own menu-3D
renderer** and bind its render-target texture as an `img://` virtual image our SWF samples. No private
D3D draw / shaders — the engine draws the model with correct materials/lighting/skinning; we only set up
the scene + redirect its output into a texture the movie can show.

All addresses are RVAs off image base `0x140000000` (SkyrimSE 1.6.1170), hand-found in Ghidra.

## Verified live (HagIPC)
- **ID3D11Device** = `*(0x3286A10)`. Confirmed: its vtable `[5]` (`+0x28`, `CreateTexture2D`) points into
  `d3d11.dll`; `[7]` (`+0x38`) = `CreateShaderResourceView`, `[9]` (`+0x48`) = `CreateRenderTargetView`.
  → lets us create a `RENDER_TARGET | SHADER_RESOURCE` texture + RTV + SRV directly.

## `img://` virtual-image mechanism (from the `BGSUserIcon` registrar `FUN_140941970`)
The AS loads `img://BGSUserIcon`; native builds a named virtual-image entry + a texture and registers it.
- **Create entry** `FUN_140d2f140(BSFixedString* name)` → 0x58-byte entry:
  `+0x10/+0x18` defaults (`DAT_142029F00/F08`), `+0x20` = interned name (BSFixedString), `+0x48` = texture,
  `+0x50 |= 8` (flag).
- **Create texture (from pixels)** `FUN_140e48660(texMgr=DAT_1432887C0, w, h, pixels, usage, format, rtFlag)`
  → wrapper `{ ID3D11Texture2D* @0x00, ID3D11ShaderResourceView* @0x10, u16 w @0x1a, u8 fmt @0x1d }`.
  `format 0x1c` = `DXGI_FORMAT_R8G8B8A8_UNORM`. Internally calls `device->CreateTexture2D` (`DAT_143286A10`
  vtbl `+0x28`) and `CreateShaderResourceView` (`+0x38`). Does NOT set RENDER_TARGET bind flag — for our RT
  we build the texture ourselves via the device (we have it) with `RENDER_TARGET|SHADER_RESOURCE`.
- **Register** `FUN_140faf5e0(loader, entry)` (loader arg TBD — resolve the scaleform image-loader singleton
  so `img://NAME` resolves; `FUN_140941970` is the working template to mirror).

## Menu-3D render machinery (the inventory item preview — reuse this)
- **Inventory3DManager singleton** = `*(0x143187778)`.
  - entry `FUN_140927850(mgr, {form, extra})` → `FUN_140927880`.
  - **update/load** `FUN_140927880(mgr, form, extra)`: gets the scene node `FUN_140928130(mgr)`, then either
    clones the model directly `model->vtable[0x250](model, extra)` OR queues an async NIF-load task
    (task vtable `PTR_FUN_141792578`; task `+0xf`=form, `+0x10`=sceneNode, `+0x11`=extra; queue
    `FUN_1409290c0` + run `FUN_140e02680`), then attach/position `FUN_1409281a0` and finalize/render
    `FUN_140928700`.
  - **clear** `FUN_140927c40(mgr)` (releases model `+0x158`, resets).
  - fields: `+0x158` current loaded model/ref, `+0x60/+0x68` loadedModels BSTSmallArray, `+0x148` count,
    `+0x38` subsystem used for rotation, `+0x40` rotation state.
  - AS callbacks (registered for ItemMenu by `FUN_1409083a0`): `ShowItem3D` handler `0x140910c60`
    (`mgr = *(0x143187778)` → jmp `FUN_140927850`/`FUN_140927c40`), `StartMouseRotation` `0x1408eff70`,
    `StopMouseRotation` `0x1408eff80`, plus INI `bShowInventory3D:Interface`.
- **Menu-3D scene setup** `FUN_140643060` builds a shadow-scene node literally named **`"3DMenu"`** and a
  **256×256 render target** (`FUN_14149fbb0(...,0x100,...)`), with `Weather/LODRoot/...` — the menu scene.
- Menu open-3D flags on the host IMenu: `kCustomRendering (1<<15)` + `kRendersOffscreenTargets (1<<12)`
  make the engine invoke a custom draw callback for the menu (how the item 3D composites).

## Player / arbitrary model source
- Player live 3D: `Actor::Get3D` / `GetNiRootNode(0)` (RVA TBD) on player `*(0x31874F8)` (see game::actor).
- Static/plant/weapon: the inventory path already loads ANY `TESBoundObject`'s model by form — reuse it.

## Roadmap (staged, each hot-load-verified)
1. **Prove img://**: create a RT texture via `*(0x3286A10)`, wrap it, `FUN_140d2f140`-register as
   `hagCharModel`, resolve+call the loader register, clear it to a solid colour, `loadMovie("img://hagCharModel")`
   in the widget → confirm the fill shows inside the panel.
2. **Redirect the menu-3D target**: find the render target `FUN_140643060` creates + its NiCamera; render
   the "3DMenu" scene into OUR RT (or bind that RT's texture as `hagCharModel`).
3. **Put a model in the scene**: drive `Inventory3DManager::UpdateItem3D` (or attach player `Get3D`) so a
   model appears in the "3DMenu" scene.
4. **Generalize** by `formID` (0x14 = player live 3D; else load the form's model) + drag-rotate via
   `StartMouseRotation`/`StopMouseRotation` semantics.

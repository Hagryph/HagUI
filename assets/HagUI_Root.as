// HagUI root boot — overwrites scripts/frame_1/DoAction.as inside the cloned Credits shell.
// Mirrors the Credits root: forward onCodeObjectInit to Menu_mc, set up CodeObj + text format.
// Keeps _root drawable so our C++ DrawWelcome (Invoke _root.beginFill/moveTo/lineTo) still paints
// the golden/black panel underneath the (logic-only, empty) Menu_mc child clip.
function onCodeObjectInit()
{
   Menu_mc.onCodeObjectInit();
}
function ReleaseCodeObject()
{
   delete CodeObj;
}
var CodeObj = new Object();
Shared.GlobalFunc.MaintainTextFormat();
// Hide the vanilla Credits chrome (the Back buttons placed on the root stage); HagUI draws its
// own UI from C++. Menu_mc stays (it carries the input handleInput + focus), but its credits
// textField is never populated by our minimal class, so it renders nothing.
BackMouseButton._visible = false;
BackGamepadButton._visible = false;
stop();

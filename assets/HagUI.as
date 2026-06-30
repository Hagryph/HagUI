// HagUI.swf frame-1 script - INPUT ONLY. All UI/visuals/logic live in C++.
// The AS catches input events and calls native callbacks that C++ binds onto
// _root.CodeObj via CreateFunction (the Credits CodeObj pattern). The AS does the
// trivial key check so native's closeMenu is a no-arg callback (like Credits').
_root.CodeObj = new Object();

var hagListener = new Object();
hagListener.onKeyDown = function() {
    if (Key.getCode() == 27 && _root.CodeObj.closeMenu != undefined) {
        _root.CodeObj.closeMenu();          // ESC -> native HagMenu::Close()
    }
};
Key.addListener(hagListener);

_root.onMouseUp = function() {
    if (_root.CodeObj.onClick != undefined) {
        _root.CodeObj.onClick(_root._xmouse, _root._ymouse);  // for the in-menu button later
    }
};
stop();

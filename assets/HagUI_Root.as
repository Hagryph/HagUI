// HagUI root boot + Welcome screen — overwrites scripts/frame_1/DoAction.as in the
// cloned Credits CLIK shell. Keeps _root drawable, wires close, and renders a
// black+gold "gentlemen's club" Welcome built entirely in AS2 (gradients, embedded
// Skyrim fonts, rounded gold-hairline panels) — the Manga List design language.
//
// Tokens (from Manga List theme.css):
//   bg-0 #0A0A0C  bg-1 #121013  panel #1A1712  panel-2 #231E16  warm #241D0F
//   accent #E0B34A  accent-dim #B8862F  text #ECE6DA  text-dim #9C9486  text-faint #6B6456
//   radius 10/14  border gold@14%  border-strong gold@42%

function onCodeObjectInit()
{
   Menu_mc.onCodeObjectInit();
}
function ReleaseCodeObject()
{
   delete CodeObj;
}

// ---- rounded-rect path helpers (fill/line begun by the caller) ----
function rrPath(mc, x, y, w, h, r)
{
   mc.moveTo(x + r, y);
   mc.lineTo(x + w - r, y);
   mc.curveTo(x + w, y, x + w, y + r);
   mc.lineTo(x + w, y + h - r);
   mc.curveTo(x + w, y + h, x + w - r, y + h);
   mc.lineTo(x + r, y + h);
   mc.curveTo(x, y + h, x, y + h - r);
   mc.lineTo(x, y + r);
   mc.curveTo(x, y, x + r, y);
}
function rect(mc, x, y, w, h)
{
   mc.moveTo(x, y); mc.lineTo(x + w, y); mc.lineTo(x + w, y + h); mc.lineTo(x, y + h); mc.lineTo(x, y);
}
// rounded only on the left side (for the accent rail)
function rrLeft(mc, x, y, w, h, r)
{
   mc.moveTo(x + r, y);
   mc.lineTo(x + w, y);
   mc.lineTo(x + w, y + h);
   mc.lineTo(x + r, y + h);
   mc.curveTo(x, y + h, x, y + h - r);
   mc.lineTo(x, y + r);
   mc.curveTo(x, y, x + r, y);
}
// FFDec's AS2 compiler can't parse inline object literals, so build the gradient "box" matrix here.
function boxM(x, y, w, h, r)
{
   var m = new Object();
   m.matrixType = "box"; m.x = x; m.y = y; m.w = w; m.h = h; m.r = r;
   return m;
}
function mkText(parent, name, depth, x, y, w, h, html)
{
   parent.createTextField(name, depth, x, y, w, h);
   var t = parent[name];
   t.selectable = false;
   t.embedFonts = true;
   t.antiAliasType = "advanced";
   t.multiline = true;
   t.wordWrap = true;
   t.html = true;
   t.htmlText = html;
   return t;
}

// ---- reusable gold button: gradient bg + border, brightens on hover, fires a GameDelegate
// callback on its own click (direct hit-test, not the backdrop rectangle). ----
function paintButton(b, hover)
{
   b.clear();
   var a1 = 18;
   var a2 = 6;
   var ba = 42;
   if (hover) { a1 = 34; a2 = 14; ba = 78; }
   b.lineStyle(1, 0xE0B34A, ba, true, "none", "round", "round");
   b.beginGradientFill("linear", [0xE0B34A, 0xE0B34A], [a1, a2], [0, 255], _root.boxM(b._bx, b._by, b._bw, b._bh, Math.PI / 2));
   _root.rrPath(b, b._bx, b._by, b._bw, b._bh, 7);
   b.endFill();
}
function makeButton(parent, name, depth, bx, by, bw, bh, html, cb)
{
   // hover glow sits BEHIND as its own clip so it never enlarges the button's hit area
   var g = parent.createEmptyMovieClip(name + "_glow", depth);
   g.beginFill(0xE0B34A, 16);
   _root.rrPath(g, bx - 5, by - 5, bw + 10, bh + 10, 11);
   g.endFill();
   g._visible = false;
   var b = parent.createEmptyMovieClip(name, depth + 1);
   b._bx = bx; b._by = by; b._bw = bw; b._bh = bh; b._cb = cb; b._glow = g;
   _root.paintButton(b, false);
   var lbl = _root.mkText(b, "lbl", 1, bx, by, bw, 30, html);
   // vertical-center on the REAL text height (minus Flash's 2px top gutter) instead of a guess
   lbl._y = by + Math.round((bh - lbl.textHeight) / 2) - 2;
   b.onRollOver = function() { _root.paintButton(this, true); this._glow._visible = true; };
   b.onRollOut = function() { _root.paintButton(this, false); this._glow._visible = false; };
   b.onReleaseOutside = function() { _root.paintButton(this, false); this._glow._visible = false; };
   b.onRelease = function() { gfx.io.GameDelegate.call(this._cb, []); };
   return b;
}

// ---- navigation tabs. _root.HAG_TABS holds the labels (addTab appends one); the active tab
// is gold + underlined, inactive tabs are dim and brighten on hover; showPage builds each page.
// New tabs: addTab("LABEL") + a branch in showPage() for its content builder. ----
function tabHtml(label, act)
{
   var col = "#9C9486";
   if (act) { col = "#E0B34A"; }
   return "<font face='$EverywhereBoldFont' size='15' color='" + col + "'>" + label + "</font>";
}
function buildNav(card, nx, ny, nw)
{
   var nav = card.createEmptyMovieClip("nav", 18);
   nav.lineStyle(1, 0xE0B34A, 16);                         // baseline hairline under the row
   nav.moveTo(nx, ny + 34); nav.lineTo(nx + nw, ny + 34);
   var x = nx;
   var i = 0;
   while (i < _root.HAG_TABS.length)
   {
      var act = (i == _root.hagActive);
      var t = nav.createEmptyMovieClip("t" + i, 10 + i);
      t._idx = i;
      var lbl = _root.mkText(t, "lbl", 2, x, ny + 6, 280, 24, _root.tabHtml(_root.HAG_TABS[i], act));
      var tw = lbl.textWidth;
      if (!(tw > 10)) { tw = _root.HAG_TABS[i].length * 11 + 4; }
      t.beginFill(0xFFFFFF, 0); _root.rect(t, x - 7, ny, tw + 14, 35); t.endFill();   // invisible hit area
      if (act) { t.lineStyle(2, 0xE0B34A, 100); t.moveTo(x, ny + 34); t.lineTo(x + tw, ny + 34); }
      t.onRelease = function() { _root.selectTab(this._idx); };
      t.onRollOver = function() { _root.tabHover(this._idx, true); };
      t.onRollOut = function() { _root.tabHover(this._idx, false); };
      x = x + tw + 30;
      i = i + 1;
   }
}
function tabHover(idx, over)
{
   if (idx == _root.hagActive) { return; }
   var col = "#9C9486";
   if (over) { col = "#ECE6DA"; }
   _root.HagWelcome.card.nav["t" + idx].lbl.htmlText =
      "<font face='$EverywhereBoldFont' size='15' color='" + col + "'>" + _root.HAG_TABS[idx] + "</font>";
}
function addTab(label) { _root.HAG_TABS.push(label); }
function selectTab(idx)
{
   if (idx == _root.hagActive) { return; }
   _root.hagActive = idx;
   var card = _root.HagWelcome.card;
   card.nav.removeMovieClip();
   _root.buildNav(card, card._nx, card._ny, card._nw);
   _root.showPage(idx);
}
function showPage(idx)
{
   var card = _root.HagWelcome.card;
   card.content.removeMovieClip();
   var c = card.createEmptyMovieClip("content", 22);
   if (idx == 0) { _root.buildWelcomePage(c, card._cx, card._cyTop, card._cw2); }
}
function buildWelcomePage(c, x, y, w)
{
   _root.mkText(c, "lbl", 1, x, y, w, 22,
      "<font face='$EverywhereBoldFont' size='13' color='#6B6456'>W&nbsp;E&nbsp;L&nbsp;C&nbsp;O&nbsp;M&nbsp;E&nbsp;&nbsp;T&nbsp;O</font>");
   _root.mkText(c, "mark", 2, x - 2, y + 14, w, 92,
      "<font face='$EverywhereBoldFont' size='64' color='#ECE6DA'>Hag<font color='#E0B34A'>UI</font></font>");
   var dv = c.createEmptyMovieClip("dv", 3);
   dv.beginGradientFill("linear", [0xE0B34A, 0xE0B34A], [60, 0], [0, 255], _root.boxM(x, 0, 250, 2, 0));
   _root.rect(dv, x, y + 112, 250, 2); dv.endFill();
   _root.mkText(c, "tag", 4, x, y + 132, w, 70,
      "<font face='$EverywhereFont' size='18' color='#9C9486'>Your private control room for every Hagryph mod &#8212;<br>configuration, tools, and more, gathered in one place.</font>");
}

function buildWelcome()
{
   var W = Stage.width;
   var H = Stage.height;
   if (!(W > 200)) { W = 1280; H = 720; }

   var s = _root.createEmptyMovieClip("HagWelcome", 200);

   // ---- backdrop: vertical bg-1 -> bg-0, plus a warm gold radial glow top-right ----
   var bg = s.createEmptyMovieClip("bg", 1);
   bg.beginGradientFill("linear", [0x121013, 0x0A0A0C], [100, 100], [0, 255],
      boxM(0, 0, W, H, Math.PI / 2));
   rect(bg, 0, 0, W, H); bg.endFill();
   var glow = s.createEmptyMovieClip("glow", 2);
   glow.beginGradientFill("radial", [0x241D0F, 0x0A0A0C], [85, 0], [0, 255],
      boxM(W * 0.34, -H * 0.62, W * 1.05, H * 1.5, 0));
   rect(glow, 0, 0, W, H); glow.endFill();

   // ---- hero card ----
   var cw = 820;
   var ch = 462;
   var cx = Math.round((W - cw) / 2);
   var cy = Math.round((H - ch) / 2) - 6;
   // expose the panel rect (in _root coords) so the mouse handler can close only on outside clicks
   _root.hagX = cx; _root.hagY = cy; _root.hagW = cw; _root.hagH = ch;
   var card = s.createEmptyMovieClip("card", 10);

   // thin, soft drop shadow (two light offset layers; the old single 14px offset read as a heavy band)
   card.beginFill(0x000000, 20); rrPath(card, cx, cy + 3, cw, ch, 14); card.endFill();
   card.beginFill(0x000000, 11); rrPath(card, cx, cy + 6, cw, ch, 14); card.endFill();
   // panel fill + gold hairline border in one pass
   card.lineStyle(1, 0xE0B34A, 42, true, "none", "round", "round");
   card.beginFill(0x1A1712, 96); rrPath(card, cx, cy, cw, ch, 14); card.endFill();

   // ---- left accent: a soft glow fading right + a bright accent->accent-dim rail. Both are plain
   // full-height bars CLIPPED by a mask using the card's IDENTICAL rounded-rect path, so they
   // follow the radius-14 corners exactly and stay flush with the border (no manual corner math). ----
   var railG = card.createEmptyMovieClip("railG", 14);
   railG.beginGradientFill("linear", [0xE0B34A, 0xE0B34A], [26, 0], [0, 255], boxM(cx, cy, 30, ch, 0));
   rect(railG, cx, cy, 30, ch); railG.endFill();
   railG.beginGradientFill("linear", [0xE0B34A, 0xB8862F], [100, 100], [0, 255], boxM(cx, cy, 6, ch, Math.PI / 2));
   rect(railG, cx, cy, 6, ch); railG.endFill();
   var rmask = card.createEmptyMovieClip("rmask", 13);
   rmask.beginFill(0xFFFFFF, 100); rrPath(rmask, cx, cy, cw, ch, 14); rmask.endFill();
   railG.setMask(rmask);

   // corner flourishes (thin gold L-brackets, top-left + bottom-right) for the framed look
   var fl = card.createEmptyMovieClip("fl", 16);
   fl.lineStyle(1, 0xE0B34A, 30);
   fl.moveTo(cx + 30, cy + 22); fl.lineTo(cx + 56, cy + 22);
   fl.moveTo(cx + cw - 30, cy + ch - 22); fl.lineTo(cx + cw - 56, cy + ch - 22);

   var px = cx + 60;
   var cwid = cw - 60 - 48;

   // ---- navigation tab strip (registerable; _root.HAG_TABS holds the labels) ----
   card._nx = px; card._ny = cy + 28; card._nw = cwid;
   buildNav(card, px, cy + 28, cwid);

   // ---- content area: the active tab's page (rebuilt by showPage on tab change) ----
   card._cx = px; card._cyTop = cy + 86; card._cw2 = cwid;
   showPage(_root.hagActive);

   // real CLOSE button (hover-highlights, closes on its own click) + a quiet ESC hint
   var btnW = 152;
   var btnH = 40;
   var btnX = px;
   var btnY = cy + ch - 86;
   makeButton(card, "closeBtn", 28, btnX, btnY, btnW, btnH,
      "<p align='center'><font face='$EverywhereBoldFont' size='15' color='#E0B34A'>CLOSE</font></p>", "CloseHagUI");
   mkText(card, "hint", 31, btnX + btnW + 18, btnY + 11, 230, 22,
      "<font face='$EverywhereFont' size='14' color='#6B6456'>or press&nbsp;&nbsp;ESC</font>");

   // footer mark (bottom-right inside the card)
   mkText(card, "foot", 24, cx + cw - 230, cy + ch - 40, 200, 20,
      "<p align='right'><font face='$EverywhereBoldFont' size='11' color='#6B6456'>HAGRYPH&nbsp;&nbsp;&#183;&nbsp;&nbsp;EST.&nbsp;MMXXVI</font></p>");
}

// ---- boot ----
var CodeObj = new Object();
Shared.GlobalFunc.MaintainTextFormat();
// Hide the vanilla Credits chrome; Menu_mc stays (carries handleInput + focus) but renders nothing.
BackMouseButton._visible = false;
BackGamepadButton._visible = false;
// tab registry (for now just Welcome; addTab("LABEL") + a showPage() branch adds more)
_root.HAG_TABS = ["WELCOME"];
_root.hagActive = 0;
buildWelcome();
stop();

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
   var cw = 768;
   var ch = 404;
   var cx = Math.round((W - cw) / 2);
   var cy = Math.round((H - ch) / 2) - 6;
   var card = s.createEmptyMovieClip("card", 10);

   // soft drop shadow (offset, faint)
   card.beginFill(0x000000, 34); rrPath(card, cx + 2, cy + 14, cw, ch, 14); card.endFill();
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

   var px = cx + 60;          // content left
   var tw = cw - 60 - 56;     // content width

   // label
   mkText(card, "lbl", 20, px, cy + 50, tw, 22,
      "<font face='$EverywhereBoldFont' size='13' color='#6B6456'>W&nbsp;E&nbsp;L&nbsp;C&nbsp;O&nbsp;M&nbsp;E&nbsp;&nbsp;T&nbsp;O</font>");

   // wordmark: Hag (light) + UI (gold)
   mkText(card, "mark", 21, px - 2, cy + 66, tw, 96,
      "<font face='$EverywhereBoldFont' size='74' color='#ECE6DA'>Hag<font color='#E0B34A'>UI</font></font>");

   // gold divider (fades out to the right)
   var dv = card.createEmptyMovieClip("dv", 22);
   dv.beginGradientFill("linear", [0xE0B34A, 0xE0B34A], [60, 0], [0, 255],
      boxM(px, 0, 250, 2, 0));
   rect(dv, px, cy + 178, 250, 2); dv.endFill();

   // tagline
   mkText(card, "tag", 23, px, cy + 198, tw, 70,
      "<font face='$EverywhereFont' size='18' color='#9C9486'>Your private control room for every Hagryph mod &#8212;<br>configuration, tools, and more, gathered in one place.</font>");

   // hint pill: ESC / click to close
   var pw = 232;
   var ph = 32;
   var plx = px;
   var ply = cy + ch - 76;
   var pill = card.createEmptyMovieClip("pill", 30);
   pill.lineStyle(1, 0xE0B34A, 34, true, "none", "round", "round");
   pill.beginFill(0xE0B34A, 12); rrPath(pill, plx, ply, pw, ph, ph / 2); pill.endFill();
   mkText(pill, "pt", 1, plx, ply + 7, pw, 22,
      "<p align='center'><font face='$EverywhereBoldFont' size='13' color='#E0B34A'>ESC&nbsp;&nbsp;&#183;&nbsp;&nbsp;CLICK&nbsp;TO&nbsp;CLOSE</font></p>");

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
buildWelcome();
stop();

var ffSettings = this;
var ffFocused = false;
var ffOpen = false;
var ffDetails = false;
var ffSelected = 0;
var ffCount = 0;
var ffTick = 0;
var ffMenuTick = 0;
// The page's rows as the Settings pages lay theirs out (subOption's coordinates): the title in the header font, the
// value under it between the two arrows, the row highlight behind the selected row
var ffRowTop = -69.2;
var ffRowStep = 77.5;
var ffSlots = 4;
var ffPanel = createEmptyMovieClip("ffPanel", 1200);
ffPanel._x = subOption._x;
ffPanel._y = subOption._y;
ffPanel._visible = false;
function ffText(parent, name, depth, x, y, width, height, font, size, align)
{
    parent.createTextField(name, depth, x, y, width, height);
    var field = parent[name];
    field.embedFonts = true;
    field.selectable = false;
    var format = new TextFormat(font, size, 0x000000);
    format.align = align;
    field.setNewTextFormat(format);
    field.setTextFormat(format);
    return field;
}
// Fits the text into the field: the font size goes down until it does
function ffFit(field, size, least)
{
    var format = field.getTextFormat();
    format.size = size;
    field.setTextFormat(format);
    while(field.textWidth > field._width - 5 && format.size > least)
    {
        format.size--;
        field.setTextFormat(format);
    }
}
// The Plugins tab under Controls, in the tabs' font and slant; the tabs' red circle marks it
var ffLink = ffText(this, "ffLink", 1199, -342, 198, 224, 38, "_Font_Header", 30, "center");
ffLink._rotation = -6.75;
ffLink.text = "PLUGINS";
var ffCircle = highlight.duplicateMovieClip("ffCircle", 1198);
ffCircle.gotoAndStop(4);
ffCircle._x = highlight._x + 12.4;
ffCircle._y = highlight._y + 67.5;
ffCircle._visible = false;
ffText(ffPanel, "heading", 1, -165, -122, 382, 34, "_Font_Header", 24, "right");
ffText(ffPanel, "help", 2, -175, 230, 410, 40, "_Font_Body", 16, "left");
ffPanel.help.multiline = true;
ffPanel.help.wordWrap = true;
ffText(ffPanel, "restart", 3, -175, 269, 280, 20, "_Font_Body", 13, "left");
ffText(ffPanel, "moreHelp", 5, 105, 269, 130, 20, "_Font_Body", 13, "right");
ffText(ffPanel, "details", 4, -175, -80, 410, 310, "_Font_Body", 19, "left");
ffPanel.details.multiline = true;
ffPanel.details.wordWrap = true;
var ffRows = new Array();
for(var ffI = 0; ffI < ffSlots; ffI++)
{
    var ffRow = ffPanel.createEmptyMovieClip("row" + ffI, 10 + ffI * 5);
    ffRow._y = ffRowTop + ffI * ffRowStep;
    var ffBar = ffRow.attachMovie("ffRowHighlight", "bar", 1);
    ffBar._x = 27.5;
    ffBar._y = 26;
    ffText(ffRow, "label", 2, -165, 0, 382, 36, "_Font_Header", 30, "right");
    ffText(ffRow, "value", 3, 36, 30.1, 169, 28, "_Font_Body", 20, "right");
    var ffLeft = ffRow.attachMovie("ffArrow", "left", 4);
    ffLeft._x = 16;
    ffLeft._y = 42.1;
    var ffRight = ffRow.attachMovie("ffArrow", "right", 5);
    ffRight._x = 215;
    ffRight._y = 41.1;
    ffRight._xscale = -100;
    ffRows.push(ffRow);
}
// More rows above or below: the page's arrow turned up or down in the empty left of the first and last rows, clickable
var ffUp = ffPanel.attachMovie("ffArrow", "up", 40);
ffUp._x = -150;
ffUp._y = ffRowTop + 14;
ffUp._rotation = 90;
var ffDown = ffPanel.attachMovie("ffArrow", "down", 41);
ffDown._x = -150;
ffDown._y = ffRowTop + (ffSlots - 1) * ffRowStep + 44;
ffDown._rotation = -90;
function ffMark(focused)
{
    ffFocused = focused;
    ffCircle._visible = focused || ffOpen;
    ffCircle._alpha = ffOpen ? 50 : 100;
    highlight._visible = !focused && !ffOpen;
    // The page shows the focused tab's settings: none for Plugins until it opens
    subOption._visible = !focused && !ffOpen;
}
function ffClose()
{
    ffOpen = false;
    ffPanel._visible = false;
    subOption._visible = true;
    pageDark.gotoAndPlay("off");
    activationEffect.gotoAndPlay(2);
    _root.soundPauseBack();
    ffMark(true);
}
function ffShow()
{
    ffOpen = true;
    ffPanel._visible = true;
    subOption._visible = false;
    pageDark.gotoAndPlay(2);
    activationEffect.gotoAndPlay(2);
    _root.soundPauseSelect();
    ffMark(false);
    fscommand("FFOpen", "");
}
_global.FFClose = function() { ffSettings.ffClose(); };
_global.FFLabel = function(text) { ffSettings.ffLink.text = text.toUpperCase(); };
_global.FFBegin = function(title, count, selection, details, help, restart, moreHelp)
{
    var panel = ffSettings.ffPanel;
    if(ffSettings.ffSelected != selection && ffSettings.ffOpen) _root.soundPauseSecondaryScroll();
    ffSettings.ffSelected = selection;
    ffSettings.ffCount = count;
    ffSettings.ffDetails = details != 0;
    panel.heading.text = title.toUpperCase();
    ffSettings.ffFit(panel.heading, 24, 14);
    panel.help.text = help;
    // Two lines of help: a longer text is set smaller, down to the size of the line under it
    var format = panel.help.getTextFormat();
    format.size = 16;
    panel.help.setTextFormat(format);
    while(panel.help.textHeight > panel.help._height - 4 && format.size > 10)
    {
        format.size--;
        panel.help.setTextFormat(format);
    }
    panel.help._visible = !ffSettings.ffDetails;
    panel.details._visible = ffSettings.ffDetails;
    panel.details.text = help;
    panel.details.scroll = 1;
    panel.restart.text = restart;
    panel.moreHelp.text = moreHelp;
    for(var i = 0; i < ffSettings.ffRows.length; i++)
    {
        var row = ffSettings.ffRows[i];
        row._visible = !ffSettings.ffDetails && i < count;
        if(row.bar._visible != (i == selection) && i == selection) row.bar.gotoAndPlay(1);
        row.bar._visible = i == selection;
    }
    if(ffSettings.ffDetails) _global.FFMarks(0, 0);
};
_global.FFRow = function(index, label, value)
{
    if(index >= 0 && index < ffSettings.ffRows.length)
    {
        var row = ffSettings.ffRows[index];
        row.label.text = label.toUpperCase();
        ffSettings.ffFit(row.label, 30, 16);
        row.value.text = value;
        ffSettings.ffFit(row.value, 20, 13);
        row.value._visible = value != "";
        row.left._visible = value != "";
        row.right._visible = value != "";
        // A row that opens a page or acts has its title alone, down to the middle of the row
        row.label._y = value != "" ? 0 : 14;
    }
};
_global.FFMarks = function(above, below)
{
    ffSettings.ffUp._visible = above != 0;
    ffSettings.ffDown._visible = below != 0;
};
// The arrow plays its press like the Settings pages' arrows
function ffPress(index, direction)
{
    var arrow = direction < 0 ? ffRows[index].left : ffRows[index].right;
    if(arrow._visible) arrow.gotoAndPlay(2);
}
var ffNativeKey = settingsListener.onKeyDown;
settingsListener.onKeyDown = function()
{
    var code = Key.getCode();
    if(ffOpen)
    {
        if(ffDetails && (code == 38 || code == 40))
        {
            ffPanel.details.scroll += code == 38 ? -1 : 1;
            return;
        }
        if(code == 38) fscommand("FFInput", "up");
        else if(code == 40) fscommand("FFInput", "down");
        else if(code == 37) { ffPress(ffSelected, -1); fscommand("FFInput", "left"); }
        else if(code == 39) { ffPress(ffSelected, 1); fscommand("FFInput", "right"); }
        else if(code == 13 || code == 17) fscommand("FFInput", "accept");
        else if(code == 16 || code == 27) fscommand("FFInput", "back");
        else if(code == 46) fscommand("FFInput", "help");
        return;
    }
    if(ffFocused)
    {
        if(code == 13 || code == 17) { ffShow(); return; }
        if(code == 40) return;
        ffMark(false);
        _root.soundPauseSecondaryScroll();
        if(code == 38) return;
    }
    if(tier == 0 && iCategory == 3 && code == 40 && !_global.SettingsPromptActive)
    {
        ffMark(true);
        _root.soundPauseSecondaryScroll();
        return;
    }
    ffNativeKey();
};
// The row under the mouse (subOption's coordinates); -1: none
// The scroll mark under the mouse: -1 up, 1 down, 0 none (an area around the small arrow)
function ffMarkAt()
{
    var x = ffPanel._xmouse;
    var y = ffPanel._ymouse;
    if(x < -175 || x > -120) return 0;
    if(ffUp._visible && Math.abs(y - ffUp._y) < 22) return -1;
    if(ffDown._visible && Math.abs(y - ffDown._y) < 22) return 1;
    return 0;
}
function ffRowAt()
{
    if(ffMarkAt() != 0) return -1;
    var x = ffPanel._xmouse;
    var y = ffPanel._ymouse;
    if(x < -170 || x > 235) return -1;
    var index = Math.floor((y - ffRowTop + 4) / ffRowStep);
    return index >= 0 && index < ffCount && index < ffSlots ? index : -1;
}
var ffNativeMouseMove = mouseListener.onMouseMove;
var ffNativeMouseDown = mouseListener.onMouseDown;
mouseListener.onMouseMove = function()
{
    if(ffOpen)
    {
        var index = ffDetails ? -1 : ffRowAt();
        if(index >= 0 && index != ffSelected) fscommand("FFInput", "select:" + index);
        return;
    }
    if(tier == 0 && ffLink.hitTest(_root._xmouse, _root._ymouse))
    {
        if(!ffFocused) _root.soundPauseSecondaryScroll();
        ffMark(true);
        return;
    }
    if(ffFocused) ffMark(false);
    ffNativeMouseMove();
};
mouseListener.onMouseDown = function()
{
    if(ffOpen)
    {
        if(_parent.IsButtonHit("B")) { fscommand("FFInput", "back"); return; }
        if(ffPanel.moreHelp.hitTest(_root._xmouse, _root._ymouse)) { fscommand("FFInput", "help"); return; }
        if(ffDetails) return;
        var mark = ffMarkAt();
        if(mark != 0) { fscommand("FFInput", "scroll:" + mark); return; }
        var index = ffRowAt();
        if(index < 0) return;
        fscommand("FFInput", "select:" + index);
        var row = ffRows[index];
        if(row.left._visible && row.left.hitTest(_root._xmouse, _root._ymouse)) { ffPress(index, -1); fscommand("FFInput", "left"); }
        else if(row.right._visible && row.right.hitTest(_root._xmouse, _root._ymouse)) { ffPress(index, 1); fscommand("FFInput", "right"); }
        else fscommand("FFInput", "accept");
        return;
    }
    if(tier == 0 && ffLink.hitTest(_root._xmouse, _root._ymouse)) { ffShow(); return; }
    ffNativeMouseDown();
};
ffPanel.onEnterFrame = function()
{
    // The tab stays like the other tabs while one of their pages is open
    // The menu around the Settings page is shown (its other pages too): no drops over it
    var shown = ffSettings._parent;
    while(shown != undefined && shown._visible) shown = shown._parent;
    if(shown == undefined && ++ffMenuTick % 15 == 0) fscommand("FFMenu", "");
    var ancestor = ffSettings;
    while(ancestor != undefined)
    {
        if(!ancestor._visible)
        {
            if(ffOpen)
            {
                ffOpen = false;
                ffPanel._visible = false;
                subOption._visible = true;
                ffMark(false);
                fscommand("FFDismiss", "");
            }
            return;
        }
        ancestor = ancestor._parent;
    }
    if(++ffTick % 15 == 0) fscommand(ffOpen ? "FFRefresh" : "FFLabel", "");
};
fscommand("FFLabel", "");

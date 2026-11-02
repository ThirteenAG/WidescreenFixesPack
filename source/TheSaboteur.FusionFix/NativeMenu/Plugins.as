var ffSettings = this;
var ffFocused = false;
var ffOpen = false;
var ffDetails = false;
var ffSelected = 0;
var ffCount = 0;
var ffTick = 0;
var ffPanel = createEmptyMovieClip("ffPanel", 1200);
ffPanel._visible = false;
function ffText(parent, name, depth, x, y, width, height, size)
{
    parent.createTextField(name, depth, x, y, width, height);
    var field = parent[name];
    field.embedFonts = true;
    field.selectable = false;
    var format = new TextFormat("_Font_Body", size, 0x251D18);
    format.leading = 3;
    field.setNewTextFormat(format);
    field.setTextFormat(format);
    return field;
}
var ffLink = ffText(this, "ffLink", 1199, -350, 184, 300, 48, 31);
ffLink._rotation = -6.75;
ffLink.text = "Plugins";
ffLink.onSetFocus = function() { Selection.setFocus(null); };
ffPanel.beginFill(0xD6C9A6, 100);
ffPanel.lineStyle(2, 0x514132, 100);
ffPanel.moveTo(-390, -224);
ffPanel.lineTo(425, -224);
ffPanel.lineTo(425, 280);
ffPanel.lineTo(-390, 280);
ffPanel.lineTo(-390, -224);
ffPanel.endFill();
ffText(ffPanel, "heading", 1, -365, -210, 765, 48, 29);
ffText(ffPanel, "help", 2, -365, 152, 765, 75, 19);
ffPanel.help.multiline = true;
ffPanel.help.wordWrap = true;
ffText(ffPanel, "restart", 3, -365, 235, 765, 42, 17);
ffPanel.restart.multiline = true;
ffPanel.restart.wordWrap = true;
ffText(ffPanel, "moreHelp", 5, -365, 213, 765, 25, 17);
ffText(ffPanel, "details", 4, -365, -148, 765, 369, 23);
ffPanel.details.multiline = true;
ffPanel.details.wordWrap = true;
var ffRows = new Array();
for(var ffI = 0; ffI < 8; ffI++)
{
    var ffRow = ffPanel.createEmptyMovieClip("row" + ffI, 10 + ffI);
    ffRow._x = -365;
    ffRow._y = -151 + ffI * 37;
    ffText(ffRow, "label", 1, 0, 0, 510, 36, 24);
    ffText(ffRow, "value", 2, 515, 0, 250, 36, 23);
    ffRows.push(ffRow);
}
function ffClose()
{
    ffOpen = false;
    ffPanel._visible = false;
    ffFocused = true;
    ffLink.textColor = 0x9A161C;
}
function ffShow()
{
    ffOpen = true;
    ffPanel._visible = true;
    fscommand("FFOpen", "");
}
_global.FFClose = function() { ffSettings.ffClose(); };
_global.FFLabel = function(text) { ffSettings.ffLink.text = text; };
_global.FFBegin = function(title, count, selection, details, help, restart, moreHelp)
{
    ffSettings.ffSelected = selection;
    ffSettings.ffCount = count;
    ffSettings.ffDetails = details != 0;
    ffSettings.ffPanel.heading.text = title;
    ffSettings.ffPanel.help.text = help;
    ffSettings.ffPanel.help._visible = !ffSettings.ffDetails;
    ffSettings.ffPanel.details._visible = ffSettings.ffDetails;
    ffSettings.ffPanel.details.text = help;
    ffSettings.ffPanel.details.scroll = 1;
    ffSettings.ffPanel.restart.text = restart;
    ffSettings.ffPanel.moreHelp.text = moreHelp;
    for(var i = 0; i < ffSettings.ffRows.length; i++)
    {
        ffSettings.ffRows[i]._visible = !ffSettings.ffDetails && i < count;
        var color = i == selection ? 0x9A161C : 0x251D18;
        ffSettings.ffRows[i].label.textColor = color;
        ffSettings.ffRows[i].value.textColor = color;
    }
};
_global.FFRow = function(index, label, value)
{
    if(index >= 0 && index < ffSettings.ffRows.length)
    {
        ffSettings.ffRows[index].label.text = label;
        ffSettings.ffRows[index].value.text = value;
        var fields = [ffSettings.ffRows[index].label, ffSettings.ffRows[index].value];
        for(var i = 0; i < fields.length; i++)
        {
            var format = fields[i].getTextFormat();
            format.size = 24;
            fields[i].setTextFormat(format);
            while(fields[i].textWidth > fields[i]._width - 5 && format.size > 16)
            {
                format.size--;
                fields[i].setTextFormat(format);
            }
        }
    }
};
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
        else if(code == 37) fscommand("FFInput", "left");
        else if(code == 39) fscommand("FFInput", "right");
        else if(code == 13 || code == 17) fscommand("FFInput", "accept");
        else if(code == 16 || code == 27) fscommand("FFInput", "back");
        else if(code == 46) fscommand("FFInput", "help");
        return;
    }
    if(ffFocused)
    {
        if(code == 13 || code == 17) { ffShow(); return; }
        if(code == 40) return;
        ffFocused = false;
        ffLink.textColor = 0x251D18;
        highlight._visible = true;
        if(code == 38) return;
    }
    if(tier == 0 && iCategory == 3 && code == 40 && !_global.SettingsPromptActive)
    {
        ffFocused = true;
        ffLink.textColor = 0x9A161C;
        highlight._visible = false;
        return;
    }
    ffNativeKey();
};
var ffNativeMouseMove = mouseListener.onMouseMove;
var ffNativeMouseDown = mouseListener.onMouseDown;
mouseListener.onMouseMove = function()
{
    if(ffOpen)
    {
        if(!ffDetails)
        {
            for(var i = 0; i < ffCount && i < 8; i++)
            {
                if(ffRows[i].hitTest(_root._xmouse, _root._ymouse) && i != ffSelected)
                {
                    fscommand("FFInput", "select:" + i);
                    return;
                }
            }
        }
        return;
    }
    if(tier == 0 && ffLink.hitTest(_root._xmouse, _root._ymouse))
    {
        ffFocused = true;
        ffLink.textColor = 0x9A161C;
        highlight._visible = false;
        return;
    }
    ffFocused = false;
    ffLink.textColor = 0x251D18;
    highlight._visible = true;
    ffNativeMouseMove();
};
mouseListener.onMouseDown = function()
{
    if(ffOpen)
    {
        if(ffPanel.moreHelp.hitTest(_root._xmouse, _root._ymouse))
        {
            fscommand("FFInput", "help");
            return;
        }
        if(!ffDetails)
        {
            for(var i = 0; i < ffCount && i < 8; i++)
            {
                if(ffRows[i].hitTest(_root._xmouse, _root._ymouse))
                {
                    fscommand("FFInput", "select:" + i);
                    fscommand("FFInput", "accept");
                    return;
                }
            }
        }
        return;
    }
    if(tier == 0 && ffLink.hitTest(_root._xmouse, _root._ymouse)) { ffShow(); return; }
    ffNativeMouseDown();
};
ffPanel.onEnterFrame = function()
{
    ffLink._visible = tier == 0 && !ffOpen;
    var ancestor = ffSettings;
    while(ancestor != undefined)
    {
        if(!ancestor._visible)
        {
            if(ffOpen)
            {
                ffOpen = false;
                ffFocused = false;
                ffPanel._visible = false;
                fscommand("FFDismiss", "");
            }
            return;
        }
        ancestor = ancestor._parent;
    }
    if(++ffTick % 15 == 0) fscommand(ffOpen ? "FFRefresh" : "FFLabel", "");
};
fscommand("FFLabel", "");

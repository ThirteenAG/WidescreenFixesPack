stop();
var ffHudRoot = this._parent;
var ffHudNames = ["Tutorial", "Mail", "Minimap", "ObjectiveTray", "Inventory", "RaceHUD", "Pickup", "TimerHUD"];
var ffHudMasks = [];
for(var ffIndex = 0; ffIndex < ffHudNames.length; ffIndex++)
{
    var ffMask = this.createEmptyMovieClip("ffHudMask" + ffIndex, this.getNextHighestDepth());
    ffMask.beginFill(0x000000, 100);
    ffMask.moveTo(0, 0);
    ffMask.lineTo(1, 0);
    ffMask.lineTo(1, 1);
    ffMask.lineTo(0, 1);
    ffMask.lineTo(0, 0);
    ffMask.endFill();
    ffMask._visible = false;
    ffHudMasks.push(ffMask);
}
_global.FFHudMask = function(inset, left, top, width, height)
{
    for(var i = 0; i < ffHudNames.length; i++)
    {
        var target = ffHudRoot[ffHudNames[i]];
        var mask = ffHudMasks[i];
        var visible = target._visible;
        if(inset > 0)
        {
            mask._x = left + inset;
            mask._y = top;
            mask._width = Math.max(1, width - 2 * inset);
            mask._height = Math.max(1, height);
            mask._visible = true;
            target.setMask(mask);
        }
        else
        {
            target.setMask(null);
            mask._visible = false;
        }
        target._visible = visible;
    }
};
var ffCinemaBars = [];
var ffCinemaSaved = null;
for(var ffBarIndex = 0; ffBarIndex < 4; ffBarIndex++)
{
    var ffBar = this.createEmptyMovieClip("ffCinemaBar" + ffBarIndex, this.getNextHighestDepth());
    ffBar.beginFill(0x000000, 100);
    ffBar.moveTo(0, 0);
    ffBar.lineTo(1, 0);
    ffBar.lineTo(1, 1);
    ffBar.lineTo(0, 1);
    ffBar.lineTo(0, 0);
    ffBar.endFill();
    ffBar._visible = false;
    ffCinemaBars.push(ffBar);
}
function ffPlaceCinemaBar(index, x, y, width, height)
{
    var bar = ffCinemaBars[index];
    bar._visible = width > 0.01 && height > 0.01;
    if(bar._visible)
    {
        bar._x = x;
        bar._y = y;
        bar._width = width;
        bar._height = height;
    }
}
_global.FFCinemaBegin = function(mode, animated, active, requested, left, top, width, height)
{
    var original = ffHudRoot.dialogLetterbox;
    ffCinemaSaved = [];
    var frameWidth = width;
    var frameHeight = height;
    if(active && (mode & 2) && width / height > 16 / 9)
        frameWidth = height * 16 / 9;
    if(active && (mode & 1) && width / height < 16 / 9)
        frameHeight = width * 9 / 16;
    var insetX = (width - frameWidth) / 2;
    var insetY = (height - frameHeight) / 2;
    if(insetX > 0.01)
    {
        ffPlaceCinemaBar(0, left - 2, top - 2, insetX + 2, height + 4);
        ffPlaceCinemaBar(1, left + width - insetX, top - 2, insetX + 2, height + 4);
    }
    else
    {
        ffCinemaBars[0]._visible = false;
        ffCinemaBars[1]._visible = false;
    }
    if(insetY > 0.01)
    {
        ffPlaceCinemaBar(2, left - 2, top - 2, width + 4, insetY + 2);
        ffPlaceCinemaBar(3, left - 2, top + height - insetY, width + 4, insetY + 2);
    }
    else
    {
        ffCinemaBars[2]._visible = false;
        ffCinemaBars[3]._visible = false;
    }
    for(var i = 0; i < 4; i++)
    {
        var part = original["q" + (i + 1)];
        var saved = [part, part._visible, part._x, part._y, part._xscale, part._yscale];
        if(!(mode & 1))
            part._visible = false;
        else
        {
            if(!animated)
                part.gotoAndStop(requested ? 65 : 1);
            if(active)
            {
                var right = i == 1 || i == 3;
                var bottom = i >= 2;
                part._x = (right ? 322 : -324) * frameWidth / 1280;
                part._y = (bottom ? 200 : -200) * frameHeight / 720;
                part._xscale = (right ? -100 : 100) * frameWidth / 1280;
                part._yscale = (bottom ? -100 : 100) * frameHeight / 720;
            }
        }
        saved.push(part._visible, part._x, part._y, part._xscale, part._yscale);
        ffCinemaSaved.push(saved);
    }
};
_global.FFCinemaEnd = function()
{
    if(ffCinemaSaved != null)
    {
        for(var i = 0; i < ffCinemaSaved.length; i++)
        {
            var saved = ffCinemaSaved[i];
            var part = saved[0];
            var properties = ["_visible", "_x", "_y", "_xscale", "_yscale"];
            for(var p = 0; p < properties.length; p++)
            {
                if(part[properties[p]] == saved[p + 6])
                    part[properties[p]] = saved[p + 1];
            }
        }
        ffCinemaSaved = null;
    }
    for(var j = 0; j < ffCinemaBars.length; j++)
        ffCinemaBars[j]._visible = false;
};
_global.FFInterfaceVersion = 2;

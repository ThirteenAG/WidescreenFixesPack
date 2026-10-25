#pragma once

namespace FarCryMenu
{
    // Native Lua 4 syntax and existing shipped UI skins/helpers only. The
    // fixed screen/control pool belongs to the engine and is retired by its
    // ordinary Release/reload paths; no stock page or callback is replaced.
    inline constexpr const char* nativeMenuLua = R"WFP(
function WFP_FindScreen(name)
    local count=UI:GetScreenCount();
    if count<0 or count>256 then return nil end
    for i=0,count-1 do
        local screen=UI:GetScreen(i);
        local actual=screen and screen:GetName();
        if actual==name or (actual and strlower and strlower(actual)==strlower(name)) then return screen end
    end
    return nil;
end
function WFP_FindWidget(screen,name)
    if not screen or not screen.GetWidgetCount or not screen.GetWidget then return nil end
    -- Shipped screens access child widgets as Sender.GameOptions, etc.
    -- The installed screen:GetWidget wrapper discards its native return value.
    local direct=screen[name];
    if direct and direct.GetScreen and direct:GetScreen()==screen then return direct end
    -- Shipped UI code uses the global getter for dynamically created widgets.
    if UI.GetWidget then
        direct=UI:GetWidget(name,screen);
        if direct and direct.GetScreen and direct:GetScreen()==screen then return direct end
    end
    direct=screen:GetWidget(name);
    if direct then return direct end
    local count=screen:GetWidgetCount();
    if count<0 or count>256 then return nil end
    for i=0,count-1 do
        local widget=screen:GetWidget(i);
        if widget and widget:GetName()==name then return widget end
    end
    return nil;
end
function WFP_Identity(expected)
    local s=WFP_Menu;
    return s and (not expected or expected==s) and not s.dead and UI==s.ui and
        WFP_FindScreen("Options")==s.source and
        WFP_FindScreen("WFPAdvancedOptions")==s.private;
end
function WFP_OwnRow(sender,expected)
    local s=WFP_Menu;
    if not WFP_Identity(expected) or not sender or not sender.user or
        sender.user.epoch~=s.epoch or sender:GetScreen()~=s.private then return nil end
    local index=sender.user.index;
    return index and s.rows[index+1]==sender and index>=0 and index<s.count;
end
function WFP_Command(sender)
    local s=WFP_Menu;
    if WFP_OwnRow(sender,s) and not s.committing and UI:IsScreenActive(s.private)==1 then
        WFP_Bridge:Queue(s.serial,1,sender.user.index,0,sender);
    end
    return 1;
end
function WFP_Key(sender,key)
    local s=WFP_Menu;
    if WFP_OwnRow(sender,s) and not s.committing and UI:IsScreenActive(s.private)==1 then
        if WFP_Bridge:Queue(s.serial,2,sender.user.index,key,sender) then return 0 end
    end
    return 1;
end
function WFP_Focus(sender)
    local s=WFP_Menu;
    if not WFP_OwnRow(sender,s) then return 1 end
    DefOnGotFocusSideButton(sender);
    if WFP_OwnRow(sender,s) and not s.committing then
        s.selected=sender.user.index;
        WFP_Bridge:Queue(s.serial,3,s.selected,0,sender);
    end
    return 1;
end
function WFP_Hover(sender)
    local s=WFP_Menu;
    if not WFP_OwnRow(sender,s) then return 1 end
    DefOnMouseEnterSideButton(sender);
    if WFP_OwnRow(sender,s) and not s.committing then
        WFP_Bridge:Queue(s.serial,4,sender.user.index,0,sender);
    end
    return 1;
end
function WFP_Released(sender)
    local s=WFP_Menu;
    local owned=s and (sender==s.private or sender==s.entry);
    if s and not owned then
        for i=1,7 do
            if sender==s.rows[i] then owned=1 end
        end
    end
    if owned and not s.dead then
        s.dead=1;
        g_WFPMenuStatus="retired";
        WFP_Bridge:Queue(s.epoch,5,-1,0,sender);
    end
    return 1;
end
function WFP_Entry(sender)
    local s=WFP_Menu;
    if WFP_Identity(s) and sender==s.entry and sender:GetScreen()==s.source and
        not s.committing and UI:IsScreenActive(s.source)==1 then
        WFP_Bridge:Queue(s.epoch,0,-1,0,sender);
    end
    return 1;
end
function WFP_Rect(widget)
    if not widget or not widget.GetRect then return nil end
    local rect=widget:GetRect();
    return {UI:ExtractLeft(rect),UI:ExtractTop(rect),UI:ExtractWidth(rect),UI:ExtractHeight(rect)};
end
function WFP_RectMatches(bounds,expected)
    if not bounds then return nil end
    -- Native button layout can round the requested rectangle.
    for i=1,4 do
        if type(bounds[i])~="number" or not (bounds[i]>=expected[i]-1 and bounds[i]<=expected[i]+1) then return nil end
    end
    return bounds[3]>0 and bounds[4]>0;
end
function WFP_Constructing(s)
    return s and WFP_Menu==s and UI==s.ui and not s.dead and
        WFP_FindScreen("Options")==s.source and
        (not s.private or WFP_FindScreen("WFPAdvancedOptions")==s.private);
end
function WFP_Prepare(epoch,title,entrySize)
    g_WFPMenuStatus="source-not-ready";
    if not UI or not UI.GetScreenCount or not UI.CreateScreenFromTable or
        not UI.CreateObjectFromTable or not UI.skins or not UI.skins.SideMenuButton or
        not UI.skins.MenuStatic or not GotoPage or not DefOnGotFocusSideButton or
        not DefOnMouseEnterSideButton or not UIFLAG_DEFAULT or not UIFLAG_CANHAVEFOCUS or
        not UISTYLE_TRANSPARENT or not UIALIGN_LEFT or not UIFLAG_VISIBLE then
        g_WFPMenuStatus="source-not-ready:ui-api-or-skin-missing";
        if UI then
            for i,key in {"GetScreenCount","CreateScreenFromTable","CreateObjectFromTable"} do
                if not UI[key] then g_WFPMenuStatus="source-not-ready:missing-"..key end
            end
        end
        return nil;
    end
    local source=WFP_FindScreen("Options");
    g_WFPMenuStatus="source-not-ready:Options-missing";
    if not source then return nil end
    local prototype=WFP_FindWidget(source,"MainMenu");
    g_WFPMenuStatus="source-not-ready:MainMenu-missing";
    if not source or not prototype or not prototype.SetText or not prototype.GetFontSize or
        WFP_FindScreen("WFPAdvancedOptions") or WFP_FindWidget(source,"WFPAdvancedEntry") then return nil end
    local rect=WFP_Rect(prototype);
    g_WFPMenuStatus="source-not-ready:MainMenu-geometry";
    if not rect or rect[1]<0 or rect[2]<0 or rect[3]<64 or rect[4]<16 or
        rect[1]+rect[3]>800 or rect[2]+2*rect[4]>580 or
        entrySize<8 or entrySize>20 then return nil end
    WFP_Menu={ui=UI,source=source,epoch=epoch,serial=epoch,rows={},count=0,selected=0,committing=1};
    local page={OnRelease=WFP_Released};
    page.WFPHeading={skin=UI.skins.MenuStatic,left=220,top=152,width=540,height=32,
        fontname="default",fonteffect="default",fontsize=24,halign=UIALIGN_LEFT,style=UISTYLE_TRANSPARENT,bordersize=0,
        flags=UIFLAG_DEFAULT-UIFLAG_CANHAVEFOCUS,text=""};
    page.WFPDescription={skin=UI.skins.MenuStatic,left=220,top=470,width=540,height=56,
        fontname="default",fonteffect="default",fontsize=14,halign=UIALIGN_LEFT,style=UISTYLE_TRANSPARENT,bordersize=0,
        flags=UIFLAG_DEFAULT-UIFLAG_CANHAVEFOCUS,text=""};
    for i=0,6 do
        page["WFPRow"..tostring(i)]={skin=UI.skins.SideMenuButton,left=220,top=200+i*36,
            width=540,height=32,fontname="default",fonteffect="default",fontsize=20,halign=UIALIGN_LEFT,tabstop=200+i,
            user={epoch=epoch,index=i},text="",OnCommand=WFP_Command,OnKeyDown=WFP_Key,
            OnGotFocus=WFP_Focus,OnMouseEnter=WFP_Hover,OnRelease=WFP_Released};
    end
    local s=WFP_Menu;
    UI:CreateScreenFromTable("WFPAdvancedOptions",page);
    s.private=WFP_FindScreen("WFPAdvancedOptions");
    g_WFPMenuStatus="private-screen-refused";
    if not WFP_Constructing(s) or not s.private then return nil end
    s.heading=WFP_FindWidget(s.private,"WFPHeading");
    if not WFP_Constructing(s) then return nil end
    s.description=WFP_FindWidget(s.private,"WFPDescription");
    if not WFP_Constructing(s) then return nil end
    g_WFPMenuStatus="private-static-controls-refused";
    if not s.heading or not s.description or not s.heading.SetText or not s.description.SetText then return nil end
    for i,item in {{s.heading,24},{s.description,14}} do
        local widget=item[1];
        if not WFP_Constructing(s) or not widget.GetFontName or not widget.GetFontEffect or not widget.GetFontSize then return nil end
        local fontName=widget:GetFontName();
        if fontName~="default" or not WFP_Constructing(s) then g_WFPMenuStatus="private-static-font:"..tostring(fontName); return nil end
        local fontEffect=widget:GetFontEffect();
        if fontEffect~="default" or not WFP_Constructing(s) then g_WFPMenuStatus="private-static-effect:"..tostring(fontEffect); return nil end
        local fontSize=widget:GetFontSize();
        if fontSize~=item[2] or not WFP_Constructing(s) then g_WFPMenuStatus="private-static-size:"..tostring(fontSize); return nil end
    end
    for i=0,6 do
        if not WFP_Constructing(s) then return nil end
        s.rows[i+1]=WFP_FindWidget(s.private,"WFPRow"..tostring(i));
        local row=s.rows[i+1];
        g_WFPMenuStatus="private-row-contract:"..tostring(i);
        if not row then g_WFPMenuStatus="private-row-missing:"..tostring(i); return nil end
        for _,key in {"SetText","SetFontSize","GetText","GetFontSize","GetFontName","GetFontEffect","GetScreen"} do
            if not row[key] then g_WFPMenuStatus="private-row-api:"..key; return nil end
        end
        if row:GetScreen()~=s.private then g_WFPMenuStatus="private-row-owner:"..tostring(i); return nil end
        if not WFP_Constructing(s) then return nil end
        local fontName=row:GetFontName();
        if fontName~="default" or not WFP_Constructing(s) then g_WFPMenuStatus="private-row-font:"..tostring(fontName); return nil end
        local fontEffect=row:GetFontEffect();
        if fontEffect~="default" or not WFP_Constructing(s) then g_WFPMenuStatus="private-row-effect:"..tostring(fontEffect); return nil end
        local fontSize=row:GetFontSize();
        if fontSize~=20 or not WFP_Constructing(s) then g_WFPMenuStatus="private-row-size:"..tostring(fontSize); return nil end
        if not WFP_Constructing(s) then return nil end
        local bounds=WFP_Rect(row);
        if not WFP_Constructing(s) then return nil end
        if not bounds then g_WFPMenuStatus="private-row-rect-missing"; return nil end
        if not WFP_RectMatches(bounds,{220,200+i*36,540,32}) then
            g_WFPMenuStatus="private-row-rect:"..tostring(bounds[1])..","..tostring(bounds[2])..","..tostring(bounds[3])..","..tostring(bounds[4]); return nil
        end
        UI:HideWidget(row);
        if not WFP_Constructing(s) then return nil end
    end
    UI:DeactivateScreen(s.private);
    if not WFP_Constructing(s) then return nil end
    g_WFPMenuStatus="entry-controls-refused";
    local createdEntry=UI:CreateObjectFromTable("WFPAdvancedEntry",{
        skin=UI.skins.SideMenuButton,left=rect[1],top=rect[2]+rect[4],width=rect[3],height=rect[4],
        fontname="default",fonteffect="default",fontsize=entrySize,text=title,
        flags=UIFLAG_DEFAULT-UIFLAG_VISIBLE,tabstop=10002,user={epoch=epoch},
        OnCommand=WFP_Entry,OnRelease=WFP_Released},source);
    s.entry=WFP_FindWidget(source,"WFPAdvancedEntry") or createdEntry;
    if not WFP_Constructing(s) then return nil end
    if not s.entry then g_WFPMenuStatus="entry-missing"; return nil end
    for _,key in {"GetFontName","GetFontEffect","GetFontSize","GetScreen","GetText"} do
        if not s.entry[key] then g_WFPMenuStatus="entry-api:"..key; return nil end
    end
    if s.entry:GetScreen()~=source or not WFP_Constructing(s) then g_WFPMenuStatus="entry-owner"; return nil end
    if WFP_FindWidget(source,"WFPAdvancedEntry")~=s.entry then g_WFPMenuStatus="entry-name"; return nil end
    local fontName=s.entry:GetFontName();
    if not WFP_Constructing(s) or fontName~="default" then g_WFPMenuStatus="entry-font:"..tostring(fontName); return nil end
    local fontEffect=s.entry:GetFontEffect();
    if not WFP_Constructing(s) or fontEffect~="default" then g_WFPMenuStatus="entry-effect:"..tostring(fontEffect); return nil end
    local fontSize=s.entry:GetFontSize();
    if not WFP_Constructing(s) or fontSize~=entrySize then g_WFPMenuStatus="entry-size:"..tostring(fontSize); return nil end
    if not WFP_Constructing(s) then return nil end
    UI:HideWidget(s.entry);
    if not WFP_Constructing(s) then return nil end
    local bounds=WFP_Rect(s.entry);
    if not WFP_Constructing(s) then return nil end
    if not WFP_RectMatches(bounds,{rect[1],rect[2]+rect[4],rect[3],rect[4]}) then g_WFPMenuStatus="entry-rect"; return nil end
    if s.entry:GetText()~=title then g_WFPMenuStatus="entry-text"; return nil end
    if not WFP_Bridge:Bind(epoch,source,s.private,s.entry,s.heading,s.description,prototype,
        s.rows[1],s.rows[2],s.rows[3],s.rows[4],s.rows[5],s.rows[6],s.rows[7]) then return nil end
    if not WFP_Constructing(s) then return nil end
    s.committing=nil;
    UI:ShowWidget(s.entry);
    if not WFP_Constructing(s) then return nil end
    g_WFPMenuStatus="ready";
    return 1;
end
function WFP_Snapshot()
    g_WFPMenuStatus="retired";
    g_WFPMenuSelected="-1";
    g_WFPMenuPause="0";
    local s=WFP_Menu;
    if not WFP_Identity(s) then return nil end
    if ClientStuff then g_WFPMenuPause="1" end
    if UI:IsScreenActive(s.private)==1 then
        g_WFPMenuStatus="private";
        local focus=UI:GetFocus();
        for i=0,s.count-1 do
            if focus==s.rows[i+1] then g_WFPMenuSelected=tostring(i) end
        end
    elseif UI:IsScreenActive(s.source)==1 then
        g_WFPMenuStatus="options";
        if UI:GetFocus()==s.entry then g_WFPMenuSelected="entry" end
    else g_WFPMenuStatus="covered-or-left" end
    return 1;
end
function WFP_Commit(serial,title,items,selected)
    g_WFPMenuStatus="commit-refused";
    if not WFP_Identity() or not items or getn(items)<1 or getn(items)>7 then return nil end
    local s=WFP_Menu;
    s.committing=1;
    s.serial=serial;
    s.count=getn(items);
    s.description:SetText("");
    if not WFP_Identity(s) then return nil end
    s.heading:SetText(title);
    if not WFP_Identity(s) then return nil end
    for i=0,6 do
        if not WFP_Identity(s) then return nil end
        local row=s.rows[i+1];
        local item=items[i+1];
        if item then
            row:SetText(item.text);
            if not WFP_Identity(s) then return nil end
            row:SetFontSize(item.size);
            if not WFP_Identity(s) then return nil end
            if row:GetText()~=item.text then return nil end
            if not WFP_Identity(s) then return nil end
            if row:GetFontSize()~=item.size then return nil end
            if not WFP_Identity(s) then return nil end
            if item.enabled==1 then UI:EnableWidget(row) else UI:DisableWidget(row) end
            if not WFP_Identity(s) then return nil end
            UI:ShowWidget(row);
        else UI:HideWidget(row) end
        if not WFP_Identity(s) then return nil end
    end
    if not WFP_Identity(s) or selected<0 or selected>=s.count then return nil end
    s.selected=selected;
    s.committing=nil;
    g_WFPMenuStatus="committed";
    return 1;
end
function WFP_Enter()
    local s=WFP_Menu;
    if not WFP_Identity(s) then return nil end
    if UI:IsScreenActive(s.source)~=1 or UI:GetFocus()~=s.entry then return nil end
    s.parentTab="GameOptions";
    for i,name in {"GameOptions","VideoOptions","SoundOptions","ControlOptions"} do
        if UI:IsScreenActive(name)==1 then s.parentTab=name end
    end
    GotoPage("WFPAdvancedOptions");
    if not WFP_Identity(s) or UI:IsScreenActive(s.private)~=1 then
        return nil;
    end
    UI:SetFocus(s.rows[s.selected+1]);
    if not WFP_Identity(s) then return nil end
    return 1;
end
function WFP_Select(index)
    local s=WFP_Menu;
    if not WFP_Identity(s) then return nil end
    if index<0 or index>=s.count or UI:IsScreenActive(s.private)~=1 then return nil end
    UI:SetFocus(s.rows[index+1]);
    if not WFP_Identity(s) then return nil end
    return 1;
end
function WFP_Describe(text,size)
    local s=WFP_Menu;
    if WFP_Identity(s) and UI:IsScreenActive(s.private)==1 then
        if not size or size<8 or size>14 then return nil end
        s.description:SetText(text);
        if not WFP_Identity(s) then return nil end
        s.description:SetFontSize(size);
        if not WFP_Identity(s) then return nil end
        local actual=s.description:GetFontSize();
        if not WFP_Identity(s) or actual~=size then return nil end
        return 1;
    end
    return nil;
end
function WFP_Retire(epoch)
    local s=WFP_Menu;
    if not s or s.epoch~=epoch or UI~=s.ui then return nil end
    s.dead=1;
    s.committing=1;
    if WFP_FindScreen("Options")~=s.source then return 1 end
    if s.entry and WFP_FindWidget(s.source,"WFPAdvancedEntry")==s.entry then UI:HideWidget(s.entry) end
    if WFP_Menu~=s or UI~=s.ui then return nil end
    if s.private and WFP_FindScreen("WFPAdvancedOptions")==s.private and UI:IsScreenActive(s.private)==1 then
        GotoPage("Options");
        if WFP_Menu~=s or UI~=s.ui or WFP_FindScreen("Options")~=s.source then return nil end
        local button=WFP_FindWidget(s.source,s.parentTab or "GameOptions");
        if button and button.OnCommand then button.OnCommand(button) end
        if WFP_Menu~=s or UI~=s.ui then return nil end
        local back=WFP_FindWidget(s.source,"MainMenu");
        if back then UI:SetFocus(back) end
    end
    return 1;
end
function WFP_Leave()
    local s=WFP_Menu;
    if not WFP_Identity(s) then return nil end
    if UI:IsScreenActive(s.private)~=1 then return nil end
    s.committing=1;
    GotoPage("Options");
    if not WFP_Identity(s) then return nil end
    local button=WFP_FindWidget(s.source,s.parentTab);
    if button and button.OnCommand then button.OnCommand(button) end
    if not WFP_Identity(s) then return nil end
    UI:SetFocus(s.entry);
    if not WFP_Identity(s) then return nil end
    s.committing=nil;
    return 1;
end
)WFP";
}

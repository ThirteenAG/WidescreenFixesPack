#pragma once
// Native function signatures used by the Plugins page.
namespace CTMenu
{
    inline void Init()
    {
        if (ready) return;
        struct Rollback
        {
            ~Rollback()
            {
                if (ready) return;
                titleBuildHook.reset(); pauseBuildHook.reset(); acceptHook.reset(); cancelHook.reset();
                keyHook.reset(); displayHook.reset(); popHook.reset(); destructorHook.reset(); stackDestructorHook.reset(); pushHook.reset();
            }
        } rollback;
        std::map<std::string, void*> found;
        auto MenuMouseMove = hook::pattern("8B 51 04 85 D2 56 8B 74 24 08 8B 46 04 89 41 16 74 0E 8B 49 08 8B C1 2B C2 C1 F8 03 85 C0 75 06 32 C0 5E C2 04 00 8B 49 F8 56 E8 41 E4 00 00 5E");
        if (MenuMouseMove.size() != 1) { LogOnce("unsupported native signature: MenuMouseMove"); return; }
        moveMouse = reinterpret_cast<decltype(moveMouse)>(MenuMouseMove.get_first());
        auto NativeLanguageCaller = hook::pattern("53 55 57 8B BC 24 24 02 00 00 85 FF 75 07 E8 3F AD 00 00 8B F8 8B 9C 24 20 02 00 00 8B AC 24 1C");
        if (NativeLanguageCaller.size() != 1) { LogOnce("unsupported native signature: NativeLanguageCaller"); return; }
        getLanguage = reinterpret_cast<decltype(getLanguage)>(injector::GetBranchDestination(NativeLanguageCaller.get_first(14)).as_int());
        auto StackPush = hook::pattern("83 EC 08 53 56 8B F1 8B 4E 04 85 C9 57 75 04 33 FF EB 20 8B 7E 08 2B F9 C1 FF 03 85 FF 7E 14 8D");
        if (StackPush.size() != 1) { LogOnce("unsupported native signature: StackPush"); return; }
        found["StackPush"] = StackPush.get_first();
        auto UserContextDtor = hook::pattern("56 8B F1 57 8B 7E 14 85 FF C7 06 ? ? ? ? 74 10 8B CF E8 98 00 00 00 57 E8 32 03 00 00 83 C4");
        if (UserContextDtor.size() != 1) { LogOnce("unsupported native signature: UserContextDtor"); return; }
        // The native context destroys its stack first, then its package pages.
        found["StackDtor"] = injector::GetBranchDestination(UserContextDtor.get_first(19)).as_int() ?
            reinterpret_cast<void*>(injector::GetBranchDestination(UserContextDtor.get_first(19)).as_int()) : nullptr;
        auto PageFactory = hook::pattern("8B 44 24 04 83 F8 04 77 7F FF 24 85 ? ? ? ? 6A 24 E8 B9 F7 FF FF 83 C4 04 85 C0 74 6A 8B C8");
        if (PageFactory.size() != 1) { LogOnce("unsupported native signature: PageFactory"); return; }
        found["PageFactory"] = PageFactory.get_first();
        auto ItemFactory = hook::pattern("53 8B 5C 24 08 83 FB 0C 56 57 8B F9 0F 87 D1 00 00 00 0F B6 83 ? ? ? ? FF 24 85 ? ? ? ?");
        if (ItemFactory.size() != 1) { LogOnce("unsupported native signature: ItemFactory"); return; }
        found["ItemFactory"] = ItemFactory.get_first();
        auto StyleBind = hook::pattern("83 EC 20 8B 44 24 24 53 56 8B F1 8B 4E 10 57 8B 7C 24 34 33 DB 85 C9 89 5C 24 0C 89 46 0C 89 7E");
        if (StyleBind.size() != 1) { LogOnce("unsupported native signature: StyleBind"); return; }
        found["StyleBind"] = StyleBind.get_first();
        auto Append = hook::pattern("8B 44 24 08 83 EC 3C 55 8B 6C 24 44 56 57 50 83 EC 1C 8B FC C7 47 18 07 00 00 00 C7 47 14 00 00");
        if (Append.size() != 1) { LogOnce("unsupported native signature: Append"); return; }
        found["Append"] = Append.get_first();
        auto Clear = hook::pattern("51 53 56 8B F1 8B 46 6C 57 8D 7E 68 33 DB 3B C3 74 1C 8B 4C 24 0C 8B 57 08 51 57 52 50 E8 AE DF");
        if (Clear.size() != 1) { LogOnce("unsupported native signature: Clear"); return; }
        found["Clear"] = Clear.get_first();
        auto Select = hook::pattern("51 53 55 56 8B F1 8B 46 6C 57 66 8B BE A8 00 00 00 66 85 FF 0F 94 C3 85 C0 74 18 8B 4E 70 2B C8");
        if (Select.size() != 1) { LogOnce("unsupported native signature: Select"); return; }
        found["Select"] = Select.get_first();
        auto AddChild = hook::pattern("83 C1 04 56 8B 71 04 85 F6 75 04 33 D2 EB 08 8B 51 08 2B D6 C1 FA 02 85 F6 74 23 8B 41 0C 2B C6 C1 F8 02 3B D0 73 17 8B 41 08 85 C0 74 06 8B 54 24 08 89 10 83 C0 04 89 41 08 5E C2 04 00 8B 41 08 8D 54 24 08 52 6A 01 50 E8 92 FB FF FF 5E C2 04 00 CC CC CC CC CC CC CC CC CC CC CC CC CC CC");
        if (AddChild.size() != 1) { LogOnce("unsupported native signature: AddChild"); return; }
        found["AddChild"] = AddChild.get_first();
        auto GetStack = hook::pattern("8B 41 0C 50 E8 17 6C 00 00 8B C8 E8 F0 71 00 00 C3 CC CC CC CC CC CC CC CC CC CC CC CC CC CC CC");
        if (GetStack.size() != 1) { LogOnce("unsupported native signature: GetStack"); return; }
        found["GetStack"] = GetStack.get_first();
        auto DisplayMenu = hook::pattern("51 53 8B 5C 24 0C 85 DB C7 44 24 04 00 00 00 00 75 07 32 C0 ? ? ? ? 00 8B 41 0C 56 50 E8 4D");
        if (DisplayMenu.size() != 1) { LogOnce("unsupported native signature: DisplayMenu"); return; }
        found["DisplayMenu"] = DisplayMenu.get_first();
        auto StackPop = hook::pattern("83 EC 0C 53 55 56 57 8B F1 C6 44 24 13 00 8B FF 8B 46 04 85 C0 75 04 33 C9 EB 08 8B 4E 08 2B C8");
        if (StackPop.size() != 1) { LogOnce("unsupported native signature: StackPop"); return; }
        found["StackPop"] = StackPop.get_first();
        auto ItemAccept = hook::pattern("F6 41 4C 02 74 05 E9 65 FC FF FF 32 C0 C3 CC CC F6 41 4C 02 74 05 E9 95 FC FF FF 32 C0 C3 CC CC");
        if (ItemAccept.size() != 1) { LogOnce("unsupported native signature: ItemAccept"); return; }
        found["ItemAccept"] = ItemAccept.get_first();
        auto ItemCancel = hook::pattern("F6 41 4C 02 74 05 E9 95 FC FF FF 32 C0 C3 CC CC 8B 44 24 08 8B 54 24 04 50 51 8B 4C 24 10 51 52");
        if (ItemCancel.size() != 1) { LogOnce("unsupported native signature: ItemCancel"); return; }
        found["ItemCancel"] = ItemCancel.get_first();
        auto KeyPress = hook::pattern("53 8B 5C 24 0C 56 57 8B 7C 24 10 8B F1 53 8B CF E8 8B 81 00 00 F6 86 B4 00 00 00 02 0F 84 36 01");
        if (KeyPress.size() != 1) { LogOnce("unsupported native signature: KeyPress"); return; }
        found["KeyPress"] = KeyPress.get_first();
        auto PageDtor = hook::pattern("53 56 8B F1 8B 46 0C 57 8B 7E 08 33 DB 3B F8 C7 06 ? ? ? ? 74 16 8B 0F 3B CB 74 06 8B 01 6A");
        if (PageDtor.size() != 1) { LogOnce("unsupported native signature: PageDtor"); return; }
        found["PageDtor"] = PageDtor.get_first();
        auto ResetFocus = hook::pattern("51 53 55 56 8B 74 24 14 32 DB 33 ED 57 89 6C 24 10 89 6E 24 8B 4E 08 85 C9 75 04 33 C0 EB 08 8B");
        if (ResetFocus.size() != 1) { LogOnce("unsupported native signature: ResetFocus"); return; }
        found["ResetFocus"] = ResetFocus.get_first();
        auto PauseBuilder = hook::pattern("56 6A 01 68 ? ? ? ? 68 ? ? ? ? 8B F1 8B 46 38 68 ? ? ? ? 68 ? ? ? ? 50 E8 1E B3");
        if (PauseBuilder.size() != 1) { LogOnce("unsupported native signature: PauseBuilder"); return; }
        found["PauseBuilder"] = PauseBuilder.get_first();
        auto Title72 = hook::pattern("83 EC 0C 53 55 56 57 6A 01 68 ? ? ? ? 68 ? ? ? ? 8B F1 8B 46 38 68 ? ? ? ? 68 ? ? ? ? 50 E8 08 96 01 00 6A 01 6A 00 6A 00 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? E8 FE 38");
        if (Title72.size() != 1) { LogOnce("unsupported native signature: Title72"); return; }
        found["Title72"] = Title72.get_first();
        auto ListCtor = hook::pattern("53 56 8B F1 8D 46 0C 89 46 04 C7 46 08 ? ? ? ? C7 06 ? ? ? ? C7 46 08 ? ? ? ? C7 00 ? ? ? ? 8B 0D ? ? ? ? 89 48 04 C7 00 ? ? ? ? 8B 15 ? ? ? ? 89 50 08 33 DB 8D 4E");
        if (ListCtor.size() != 1) { LogOnce("unsupported native signature: ListCtor"); return; }
        found["ListCtor"] = ListCtor.get_first();
        auto ListDtor = hook::pattern("51 53 56 8B F1 8B 46 7C 33 DB 3B C3 57 74 09 50 E8 6B D5 DA FF 83 C4 04 8D 7E 68 89 5E 7C 89 9E 80 00 00 00 89 9E 84 00 00 00 8B 47 04 3B C3 74 1C 8B 4C 24 0C 8B 57 08 51 57 52 50 E8 6F D9 FF");
        if (ListDtor.size() != 1) { LogOnce("unsupported native signature: ListDtor"); return; }
        found["ListDtor"] = ListDtor.get_first();
        auto StyleCtor = hook::pattern("56 8B F1 6A 00 6A 20 C7 06 ? ? ? ? C7 46 08 00 00 00 00 C7 46 0C 00 00 00 00 E8 F0 4D DB FF 83 C4 08 85 C0 74 17 8B C8 E8 72 D0 03 00 6A 00 8B CE 89 46 10 E8 16 C0 00 00 8B C6 5E C3 33 C0");
        if (StyleCtor.size() != 1) { LogOnce("unsupported native signature: StyleCtor"); return; }
        found["StyleCtor"] = StyleCtor.get_first();
        auto NativeMemory = hook::pattern("8B 44 24 08 85 C0 75 05 B8 ? ? ? ? 8B 0D ? ? ? ? 8B 11 50 8B 44 24 08 50 FF 12 C3 CC CC 8B 0D ? ? ? ? 8B 54");
        if (NativeMemory.size() != 1)
        { LogOnce("native template allocator signature unavailable"); return; }
        CTLayout::allocate = reinterpret_cast<decltype(CTLayout::allocate)>(NativeMemory.get_first());
        CTLayout::release = reinterpret_cast<decltype(CTLayout::release)>(NativeMemory.get_first(32));
        makePage = reinterpret_cast<decltype(makePage)>(found["PageFactory"]);
        makeItem = reinterpret_cast<decltype(makeItem)>(found["ItemFactory"]);
        bindStyle = reinterpret_cast<decltype(bindStyle)>(found["StyleBind"]);
        append = reinterpret_cast<decltype(append)>(found["Append"]);
        clear = reinterpret_cast<decltype(clear)>(found["Clear"]);
        select = reinterpret_cast<decltype(select)>(found["Select"]);
        addChild = reinterpret_cast<decltype(addChild)>(found["AddChild"]);
        getStack = reinterpret_cast<decltype(getStack)>(found["GetStack"]);
        resetFocus = reinterpret_cast<decltype(resetFocus)>(found["ResetFocus"]);
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&module), &module) || !module)
        { LogOnce("native host module unavailable"); return; }
        titleBuildHook = safetyhook::create_inline(found["Title72"], TitleBuild);
        if (!titleBuildHook) { LogOnce("native hook failed: Title72"); return; }
        pauseBuildHook = safetyhook::create_inline(found["PauseBuilder"], PauseBuild);
        if (!pauseBuildHook) { LogOnce("native hook failed: PauseBuilder"); return; }
        acceptHook = safetyhook::create_inline(found["ItemAccept"], Accept);
        if (!acceptHook) { LogOnce("native hook failed: ItemAccept"); return; }
        cancelHook = safetyhook::create_inline(found["ItemCancel"], Cancel);
        if (!cancelHook) { LogOnce("native hook failed: ItemCancel"); return; }
        keyHook = safetyhook::create_inline(found["KeyPress"], Key);
        if (!keyHook) { LogOnce("native hook failed: KeyPress"); return; }
        displayHook = safetyhook::create_inline(found["DisplayMenu"], Display);
        if (!displayHook) { LogOnce("native hook failed: DisplayMenu"); return; }
        popHook = safetyhook::create_inline(found["StackPop"], Pop);
        if (!popHook) { LogOnce("native hook failed: StackPop"); return; }
        destructorHook = safetyhook::create_inline(found["PageDtor"], PageDestroyed);
        if (!destructorHook) { LogOnce("native hook failed: PageDtor"); return; }
        stackDestructorHook = safetyhook::create_inline(found["StackDtor"], StackDestroyed);
        if (!stackDestructorHook) { LogOnce("native hook failed: StackDtor"); return; }
        pushHook = safetyhook::create_inline(found["StackPush"], Push);
        if (!pushHook) { LogOnce("native hook failed: StackPush"); return; }
        NativeMenuTick = nullptr;
        NativeWindowsMenuMouse = WindowsMenuMouse;
        NativeWindowsMenuHelp = WindowsMenuHelp;
        ready = true;
        CTSettings::makeItem = makeItem;
        CTSettings::bindStyle = bindStyle;
        CTSettings::addChild = addChild;
        CTSettings::getStack = getStack;
        CTSettings::caption = SettingsCaption;
        CTSettings::stockFits = SettingsTextFits;
        CTSettings::current = SettingsCurrent;
        CTSettings::open = OpenSettings;
        CTSettings::retire = RetireSettings;
        CTSettings::frame = Tick;
        CTSettings::log = LogOnce;
        if (!CTSettings::Init()) LogOnce("native Settings Plugins bridge unavailable");
        else
        {
            CTSettings::controllerRetired = [](void* controller) { CTControls::ControllerRetired(controller); CTDisplay::ControllerRetired(controller); };
            CTControls::module = module;
            CTControls::language = Language;
            CTControls::wide = Wide;
            CTControls::metrics = RowMetrics;
            CTControls::select = select;
            CTControls::log = LogOnce;
            if (!CTControls::Init()) LogOnce("native joined Controls bridge unavailable");
            CTDisplay::module = module;
            CTDisplay::language = Language;
            CTDisplay::wide = Wide;
            CTDisplay::metrics = PrototypeMetrics;
            CTDisplay::append = append;
            CTDisplay::select = select;
            if (!CTDisplay::Init()) LogOnce("native joined Display bridge unavailable");
        }
    }
}

class sc5.custom.containers.SettingsPage extends sc5.framework.containers.Page
{
   var activated;
   var addChild;
   var onPageChanged;
   var pageRoot;
   var menu = null;
   var hasCalibration = false;
   var fixIndex = -1;
   var fixPage = null;
   function SettingsPage(hasCalibration)
   {
      super(sc5.custom.containers.MainMenuFrame.PAGE_SETTINGS);
      this.hasCalibration = hasCalibration;
   }
   function activate()
   {
      super.activate();
      _root.FlashMenu.sendCommand("FixMenuEntry","",this);
   }
   function openFixNotice(title, text)
   {
      if(this.fixPage == null) { return false; }
      this.onPageChanged("FixAdvancedOptions",true);
      this.fixPage.showFixDetails(title,text);
      return true;
   }
   function deactivate()
   {
      super.deactivate();
   }
   function getTips()
   {
      var _loc3_ = super.getTips();
      if(_loc3_ == null)
      {
         _loc3_ = new sc5.framework.collections.Tips();
      }
      if(this.activated)
      {
         _loc3_.addItem(sc5.framework.util.TipFactory.CreateTip(sc5.framework.util.Tip.TIP_A,sc5.framework.util.Tip.TIP_ACTION_SELECT));
         _loc3_.addItem(sc5.framework.util.TipFactory.CreateTip(sc5.framework.util.Tip.TIP_B,sc5.framework.util.Tip.TIP_ACTION_BACK));
      }
      return _loc3_;
   }
   function handleInput(event)
   {
      if(event.getAction() == sc5.framework.input.InputAction.PRESS && event.getKey() == sc5.framework.input.InputKey.JOY_A)
      {
         if(this.menu.getSelectedIndex() == this.fixIndex && this.fixPage != null)
         {
            this.onPageChanged("FixAdvancedOptions",true);
            return true;
         }
         _root.FlashMenu.playSound("Play_random_SFX_Menu_SA");
         switch(this.menu.getSelectedIndex())
         {
            case 0:
               this.onPageChanged(sc5.custom.containers.MainMenuFrame.PAGE_GAME,true);
               break;
            case 1:
               this.onPageChanged(sc5.custom.containers.MainMenuFrame.PAGE_AUDIO,true);
               break;
            case 2:
               this.onPageChanged(sc5.custom.containers.MainMenuFrame.PAGE_CONTROLS,true);
               break;
            case 3:
               this.onPageChanged(sc5.custom.containers.MainMenuFrame.PAGE_VIDEO,true);
         }
         return true;
      }
      if(event.getAction() == sc5.framework.input.InputAction.PRESS && event.getKey() == sc5.framework.input.InputKey.JOY_B)
      {
         _root.FlashMenu.playSound("Play_random_SFX_Menu_SB");
         this.onPageChanged(sc5.framework.containers.Frame.PAGE_LASTPAGE,false);
         this.menu.selectFirstEnabledItem();
         return true;
      }
      return super.handleInput(event);
   }
   function resetPage()
   {
      this.menu.selectFirstEnabledItem();
   }
   function bind(movies)
   {
      super.bind(movies);
      this.menu = new sc5.framework.containers.ListBox();
      this.registerEventListeners(this.menu);
      this.addChild(this.menu);
      movies.clear();
      movies.addItem(this.pageRoot.Menu);
      this.menu.bind(movies);
      this.pageRoot.Menu.SubMenu.hitTestDisable = true;
      this.pageRoot.Menu.SubMenu._visible = false;
      var _loc5_;
      if(_root.FlashMenu.multiplayer && !_root.FlashMenu.isoneplayer)
      {
         this.menu.addLabels(2,sc5.framework.containers.ListBox.DEFAULTLISTSIZE,"Menu","MenuItem",true);
         _loc5_ = new sc5.framework.util.LocalizationInfo("SETTINGSPAGE","Localization\\Menus",new Array("GameSettings","AudioSettings"));
         this.menu.addLocalizationInfo(_loc5_);
      }
      else
      {
         this.menu.addLabels(4,sc5.framework.containers.ListBox.DEFAULTLISTSIZE,"Menu","MenuItem",true);
         _loc5_ = new sc5.framework.util.LocalizationInfo("SETTINGSPAGE","Localization\\Menus",new Array("GameSettings","AudioSettings","ControlsSettings","VideoSettings"));
         this.menu.addLocalizationInfo(_loc5_);
      }
   }

   // A separate native Page and cloned native artwork; stock controls stay owned by SettingsPage.
   function fixMenuEntry(label)
   {
      if(this.fixPage == null)
      {
         var owner = this;
         var frame = _root.FlashMenu.frame;
         var page = new sc5.framework.containers.Page("FixAdvancedOptions");
         var clip = this.pageRoot;
         var nativeHitTest = clip.hitTestDisable;
         var privateMenu = clip.Menu.duplicateMovieClip("FixPluginMenu",clip.getNextHighestDepth());
         if(privateMenu == undefined || privateMenu.Menu0 == undefined || privateMenu.Menu1 == undefined)
         {
            if(privateMenu != undefined) { privateMenu.removeMovieClip(); }
            return false;
         }
         page.localizations.clear();
         var pageMovies = new sc5.framework.collections.Movies();
         pageMovies.addItem(clip);
         page.bind(pageMovies);
         clip.hitTestDisable = nativeHitTest;
         page.privateMenu = privateMenu;
         page.menu = new sc5.framework.containers.ListBox();
         var movies = new sc5.framework.collections.Movies();
         movies.addItem(privateMenu);
         page.menu.bind(movies);
         page.addChild(page.menu);
         page.ownerSettings = this;
         page.fixLabels = new Array();
         page.fixValues = new Array();
         page.fixRendered = new Array();
         page.fixEnabled = new Array();
         page.fixReady = false;
         var spacing = privateMenu.Menu1._y - privateMenu.Menu0._y;
         var firstY = privateMenu.Menu0._y;
         var nativeField = privateMenu.Menu0.MenuItem.txtField;
         var nativeWidth = nativeField._width;
         if(nativeField == undefined || !isFinite(nativeWidth) || nativeWidth < 64 || !isFinite(spacing) || spacing <= 0)
         {
            privateMenu.removeMovieClip();
            return false;
         }
         var i = 0;
         while(i < 7)
         {
            if(privateMenu["Menu"+i] == undefined)
            {
               privateMenu.Menu0.duplicateMovieClip("Menu"+i,privateMenu.getNextHighestDepth());
            }
            privateMenu["Menu"+i]._y = firstY + spacing*i;
            privateMenu["Menu"+i]._visible = false;
            i++;
         }
         privateMenu.SubMenu._visible = false;
         privateMenu.SubMenu.hitTestDisable = true;
         page.fixDescription = privateMenu.Menu0.duplicateMovieClip("FixDescription",privateMenu.getNextHighestDepth());
         page.fixDescription._y = firstY + spacing*8;
         page.fixDescription._visible = false;
         page.fixDescription.hitTestDisable = true;
         page.fixDescription._alpha = 100;
         page.fixDescription.Background._visible = false;
         page.activate = function()
         {
            sc5.framework.containers.Page.prototype.activate.call(this);
            this.pageRoot._visible = true;
            this.pageRoot._alpha = 100;
            this.menu.show();
            _root.FlashMenu.sendCommand("FixMenuOpen",this.ownerSettings.hasCalibration ? "title" : "pause",this);
         };
         page.deactivate = function()
         {
            _root.FlashMenu.sendCommand("FixMenuClose","",this);
            this.fixReady = false;
            this.privateMenu._visible = false;
            sc5.framework.containers.Page.prototype.deactivate.call(this);
         };
         page.destroy = function()
         {
            _root.FlashMenu.sendCommand("FixMenuClose","",this);
            this.fixReady = false;
            delete this.privateMenu.onEnterFrame;
            this.privateMenu.removeMovieClip();
            sc5.framework.containers.Page.prototype.destroy.call(this);
         };
         page.getTips = function()
         {
            var tips = new sc5.framework.collections.Tips();
            if(this.activated)
            {
               tips.addItem(sc5.framework.util.TipFactory.CreateTip(sc5.framework.util.Tip.TIP_A,sc5.framework.util.Tip.TIP_ACTION_SELECT));
               tips.addItem(sc5.framework.util.TipFactory.CreateTip(sc5.framework.util.Tip.TIP_B,sc5.framework.util.Tip.TIP_ACTION_BACK));
            }
            return tips;
         };
         page.tick = function(delta)
         {
            if(this.activated && this.fixReady)
            {
               _root.FlashMenu.sendCommand("FixMenuRefresh",String(this.menu.getSelectedIndex()),this);
               this.refreshFixLabels();
            }
         };
         page.resetFixMenu = function(title)
         {
            if(this.fixHelpVisibility != undefined)
            {
               var restoreIndex = 0;
               while(restoreIndex < this.fixHelpVisibility.length)
               {
                  this.privateMenu["Menu"+restoreIndex]._visible = this.fixHelpVisibility[restoreIndex];
                  restoreIndex++;
               }
               this.fixHelpVisibility = undefined;
            }
            this.fixDetailsMode = false;
            if(this.fixDetails != undefined) { this.fixDetails._visible = false; }
            this.fixReady = false;
            this.fixDescription.MenuItem.txtField.text = "";
            this.fixDescription._visible = false;
            this.title = title == null || title == undefined ? "" : String(title);
            if(this.title.indexOf("SPLINTER CELL CONVICTION") == 0) { this.title = "CONVICTION FUSIONFIX"; }
            this.fixLabels = new Array(); this.fixValues = new Array(); this.fixEnabled = new Array();
            return true;
         };
         page.appendFixMenu = function(label,value,enabled)
         {
            label = label == null || label == undefined ? "" : String(label);
            value = value == null || value == undefined ? "" : String(value);
            if(this.fixLabels.length < 7)
            {
               this.fixLabels.push(label); this.fixValues.push(value); this.fixEnabled.push(enabled);
               return true;
            }
            return false;
         };
         page.escapeFixText = function(value)
         {
            return value.split("&").join("&amp;").split("<").join("&lt;").split(">").join("&gt;");
         };
         page.fitFixText = function(value,format,width)
         {
            value = value == null || value == undefined ? "" : String(value);
            if(format.getTextExtent(value).textFieldWidth <= width) { return value; }
            var tail = "...";
            while(value.length > 0 && format.getTextExtent(value+tail).textFieldWidth > width)
            {
               value = value.substr(0,value.length-1);
               var code = value.charCodeAt(value.length-1);
               if(code >= 55296 && code <= 56319) { value = value.substr(0,value.length-1); }
            }
            return value+tail;
         };
         page.refreshFixLabels = function()
         {
            if(!this.fixReady || this.fixDetailsMode) { return; }
            var i = 0;
            while(i < this.fixRendered.length)
            {
               var label = this.menu.getChildAt(i);
               var appearance = label.getAppearance();
               var text = appearance.MenuItem;
               var field = text.txtField;
               if(field != undefined)
               {
                  // Up/RollOver may replace the timeline text clip after bind.
                  label.textfield = text;
                  field.html = false;
                  field.text = this.fixRendered[i];
                  field._visible = true;
                  field._alpha = 100;
                  text._visible = true;
                  text._alpha = 100;
                  appearance._visible = true;
                  appearance._alpha = 100;
                  label.setTextColor();
               }
               i++;
            }
         };
         page.finishFixMenu = function(selected)
         {
            this.menu.clear();
            this.menu.addLabels(this.fixLabels.length,7,"Menu","MenuItem",true);
            var rendered = new Array();
            this.fixRendered = new Array();
            var i = 0;
            while(i < this.fixLabels.length)
            {
               var label = this.menu.getChildAt(i);
               var field = label.getAppearance().MenuItem.txtField;
               var format = field.getTextFormat();
               var width = field._width;
               var value = this.fixValues[i];
               var suffix = value.length == 0 ? "" : ": "+value;
               var valueWidth = suffix.length == 0 ? 0 : format.getTextExtent(suffix).textFieldWidth;
               var labelWidth = width-valueWidth-4;
               if(!isFinite(width) || !isFinite(valueWidth) || !isFinite(labelWidth) || labelWidth < format.getTextExtent("...").textFieldWidth)
               {
                  return false;
               }
               var fitted = this.fitFixText(this.fixLabels[i],format,labelWidth);
               while(fitted.length > 0 && format.getTextExtent(fitted+suffix).textFieldWidth > width)
               {
                  fitted = fitted.substr(0,fitted.length-1);
                  var endCode = fitted.charCodeAt(fitted.length-1);
                  if(endCode >= 55296 && endCode <= 56319) { fitted = fitted.substr(0,fitted.length-1); }
               }
               var text = fitted+suffix;
               label.setText(this.escapeFixText(text));
               rendered.push(this.escapeFixText(text));
               this.fixRendered.push(text);
               if(!this.fixEnabled[i]) { label.disable(); }
               i++;
            }
            this.menu.addItems(rendered);
            this.menu.select(Math.max(0,Math.min(selected,this.fixLabels.length-1)));
            this.fixReady = true;
            this.refreshFixLabels();
            _root.FlashMenu.frame.refreshPageTitle();
            var background = _root.FlashMenu.frame.background;
            if(background != undefined && background.title_MC != undefined)
            {
               var titleField = background.title_MC.Title.txtField;
               this.title = this.fitFixText(this.title,titleField.getTextFormat(),titleField._width);
               background.setTitle(this.title);
               _root.FlashMenu.frame.refreshPageTitle();
            }
            _root.FlashMenu.sendCommand("SetPageTitle","",_root.FlashMenu.frame);
            _root.FlashMenu.sendCommand("FixMenuDescription",String(this.menu.getSelectedIndex()),this);
            return true;
         };
         page.describeFixMenu = function(description)
         {
            if(this.fixDetailsMode) { return true; }
            description = description == null || description == undefined ? "" : String(description);
            if(description.length > 36)
            {
               description = description.substr(0,36);
               var cut = description.lastIndexOf(" ");
               if(cut >= 18) { description = description.substr(0,cut); }
               var code = description.charCodeAt(description.length-1);
               if(code >= 55296 && code <= 56319) { description = description.substr(0,description.length-1); }
               description += "...";
            }
            var field = this.fixDescription.MenuItem.txtField;
            field.text = this.fitFixText(description,field.getTextFormat(),field._width);
            this.fixDescription._visible = description.length > 0;
            return true;
         };
         page.showFixDetails = function(title,description)
         {
            if(!this.fixDetailsMode)
            {
               this.fixHelpVisibility = new Array();
               var saveIndex = 0;
               while(saveIndex < 10)
               {
                  this.fixHelpVisibility.push(this.privateMenu["Menu"+saveIndex]._visible);
                  saveIndex++;
               }
            }
            this.fixDetailsMode = true;
            this.fixDescription._visible = false;
            var i = 0;
            while(i < 10) { this.privateMenu["Menu"+i]._visible = false; i++; }
            if(this.fixDetails == undefined)
            {
               this.fixDetails = this.privateMenu.Menu0.duplicateMovieClip("FixDetails",this.privateMenu.getNextHighestDepth());
               this.fixDetails.Background._visible = false;
               this.fixDetails.hitTestDisable = true;
            }
            this.fixDetails._y = firstY;
            this.fixDetails._visible = true;
            this.fixDetails._alpha = 100;
            var field = this.fixDetails.MenuItem.txtField;
            field.multiline = true;
            field.wordWrap = true;
            field.autoSize = "none";
            field._height = spacing*7;
            field.text = description;
            field.scroll = 1;
            this.title = title;
            _root.FlashMenu.frame.refreshPageTitle();
            var background = _root.FlashMenu.frame.background;
            if(background != undefined) { background.setTitle(title); }
            _root.FlashMenu.sendCommand("SetPageTitle","",_root.FlashMenu.frame);
            return true;
         };
         page.leaveFixMenu = function()
         {
            this.fixReady = false;
            this.onPageChanged(sc5.framework.containers.Frame.PAGE_LASTPAGE,false);
            return true;
         };
         page.handleInput = function(event)
         {
            var action = event.getAction(); var key = event.getKey();
            if(this.fixDetailsMode)
            {
               if(action == sc5.framework.input.InputAction.PRESS && key == sc5.framework.input.InputKey.JOY_B)
               {
                  _root.FlashMenu.sendCommand("FixMenuHelpBack","",this);
               }
               if(action == sc5.framework.input.InputAction.PRESS || action == sc5.framework.input.InputAction.HOLD)
               {
                  var field = this.fixDetails.MenuItem.txtField;
                  if(key == sc5.framework.input.InputKey.JOY_DOWN) { field.scroll = Math.min(field.maxscroll,field.scroll+1); }
                  if(key == sc5.framework.input.InputKey.JOY_UP) { field.scroll = Math.max(1,field.scroll-1); }
               }
               return true;
            }
            if(action == sc5.framework.input.InputAction.PRESS && key == sc5.framework.input.InputKey.JOY_Y)
            {
               _root.FlashMenu.sendCommand("FixMenuHelp",String(this.menu.getSelectedIndex()),this);
               return true;
            }
            if(!this.fixReady)
            {
               if(action == sc5.framework.input.InputAction.PRESS && key == sc5.framework.input.InputKey.JOY_B)
               {
                  _root.FlashMenu.sendCommand("FixMenuClose","",this);
                  this.leaveFixMenu();
               }
               return true;
            }
            var index = this.menu.getSelectedIndex();
            if(action == sc5.framework.input.InputAction.PRESS)
            {
               if(key == sc5.framework.input.InputKey.JOY_B)
               {
                  _root.FlashMenu.sendCommand("FixMenuBack",String(index),this); return true;
               }
               if(key == sc5.framework.input.InputKey.JOY_A)
               {
                  _root.FlashMenu.sendCommand("FixMenuAccept",String(index),this); return true;
               }
            }
            if(action == sc5.framework.input.InputAction.PRESS || action == sc5.framework.input.InputAction.HOLD)
            {
               if(key == sc5.framework.input.InputKey.JOY_LEFT || key == sc5.framework.input.InputKey.JOY_LB)
               {
                  _root.FlashMenu.sendCommand("FixMenuLeft",String(index),this); return true;
               }
               if(key == sc5.framework.input.InputKey.JOY_RIGHT || key == sc5.framework.input.InputKey.JOY_RB)
               {
                  _root.FlashMenu.sendCommand("FixMenuRight",String(index),this); return true;
               }
            }
            var handled = sc5.framework.containers.Page.prototype.handleInput.call(this,event);
            _root.FlashMenu.sendCommand("FixMenuDescription",String(this.menu.getSelectedIndex()),this);
            return handled;
         };
         var nativeHover = page.menu.onItemMouseRollOverHandler;
         page.menu.onItemMouseRollOverHandler = function(event)
         {
            nativeHover.call(this,event);
            _root.FlashMenu.sendCommand("FixMenuDescription",String(this.getSelectedIndex()),page);
         };
         frame.addChild(page,"C");
         // The movie advances while either native Settings page is open.
         page.privateMenu.onEnterFrame = function()
         {
            page.tick(0);
            if(owner.isActivated()) { _root.FlashMenu.sendCommand("FixMenuNotice","",owner); }
         };
         page.privateMenu._visible = false;
         this.fixPage = page;
         this.fixIndex = this.menu.getItemCount();
         var entryClip = this.pageRoot.Menu["Menu"+this.fixIndex];
         if(entryClip == undefined)
         {
            entryClip = this.pageRoot.Menu.Menu0.duplicateMovieClip("Menu"+this.fixIndex,this.pageRoot.Menu.getNextHighestDepth());
            entryClip._y = this.pageRoot.Menu.Menu0._y + spacing*this.fixIndex;
         }
         entryClip._visible = true;
         this.menu.addLabel("Menu"+this.fixIndex,"MenuItem",true);
         this.menu.capacity = this.menu.getItemCount();
      }
      var entryField = this.menu.getChildAt(this.fixIndex).getAppearance().MenuItem.txtField;
      var entryFitted = this.fixPage.fitFixText(label,entryField.getTextFormat(),entryField._width);
      this.menu.items[this.fixIndex] = this.fixPage.escapeFixText(entryFitted);
      this.menu.refreshLabelText();
      this.menu.select(this.menu.getSelectedIndex());
      return true;
   }
}

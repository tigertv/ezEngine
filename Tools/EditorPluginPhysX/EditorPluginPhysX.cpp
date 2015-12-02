#include <PCH.h>
#include <EditorPluginPhysX/EditorPluginPhysX.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <Foundation/Reflection/Reflection.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Actions/AssetActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <CoreUtils/Localization/TranslationLookup.h>

#include <PhysXCooking/PhysXCooking.h>
#include <EditorPluginPhysX/Actions/PhysXActions.h>

void OnLoadPlugin(bool bReloading)
{
  ezQtEditorApp::GetInstance()->RegisterPluginNameForSettings("EditorPluginPhysX");
  ezTranslatorFromFiles::AddTranslationFile("PhysXPlugin.txt");

  // Mesh Asset
  {
    // Menu Bar
    {
      ezActionMapManager::RegisterActionMap("CollisionMeshAssetMenuBar");
      ezProjectActions::MapActions("CollisionMeshAssetMenuBar");
      ezStandardMenus::MapActions("CollisionMeshAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
      ezDocumentActions::MapActions("CollisionMeshAssetMenuBar", "Menu.File", false);
      ezCommandHistoryActions::MapActions("CollisionMeshAssetMenuBar", "Menu.Edit");
    }

    // Tool Bar
    {
      ezActionMapManager::RegisterActionMap("CollisionMeshAssetToolBar");
      ezDocumentActions::MapActions("CollisionMeshAssetToolBar", "", true);
      ezCommandHistoryActions::MapActions("CollisionMeshAssetToolBar", "");
      ezAssetActions::MapActions("CollisionMeshAssetToolBar", true);
    }
  }

  // Scene
  {
    // Menu Bar
    {
      ezPhysXActions::RegisterActions();
      ezPhysXActions::MapMenuActions();
    }

    // Tool Bar
    {

    }
  }
}

void OnUnloadPlugin(bool bReloading)
{
  ezPhysXActions::UnregisterActions();
}

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin, "ezEditorPluginScene", "ezPhysXPlugin");

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(EZ_EDITORPLUGINPHYSX_DLL, ezEditorPluginPhysX);


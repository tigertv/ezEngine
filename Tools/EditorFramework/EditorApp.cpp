#include <PCH.h>
#include <EditorFramework/EditorApp.moc.h>
#include <EditorFramework/EditorGUI.moc.h>
#include <EditorFramework/Dialogs/DocumentList.moc.h>
#include <EditorFramework/EngineProcess/EngineProcessConnection.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/IO/JSONReader.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <ToolsFoundation/Reflection/ReflectedTypeManager.h>
#include <QMainWindow>
#include <QSettings>
#include <QTimer>
#include <qstylefactory.h>

ezEditorApp* ezEditorApp::s_pInstance = nullptr;

QMainWindow* ezEditorApp::GetMainWindow()
{
  return s_ContainerWindows[0];
}

void SetStyleSheet()
{
  QApplication::setStyle(QStyleFactory::create("fusion"));
  QPalette palette;

  palette.setColor(QPalette::WindowText, QColor(200, 200, 200, 255));
  palette.setColor(QPalette::Button, QColor(100, 100, 100, 255));
  palette.setColor(QPalette::Light, QColor(97, 97, 97, 255));
  palette.setColor(QPalette::Midlight, QColor(59, 59, 59, 255));
  palette.setColor(QPalette::Dark, QColor(37, 37, 37, 255));
  palette.setColor(QPalette::Mid, QColor(45, 45, 45, 255));
  palette.setColor(QPalette::Text, QColor(200, 200, 200, 255));
  palette.setColor(QPalette::BrightText, QColor(37, 37, 37, 255));
  palette.setColor(QPalette::ButtonText, QColor(200, 200, 200, 255));
  palette.setColor(QPalette::Base, QColor(42, 42, 42, 255));
  palette.setColor(QPalette::Window, QColor(68, 68, 68, 255));
  palette.setColor(QPalette::Shadow, QColor(0, 0, 0, 255));
  palette.setColor(QPalette::Highlight, QColor(103, 141, 178, 255));
  palette.setColor(QPalette::HighlightedText, QColor(255, 255, 255, 255));
  palette.setColor(QPalette::Link, QColor(0, 0, 238, 255));
  palette.setColor(QPalette::LinkVisited, QColor(82, 24, 139, 255));
  palette.setColor(QPalette::AlternateBase, QColor(46, 46, 46, 255));
  QBrush NoRoleBrush(QColor(0, 0, 0, 255), Qt::NoBrush);
  palette.setBrush(QPalette::NoRole, NoRoleBrush);
  palette.setColor(QPalette::ToolTipBase, QColor(255, 255, 220, 255));
  palette.setColor(QPalette::ToolTipText, QColor(0, 0, 0, 255));

  palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(128, 128, 128, 255));
  palette.setColor(QPalette::Disabled, QPalette::Button, QColor(80, 80, 80, 255));
  palette.setColor(QPalette::Disabled, QPalette::Text, QColor(105, 105, 105, 255));
  palette.setColor(QPalette::Disabled, QPalette::BrightText, QColor(255, 255, 255, 255));
  palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(128, 128, 128, 255));
  palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(86, 117, 148, 255));

  QApplication::setPalette(palette);
}

ezEditorApp::ezEditorApp() :
  s_RecentProjects(5),
  s_RecentDocuments(50)
{
  s_pInstance = this;

  s_sApplicationName = "ezEditor";
  s_sUserName = "DefaultUser";
  s_pQtApplication = nullptr;
  s_pEngineViewProcess = nullptr;

  m_pTimer = new QTimer(nullptr);

}

ezEditorApp::~ezEditorApp()
{
  delete m_pTimer;
  m_pTimer = nullptr;
  s_pInstance = nullptr;
}

void ezEditorApp::StartupEditor(const char* szAppName, const char* szUserName, int argc, char** argv)
{
  s_sApplicationName = ezCommandLineUtils::GetInstance()->GetStringOption("-appname", 0, szAppName);
  s_sUserName = szUserName;

  RegisterPluginNameForSettings("-Main-");

  s_pQtApplication = new QApplication(argc, argv);
  s_pEngineViewProcess = new ezEditorEngineProcessConnection;

  QCoreApplication::setOrganizationDomain("www.ezEngine.net");
  QCoreApplication::setOrganizationName("ezEngine Project");
  QCoreApplication::setApplicationName(GetApplicationName().GetData());
  QCoreApplication::setApplicationVersion("1.0.0");

  SetStyleSheet();

  s_ContainerWindows.PushBack(new ezContainerWindow());
  s_ContainerWindows[0]->show();

  ezDocumentManagerBase::s_Requests.AddEventHandler(ezMakeDelegate(&ezEditorApp::DocumentManagerRequestHandler, this));
  ezDocumentManagerBase::s_Events.AddEventHandler(ezMakeDelegate(&ezEditorApp::DocumentManagerEventHandler, this));
  ezDocumentBase::s_EventsAny.AddEventHandler(ezMakeDelegate(&ezEditorApp::DocumentEventHandler, this));
  ezEditorProject::s_Requests.AddEventHandler(ezMakeDelegate(&ezEditorApp::ProjectRequestHandler, this));
  ezEditorProject::s_Events.AddEventHandler(ezMakeDelegate(&ezEditorApp::ProjectEventHandler, this));
  ezEditorEngineProcessConnection::s_Events.AddEventHandler(ezMakeDelegate(&ezEditorApp::EngineProcessMsgHandler, this));

  ezStartup::StartupCore();

  ezStringBuilder sAppDir = ezOSFile::GetApplicationDirectory();
  sAppDir.AppendPath("../../../Shared/Tools", GetApplicationName().GetData());

  ezOSFile osf;
  osf.CreateDirectoryStructure(sAppDir);

  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
  ezFileSystem::AddDataDirectory("", ezFileSystem::AllowWrites, "App"); // for absolute paths
  ezFileSystem::AddDataDirectory(sAppDir.GetData(), ezFileSystem::AllowWrites, "App"); // for everything relative

  m_LogHTML.BeginLog("Log_Editor.htm", "ezEditor");

  ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLoggingEvent::Handler(&ezLogWriter::HTML::LogMessageHandler, &m_LogHTML));
 

  ezEditorGUI::GetInstance()->LoadState();

  LoadRecentFiles();

  LoadPlugins();

  s_ContainerWindows[0]->ShowSettingsTab();


  ezEditorEngineProcessConnection::GetInstance()->RestartProcess();
}

void ezEditorApp::ShutdownEditor()
{
  ezEditorProject::CloseProject();

  ezEditorEngineProcessConnection::s_Events.RemoveEventHandler(ezMakeDelegate(&ezEditorApp::EngineProcessMsgHandler, this));
  ezEditorProject::s_Requests.RemoveEventHandler(ezMakeDelegate(&ezEditorApp::ProjectRequestHandler, this));
  ezEditorProject::s_Events.RemoveEventHandler(ezMakeDelegate(&ezEditorApp::ProjectEventHandler, this));
  ezDocumentBase::s_EventsAny.RemoveEventHandler(ezMakeDelegate(&ezEditorApp::DocumentEventHandler, this));
  ezDocumentManagerBase::s_Requests.RemoveEventHandler(ezMakeDelegate(&ezEditorApp::DocumentManagerRequestHandler, this));
  ezDocumentManagerBase::s_Events.RemoveEventHandler(ezMakeDelegate(&ezEditorApp::DocumentManagerEventHandler, this));

  SaveSettings();

  ezEditorGUI::GetInstance()->SaveState();

  delete s_pEngineViewProcess;
  delete s_pQtApplication;

  m_LogHTML.EndLog();
}

ezInt32 ezEditorApp::RunEditor()
{
  connect(m_pTimer, SIGNAL(timeout()), this, SLOT(SlotTimedUpdate()), Qt::QueuedConnection);
  m_pTimer->start(1);

  return s_pQtApplication->exec();
}

void ezEditorApp::SlotTimedUpdate()
{
  if (ezEditorEngineProcessConnection::GetInstance())
    ezEditorEngineProcessConnection::GetInstance()->Update();

  m_pTimer->start(1);
}

void ezEditorApp::DocumentManagerEventHandler(const ezDocumentManagerBase::Event& r)
{
  switch (r.m_Type)
  {
  case ezDocumentManagerBase::Event::Type::DocumentOpened:
    {
      s_RecentDocuments.Insert(r.m_pDocument->GetDocumentPath());
      SaveSettings();
    }
    break;
  case ezDocumentManagerBase::Event::Type::DocumentClosing:
    {
      // Clear all document settings when it is closed
      s_DocumentSettings.Remove(r.m_pDocument->GetDocumentPath());
    }
    break;
  }
}

void ezEditorApp::DocumentEventHandler(const ezDocumentBase::Event& e)
{
  switch (e.m_Type)
  {
  case ezDocumentBase::Event::Type::SaveDocumentMetaState:
    {
      SaveDocumentSettings(e.m_pDocument);
    }
    break;
  }
}

void ezEditorApp::DocumentManagerRequestHandler(ezDocumentManagerBase::Request& r)
{
  switch (r.m_Type)
  {
  case ezDocumentManagerBase::Request::Type::DocumentAllowedToOpen:
    {
      // if someone else already said no, don't bother to check further
      if (r.m_RequestStatus.m_Result.Failed())
        return;

      if (!ezEditorProject::IsProjectOpen())
      {
        // if no project is open yet, try to open the corresponding one

        ezString sProjectPath = ezEditorProject::FindProjectForDocument(r.m_sDocumentPath);

        // if no project could be located, just reject the request
        if (sProjectPath.IsEmpty())
        {
          r.m_RequestStatus = ezStatus("No project could be opened");
          return;
        }
        else
        {
          // if a project could be found, try to open it
          ezStatus res = ezEditorProject::OpenProject(sProjectPath);

          // if project opening failed, relay that error message
          if (res.m_Result.Failed())
          {
            r.m_RequestStatus = res;
            return;
          }
        }
      }
      else
      {
        if (!ezEditorProject::GetInstance()->IsDocumentInProject(r.m_sDocumentPath))
        {
          r.m_RequestStatus = ezStatus("The document is not part of the currently open project");
          return;
        }
      }
    }
    return;
  }
}

void ezEditorApp::EngineProcessMsgHandler(const ezEditorEngineProcessConnection::Event& e)
{
  switch (e.m_Type)
  {
  case ezEditorEngineProcessConnection::Event::Type::ProcessMessage:
    {
      static ezReflectedTypeDescriptor s_TypeDesc;

      if (e.m_pMsg->GetDynamicRTTI()->IsDerivedFrom<ezUpdateReflectionTypeMsgToEditor>())
      {
        const ezUpdateReflectionTypeMsgToEditor* pMsg = static_cast<const ezUpdateReflectionTypeMsgToEditor*>(e.m_pMsg);

        s_TypeDesc.m_sTypeName = pMsg->m_sTypeName;
        s_TypeDesc.m_sDefaultInitialization = pMsg->m_sDefaultInitialization;
        s_TypeDesc.m_sParentTypeName = pMsg->m_sParentTypeName;
        s_TypeDesc.m_sPluginName = pMsg->m_sPluginName;
        s_TypeDesc.m_Properties.SetCount(pMsg->m_uiNumProperties);

        if (pMsg->m_uiNumProperties == 0)
        {
          ezReflectedTypeManager::RegisterType(s_TypeDesc);
        }
      }
      else if (e.m_pMsg->GetDynamicRTTI()->IsDerivedFrom<ezUpdateReflectionPropertyMsgToEditor>())
      {
        const ezUpdateReflectionPropertyMsgToEditor* pMsg = static_cast<const ezUpdateReflectionPropertyMsgToEditor*>(e.m_pMsg);

        auto& ref = s_TypeDesc.m_Properties[pMsg->m_uiPropertyIndex];
        ref.m_ConstantValue = pMsg->m_ConstantValue;
        ref.m_Flags = ((PropertyFlags::Enum) pMsg->m_Flags);
        ref.m_sName = pMsg->m_sName;
        ref.m_Type = (ezVariant::Type::Enum) pMsg->m_Type;
        ref.m_sType = pMsg->m_sType;

        if (pMsg->m_uiPropertyIndex + 1 == s_TypeDesc.m_Properties.GetCount())
        {
          ezReflectedTypeManager::RegisterType(s_TypeDesc);
          s_TypeDesc.m_Properties.Clear();
        }
      }
    }
    break;

  default:
    return;
  }
}

void ezEditorApp::ProjectEventHandler(const ezEditorProject::Event& r)
{
  switch (r.m_Type)
  {
  case ezEditorProject::Event::Type::ProjectOpened:
  case ezEditorProject::Event::Type::ProjectClosing:
    {
      s_RecentProjects.Insert(ezEditorProject::GetInstance()->GetProjectPath());
      SaveSettings();
    }
    break;
  case ezEditorProject::Event::Type::ProjectClosed:
    {
      // make sure to clear all project settings when a project is closed
      s_ProjectSettings.Clear();
    }
    break;
  }
}

void ezEditorApp::ProjectRequestHandler(ezEditorProject::Request& r)
{
  switch (r.m_Type)
  {
    case ezEditorProject::Request::Type::CanProjectClose:
    {
      if (r.m_bProjectCanClose == false)
        return;

      ezHybridArray<ezDocumentBase*, 32> ModifiedDocs;

      for (ezDocumentManagerBase* pMan : ezDocumentManagerBase::GetAllDocumentManagers())
      {
        for (ezDocumentBase* pDoc : pMan->GetAllDocuments())
        {
          if (pDoc->IsModified())
            ModifiedDocs.PushBack(pDoc);
        }
      }

      if (!ModifiedDocs.IsEmpty())
      {
        DocumentList dlg(s_ContainerWindows[0], ModifiedDocs);
        if (dlg.exec() == 0)
          r.m_bProjectCanClose = false;
      }
    }
      return;
  }
}

void ezRecentFilesList::Insert(const char* szFile)
{
  ezStringBuilder sCleanPath = szFile;
  sCleanPath.MakeCleanPath();

  ezString s = sCleanPath;

  m_Files.Remove(s);
  m_Files.PushFront(s);

  if (m_Files.GetCount() > m_uiMaxElements)
    m_Files.SetCount(m_uiMaxElements);
}

void ezRecentFilesList::Save(const char* szFile)
{
  ezFileWriter File;
  if (File.Open(szFile).Failed())
    return;

  for (const ezString& s : m_Files)
  {
    File.WriteBytes(s.GetData(), s.GetElementCount());
    File.WriteBytes("\n", sizeof(char));
  }
}

void ezRecentFilesList::Load(const char* szFile)
{
  m_Files.Clear();

  ezFileReader File;
  if (File.Open(szFile).Failed())
    return;

  ezStringBuilder sAllLines;
  sAllLines.ReadAll(File);

  ezHybridArray<ezStringView, 16> Lines;
  sAllLines.Split(false, Lines, "\n");

  for (const ezStringView& sv : Lines)
  {
    m_Files.PushBack(sv);
  }
}

ezString ezEditorApp::GetDocumentDataFolder(const char* szDocument)
{
  ezStringBuilder sPath = szDocument;
  sPath.Append("_data");

  return sPath;
}






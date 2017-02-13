#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <FileservePlugin/Network/NetworkInterface.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/Uuid.h>

namespace ezDataDirectory
{
  class FileserveType;
}

/// \brief Singleton that represents the client side part of a fileserve connection
///
/// Whether the fileserve plugin will be enabled is controled by ezFileserveClient::s_bEnableFileserve
/// By default this is on, but if switched off, the fileserve client functionality will be disabled.
/// ezFileserveClient will also switch its functionality off, if the command line argument "-fsoff" is specified.
/// If a program knows that it always wants to switch fileserving off, it should either simply not load the plugin at all,
/// or it can inject that command line argument through ezCommandLineUtils. This should be done before application startup
/// and especially before any data directories get mounted.
class EZ_FILESERVEPLUGIN_DLL ezFileserveClient
{
  EZ_DECLARE_SINGLETON(ezFileserveClient);

public:
  ezFileserveClient();
  ~ezFileserveClient();

  /// \brief Allows to disable the fileserving functionality. Should be called before mounting data directories.
  ///
  /// Also achieved through the command line argument "-fsoff"
  static void DisabledFileserveClient() { s_bEnableFileserve = false; }

  /// \brief Sets the address through which the client tries to connect to the server. Default is "localhost:1042"
  ///
  /// Can also be set through the command line argument "-fsserver" followed by the address and port.
  void SetServerConnectionAddress(const char* szAddress) { m_sServerConnectionAddress = szAddress; }

  /// \brief Can be called to ensure a fileserve connection. Otherwise automatically called when a data directory is mounted.
  ezResult EnsureConnected();

  /// \brief Needs to be called regularly to update the network. By default this is automatically called when the global event
  /// 'GameApp_UpdatePlugins' is fired, which is done by ezGameApplication.
  void UpdateClient();


private:
  friend class ezDataDirectory::FileserveType;

  /// \brief True by default, can
  static bool s_bEnableFileserve;

  struct FileCacheStatus
  {
    ezUInt16 m_uiDataDir = 0xffff;
    ezInt64 m_TimeStamp = 0;
    ezUInt64 m_FileHash = 0;
    ezTime m_LastCheck;
  };

  struct DataDir
  {
    //ezString m_sRootName;
    //ezString m_sPathOnClient;
    ezString m_sMountPoint;
    bool m_bMounted = false;
  };

  void DeleteFile(ezUInt16 uiDataDir, const char* szFile);
  ezUInt16 MountDataDirectory(const char* szDataDir, const char* szRootName);
  void UnmountDataDirectory(ezUInt16 uiDataDir);
  void ComputeDataDirMountPoint(const char* szDataDir, ezStringBuilder& out_sMountPoint) const;
  void BuildPathInCache(const char* szFile, const char* szMountPoint, ezStringBuilder& out_sAbsPath, ezStringBuilder& out_sFullPathMeta) const;
  void GetFullDataDirCachePath(const char* szDataDir, ezStringBuilder& out_sFullPath, ezStringBuilder& out_sFullPathMeta) const;
  void NetworkMsgHandler(ezNetworkMessage& msg);
  void HandleFileTransferMsg(ezNetworkMessage &msg);
  void HandleFileTransferFinishedMsg(ezNetworkMessage &msg);
  ezResult DownloadFile(ezUInt16 uiDataDirID, const char* szFile, bool bForceThisDataDir);
  void DetermineCacheStatus(ezUInt16 uiDataDirID, const char* szFile, FileCacheStatus& out_Status) const;
  void UploadFile(ezUInt16 uiDataDirID, const char* szFile, const ezDynamicArray<ezUInt8>& fileContent);

  ezString m_sServerConnectionAddress;
  ezString m_sFileserveCacheFolder;
  ezString m_sFileserveCacheMetaFolder;
  bool m_bDownloading = false;
  bool m_bFailedToConnect = false;
  ezUuid m_CurFileRequestGuid;
  ezUInt16 m_uiCurFileRequestDataDir = 0;
  ezStringBuilder m_sCurFileRequest;
  ezStringBuilder m_sCurFileRequestCacheName;
  ezUniquePtr<ezNetworkInterface> m_Network;
  ezDynamicArray<ezUInt8> m_Download;
  ezMap<ezString, FileCacheStatus> m_CachedFileStatus;


  ezHybridArray<DataDir, 8> m_MountedDataDirs;
};


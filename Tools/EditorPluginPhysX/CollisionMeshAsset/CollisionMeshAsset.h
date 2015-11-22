#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginPhysX/CollisionMeshAsset/CollisionMeshAssetObjects.h>

class ezPhysXMeshResourceDescriptor;
class ezGeometry;

class ezCollisionMeshAssetDocument : public ezSimpleAssetDocument<ezCollisionMeshAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCollisionMeshAssetDocument, ezSimpleAssetDocument<ezCollisionMeshAssetProperties>);

public:
  ezCollisionMeshAssetDocument(const char* szDocumentPath);

  virtual const char* GetDocumentTypeDisplayString() const override { return "Collision Mesh Asset"; }

  virtual const char* QueryAssetType() const override { return "Collision Mesh"; }

protected:
  virtual ezUInt16 GetAssetTypeVersion() const override { return 1; }
  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) override;
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szPlatform) override;

  ezStatus CreateMeshFromFile(const ezCollisionMeshAssetProperties* pProp, ezPhysXMeshResourceDescriptor &desc, bool bFlipTriangles, const ezMat3 &mTransformation);

  virtual ezStatus InternalRetrieveAssetInfo(const char* szPlatform) override;

};

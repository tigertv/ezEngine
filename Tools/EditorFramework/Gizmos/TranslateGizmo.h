#pragma once

#include <ToolsFoundation/Basics.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <QPoint>

class EZ_EDITORFRAMEWORK_DLL ezTranslateGizmo : public ezGizmo
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTranslateGizmo, ezGizmo);

public:
  ezTranslateGizmo();

  const ezVec3 GetTranslationResult() const { return GetTransformation().GetTranslationVector() - m_vStartPosition; }
  const ezVec3 GetTranslationDiff() const { return m_vLastMoveDiff; }

  enum class MovementMode
  {
    ScreenProjection,
    MouseDiff
  };

  void SetMovementMode(MovementMode mode);

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual ezEditorInut DoMousePressEvent(QMouseEvent* e) override;
  virtual ezEditorInut DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual ezEditorInut DoMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const ezMat4& transform) override;

  ezResult GetPointOnAxis(ezInt32 iScreenPosX, ezInt32 iScreenPosY, ezVec3& out_Result) const;
  ezResult GetPointOnPlane(ezInt32 iScreenPosX, ezInt32 iScreenPosY, ezVec3& out_Result) const;

private:
  ezVec2I32 m_LastMousePos;

  ezVec3 m_vLastMoveDiff;

  MovementMode m_MovementMode;
  ezEngineGizmoHandle m_AxisX;
  ezEngineGizmoHandle m_AxisY;
  ezEngineGizmoHandle m_AxisZ;

  ezEngineGizmoHandle m_PlaneXY;
  ezEngineGizmoHandle m_PlaneXZ;
  ezEngineGizmoHandle m_PlaneYZ;

  enum class TranslateMode
  {
    None,
    Axis,
    Plane
  };

  TranslateMode m_Mode;

  float m_fStartScale;

  ezTime m_LastInteraction;
  ezVec3 m_vMoveAxis;
  ezVec3 m_vPlaneAxis[2];
  ezVec3 m_vStartPosition;
  ezMat4 m_InvViewProj;
};

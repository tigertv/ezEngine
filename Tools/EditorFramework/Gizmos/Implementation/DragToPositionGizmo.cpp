#include <PCH.h>
#include <EditorFramework/Gizmos/DragToPositionGizmo.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <Foundation/Logging/Log.h>
#include <QMouseEvent>
#include <CoreUtils/Graphics/Camera.h>
#include <Foundation/Utilities/GraphicsUtils.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDragToPositionGizmo, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezDragToPositionGizmo::ezDragToPositionGizmo()
{
  m_bModifiesRotation = false;

  m_Bobble.Configure(this, ezEngineGizmoHandleType::Box, ezColor::DodgerBlue);
  m_AlignPX.Configure(this, ezEngineGizmoHandleType::HalfPiston, ezColor::SteelBlue);
  m_AlignNX.Configure(this, ezEngineGizmoHandleType::HalfPiston, ezColor::SteelBlue);
  m_AlignPY.Configure(this, ezEngineGizmoHandleType::HalfPiston, ezColor::SteelBlue);
  m_AlignNY.Configure(this, ezEngineGizmoHandleType::HalfPiston, ezColor::SteelBlue);
  m_AlignPZ.Configure(this, ezEngineGizmoHandleType::HalfPiston, ezColor::SteelBlue);
  m_AlignNZ.Configure(this, ezEngineGizmoHandleType::HalfPiston, ezColor::SteelBlue);

  SetVisible(false);
  SetTransformation(ezMat4::IdentityMatrix());
}

void ezDragToPositionGizmo::OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView)
{
  m_Bobble.SetOwner(pOwnerWindow);
  m_AlignPX.SetOwner(pOwnerWindow);
  m_AlignNX.SetOwner(pOwnerWindow);
  m_AlignPY.SetOwner(pOwnerWindow);
  m_AlignNY.SetOwner(pOwnerWindow);
  m_AlignPZ.SetOwner(pOwnerWindow);
  m_AlignNZ.SetOwner(pOwnerWindow);
}

void ezDragToPositionGizmo::OnVisibleChanged(bool bVisible)
{
  m_Bobble.SetVisible(bVisible);
  m_AlignPX.SetVisible(bVisible);
  m_AlignNX.SetVisible(bVisible);
  m_AlignPY.SetVisible(bVisible);
  m_AlignNY.SetVisible(bVisible);
  m_AlignPZ.SetVisible(bVisible);
  m_AlignNZ.SetVisible(bVisible);
}

void ezDragToPositionGizmo::OnTransformationChanged(const ezMat4& transform)
{
  m_Bobble.SetTransformation(transform);

  ezMat4 m;

  m.SetIdentity();
  m_AlignPX.SetTransformation(transform * m);
  m.SetRotationMatrixY(ezAngle::Degree(180));
  m_AlignNX.SetTransformation(transform * m);

  m.SetRotationMatrixZ(ezAngle::Degree(+90));
  m_AlignPY.SetTransformation(transform * m);
  m.SetRotationMatrixZ(ezAngle::Degree(-90));
  m_AlignNY.SetTransformation(transform * m);

  m.SetRotationMatrixY(ezAngle::Degree(-90));
  m_AlignPZ.SetTransformation(transform * m);
  m.SetRotationMatrixY(ezAngle::Degree(+90));
  m_AlignNZ.SetTransformation(transform * m);

}

void ezDragToPositionGizmo::FocusLost(bool bCancel)
{
  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = bCancel ? ezGizmoEvent::Type::CancelInteractions : ezGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  ezViewHighlightMsgToEngine msg;
  msg.SendHighlightObjectMessage(GetOwnerWindow()->GetEditorEngineConnection());

  m_Bobble.SetVisible(true);
  m_AlignPX.SetVisible(true);
  m_AlignNX.SetVisible(true);
  m_AlignPY.SetVisible(true);
  m_AlignNY.SetVisible(true);
  m_AlignPZ.SetVisible(true);
  m_AlignNZ.SetVisible(true);
}

ezEditorInut ezDragToPositionGizmo::mousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return ezEditorInut::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInut::MayBeHandledByOthers;

  ezViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  msg.SendHighlightObjectMessage(GetOwnerWindow()->GetEditorEngineConnection());

  // The gizmo is actually "hidden" somewhere else during dragging,
  // because it musn't be rendered into the picking buffer, to avoid picking against the gizmo
  //m_Bobble.SetVisible(false);
  //m_AlignPX.SetVisible(false);
  //m_AlignNX.SetVisible(false);
  //m_AlignPY.SetVisible(false);
  //m_AlignNY.SetVisible(false);
  //m_AlignPZ.SetVisible(false);
  //m_AlignNZ.SetVisible(false);
  //m_pInteractionGizmoHandle->SetVisible(true);

  m_vStartPosition = GetTransformation().GetTranslationVector();

  m_LastInteraction = ezTime::Now();

  SetActiveInputContext(this);

  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = ezGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return ezEditorInut::WasExclusivelyHandled;
}

ezEditorInut ezDragToPositionGizmo::mouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInut::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return ezEditorInut::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return ezEditorInut::WasExclusivelyHandled;
}

static const ezVec3 GetOrthogonalVector(const ezVec3& vDir)
{
  if (ezMath::Abs(vDir.Dot(ezVec3(0, 0, 1))) < 0.999f)
    return -vDir.Cross(ezVec3(0, 0, 1));

  return -vDir.Cross(ezVec3(1, 0, 0));
}

ezEditorInut ezDragToPositionGizmo::mouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInut::MayBeHandledByOthers;

  const ezTime tNow = ezTime::Now();

  if (tNow - m_LastInteraction < ezTime::Seconds(1.0 / 25.0))
    return ezEditorInut::WasExclusivelyHandled;

  m_LastInteraction = tNow;

  const ezObjectPickingResult& res = GetOwnerWindow()->PickObject(e->pos().x(), e->pos().y());

  if (!res.m_PickedObject.IsValid())
    return ezEditorInut::WasExclusivelyHandled;

  if (res.m_vPickedPosition.IsNaN() || res.m_vPickedNormal.IsNaN() || res.m_vPickedNormal.IsZero())
    return ezEditorInut::WasExclusivelyHandled;

  const ezVec3 vTangent = GetOrthogonalVector(res.m_vPickedNormal).GetNormalized();
  const ezVec3 vBiTangent = res.m_vPickedNormal.Cross(vTangent).GetNormalized();

  ezMat3 mRot;
  ezMat4 mTrans = GetTransformation();
  mTrans.SetTranslationVector(res.m_vPickedPosition);

  m_bModifiesRotation = true;

  if (m_pInteractionGizmoHandle == &m_AlignPX)
  {
    mRot.SetColumn(0, res.m_vPickedNormal);
    mRot.SetColumn(1, vTangent);
    mRot.SetColumn(2, vBiTangent);
  }
  else if (m_pInteractionGizmoHandle == &m_AlignNX)
  {
    mRot.SetColumn(0, -res.m_vPickedNormal);
    mRot.SetColumn(2, vBiTangent);
    mRot.SetColumn(1, -vTangent);
  }
  else if (m_pInteractionGizmoHandle == &m_AlignPY)
  {
    mRot.SetColumn(0, -vTangent);
    mRot.SetColumn(1, res.m_vPickedNormal);
    mRot.SetColumn(2, vBiTangent);
  }
  else if (m_pInteractionGizmoHandle == &m_AlignNY)
  {
    mRot.SetColumn(0, vTangent);
    mRot.SetColumn(1, -res.m_vPickedNormal);
    mRot.SetColumn(2, vBiTangent);
  }
  else if (m_pInteractionGizmoHandle == &m_AlignPZ)
  {
    mRot.SetColumn(0, vTangent);
    mRot.SetColumn(1, vBiTangent);
    mRot.SetColumn(2, res.m_vPickedNormal);
  }
  else if (m_pInteractionGizmoHandle == &m_AlignNZ)
  {
    mRot.SetColumn(0, -vTangent);
    mRot.SetColumn(1, vBiTangent);
    mRot.SetColumn(2, -res.m_vPickedNormal);
  }
  else
  {
    m_bModifiesRotation = false;
    mRot.SetIdentity();
  }

  mTrans.SetRotationalPart(mRot);
  SetTransformation(mTrans);

  ezGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = ezGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return ezEditorInut::WasExclusivelyHandled;
}


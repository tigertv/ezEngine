#include <Foundation/PCH.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Logging/Log.h>

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezObjectChangeType, 1)
EZ_ENUM_CONSTANTS(ezObjectChangeType::NodeAdded, ezObjectChangeType::NodeRemoved)
EZ_ENUM_CONSTANTS(ezObjectChangeType::PropertySet, ezObjectChangeType::PropertyInserted, ezObjectChangeType::PropertyRemoved)
EZ_END_STATIC_REFLECTED_ENUM();

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezDiffOperation, ezNoBase, 1, ezRTTIDefaultAllocator<ezDiffOperation>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Operation", ezObjectChangeType, m_Operation),
    EZ_MEMBER_PROPERTY("Node", m_Node),
    EZ_MEMBER_PROPERTY("Property", m_sProperty),
    EZ_MEMBER_PROPERTY("Index", m_Index),
    EZ_MEMBER_PROPERTY("Value", m_Value),
  }
    EZ_END_PROPERTIES
}
EZ_END_STATIC_REFLECTED_TYPE


ezAbstractObjectGraph::~ezAbstractObjectGraph()
{
  Clear();
}

void ezAbstractObjectGraph::Clear()
{
  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    EZ_DEFAULT_DELETE(it.Value());
  }
  m_Nodes.Clear();
  m_NodesByName.Clear();
  m_Strings.Clear();
}

const char* ezAbstractObjectGraph::RegisterString(const char* szString)
{
  auto it = m_Strings.Insert(szString);
  EZ_ASSERT_DEV(it.IsValid(), "");
  return it.Key().GetData();
}

ezAbstractObjectNode* ezAbstractObjectGraph::GetNode(const ezUuid& guid)
{
  auto it = m_Nodes.Find(guid);
  if (it.IsValid())
    return it.Value();

  return nullptr;
}

const ezAbstractObjectNode* ezAbstractObjectGraph::GetNode(const ezUuid& guid) const
{
  return const_cast<ezAbstractObjectGraph*>(this)->GetNode(guid);
}

const ezAbstractObjectNode* ezAbstractObjectGraph::GetNodeByName(const char* szName) const
{
  return const_cast<ezAbstractObjectGraph*>(this)->GetNodeByName(szName);
}

ezAbstractObjectNode* ezAbstractObjectGraph::GetNodeByName(const char* szName)
{
  auto it = m_Strings.Find(szName);
  if (!it.IsValid())
    return nullptr;

  auto itNode = m_NodesByName.Find(it.Key().GetData());
  if (!itNode.IsValid())
    return nullptr;

  return itNode.Value();
}

ezAbstractObjectNode* ezAbstractObjectGraph::AddNode(const ezUuid& guid, const char* szType, const char* szNodeName)
{
  EZ_ASSERT_DEV(!m_Nodes.Contains(guid), "object must not yet exist");
  if (szNodeName != nullptr)
  {
    szNodeName = RegisterString(szNodeName);
  }

  ezAbstractObjectNode* pNode = EZ_DEFAULT_NEW(ezAbstractObjectNode);
  pNode->m_Guid = guid;
  pNode->m_pOwner = this;
  pNode->m_szType = RegisterString(szType);
  pNode->m_szNodeName = szNodeName;

  m_Nodes[guid] = pNode;

  if (szNodeName != nullptr)
  {
    m_NodesByName[szNodeName] = pNode;
  }

  return pNode;
}

void ezAbstractObjectGraph::RemoveNode(const ezUuid& guid)
{
  auto it = m_Nodes.Find(guid);

  if (it.IsValid())
  {
    ezAbstractObjectNode* pNode = it.Value();
    if (pNode->m_szNodeName != nullptr)
      m_NodesByName.Remove(pNode->m_szNodeName);

    m_Nodes.Remove(guid);
    EZ_DEFAULT_DELETE(pNode);
  }
}

void ezAbstractObjectNode::AddProperty(const char* szName, const ezVariant& value)
{
  auto& prop = m_Properties.ExpandAndGetRef();
  prop.m_szPropertyName = m_pOwner->RegisterString(szName);
  prop.m_Value = value;
}

void ezAbstractObjectNode::ChangeProperty(const char* szName, const ezVariant& value)
{
  for (ezUInt32 i = 0; i < m_Properties.GetCount(); ++i)
  {
    if (ezStringUtils::IsEqual(m_Properties[i].m_szPropertyName, szName))
    {
      m_Properties[i].m_Value = value;
      return;
    }
  }

  EZ_REPORT_FAILURE("Property '%s' is unknown", szName);
}

void ezAbstractObjectNode::RemoveProperty(const char* szName)
{
  for (ezUInt32 i = 0; i < m_Properties.GetCount(); ++i)
  {
    if (ezStringUtils::IsEqual(m_Properties[i].m_szPropertyName, szName))
    {
      m_Properties.RemoveAtSwap(i);
      return;
    }
  }
}

const ezAbstractObjectNode::Property* ezAbstractObjectNode::FindProperty(const char* szName) const
{
  for (ezUInt32 i = 0; i < m_Properties.GetCount(); ++i)
  {
    if (ezStringUtils::IsEqual(m_Properties[i].m_szPropertyName, szName))
    {
      return &m_Properties[i];
    }
  }

  return nullptr;
}

ezAbstractObjectNode::Property* ezAbstractObjectNode::FindProperty(const char* szName)
{
  for (ezUInt32 i = 0; i < m_Properties.GetCount(); ++i)
  {
    if (ezStringUtils::IsEqual(m_Properties[i].m_szPropertyName, szName))
    {
      return &m_Properties[i];
    }
  }

  return nullptr;
}

void ezAbstractObjectGraph::ReMapNodeGuids(const ezUuid& seedGuid, bool bRemapInverse /*= false*/)
{
  ezHybridArray<ezAbstractObjectNode*, 16> nodes;
  ezMap<ezUuid, ezUuid> guidMap;
  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    ezUuid newGuid = it.Key();

    if (bRemapInverse)
      newGuid.RevertCombinationWithSeed(seedGuid);
    else
      newGuid.CombineWithSeed(seedGuid);

    guidMap[it.Key()] = newGuid;

    nodes.PushBack(it.Value());
  }

  m_Nodes.Clear();

  // go through all nodes to remap guids
  for (auto* pNode : nodes)
  {
    pNode->m_Guid = guidMap[pNode->m_Guid];

    // check every property
    for (auto& prop : pNode->m_Properties)
    {
      RemapVariant(prop.m_Value, guidMap);

    }
    m_Nodes[pNode->m_Guid] = pNode;
  }
}

void ezAbstractObjectGraph::CopyNodeIntoGraph(ezAbstractObjectNode* pNode)
{
  auto pNewNode = AddNode(pNode->GetGuid(), pNode->GetType(), pNode->GetNodeName());

  for (const auto& props : pNode->GetProperties())
    pNewNode->AddProperty(props.m_szPropertyName, props.m_Value);
}


void ezAbstractObjectGraph::CreateDiffWithBaseGraph(const ezAbstractObjectGraph& base, ezDeque<ezAbstractGraphDiffOperation>& out_DiffResult)
{
  out_DiffResult.Clear();

  // check whether any nodes have been deleted
  {
    for (auto itNodeBase = base.GetAllNodes().GetIterator(); itNodeBase.IsValid(); ++itNodeBase)
    {
      if (GetNode(itNodeBase.Key()) == nullptr)
      {
        // does not exist in this graph -> has been deleted from base
        ezAbstractGraphDiffOperation op;
        op.m_Node = itNodeBase.Key();
        op.m_Operation = ezAbstractGraphDiffOperation::Op::NodeRemoved;
        op.m_sProperty = itNodeBase.Value()->m_szType;
        op.m_Value = itNodeBase.Value()->m_szNodeName;

        out_DiffResult.PushBack(op);
      }
    }
  }

  // check whether any nodes have been added
  {
    for (auto itNodeThis = GetAllNodes().GetIterator(); itNodeThis.IsValid(); ++itNodeThis)
    {
      if (base.GetNode(itNodeThis.Key()) == nullptr)
      {
        // does not exist in base graph -> has been added
        ezAbstractGraphDiffOperation op;
        op.m_Node = itNodeThis.Key();
        op.m_Operation = ezAbstractGraphDiffOperation::Op::NodeAdded;
        op.m_sProperty = itNodeThis.Value()->m_szType;
        op.m_Value = itNodeThis.Value()->m_szNodeName;

        out_DiffResult.PushBack(op);

        // set all properties
        for (const auto& prop : itNodeThis.Value()->GetProperties())
        {
          op.m_Operation = ezAbstractGraphDiffOperation::Op::PropertyChanged;
          op.m_sProperty = prop.m_szPropertyName;
          op.m_Value = prop.m_Value;

          out_DiffResult.PushBack(op);
        }
      }
    }
  }

  // check whether any properties have been modified
  {
    for (auto itNodeThis = GetAllNodes().GetIterator(); itNodeThis.IsValid(); ++itNodeThis)
    {
      const auto pBaseNode = base.GetNode(itNodeThis.Key());

      if (pBaseNode == nullptr)
        continue;

      for (const ezAbstractObjectNode::Property& prop : itNodeThis.Value()->GetProperties())
      {
        bool bDifferent = true;

        for (const ezAbstractObjectNode::Property& baseProp : pBaseNode->GetProperties())
        {
          if (ezStringUtils::IsEqual(baseProp.m_szPropertyName, prop.m_szPropertyName))
          {
            if (baseProp.m_Value == prop.m_Value)
            {
              bDifferent = false;
              break;
            }

            bDifferent = true;
            break;
          }
        }

        if (bDifferent)
        {
          ezAbstractGraphDiffOperation op;
          op.m_Node = itNodeThis.Key();
          op.m_Operation = ezAbstractGraphDiffOperation::Op::PropertyChanged;
          op.m_sProperty = prop.m_szPropertyName;
          op.m_Value = prop.m_Value;

          out_DiffResult.PushBack(op);
        }
      }
    }
  }
}


void ezAbstractObjectGraph::ApplyDiff(ezDeque<ezAbstractGraphDiffOperation>& Diff)
{
  for (const auto& op : Diff)
  {
    switch (op.m_Operation)
    {
    case ezObjectChangeType::NodeAdded:
      {
        AddNode(op.m_Node, op.m_sProperty, op.m_Value.Get<ezString>());
      }
      break;

    case ezObjectChangeType::NodeRemoved:
      {
        RemoveNode(op.m_Node);
      }
      break;

    case ezObjectChangeType::PropertySet:
      {
        auto* pNode = GetNode(op.m_Node);
        if (pNode)
        {
          auto* pProp = pNode->FindProperty(op.m_sProperty);

          if (!pProp)
            pNode->AddProperty(op.m_sProperty, op.m_Value);
          else
            pProp->m_Value = op.m_Value;
        }
      }
      break;
    }
  }
}


void ezAbstractObjectGraph::MergeDiffs(const ezDeque<ezAbstractGraphDiffOperation>& lhs, const ezDeque<ezAbstractGraphDiffOperation>& rhs, ezDeque<ezAbstractGraphDiffOperation>& out)
{
  struct Prop
  {
    Prop() {}
    Prop(ezUuid node, ezStringView sProperty)
    {
      m_Node = node;
      m_sProperty = sProperty;
    }
    ezUuid m_Node;
    ezStringView m_sProperty;

    bool operator<(const Prop& rhs) const
    {
      if (m_Node == rhs.m_Node)
        return m_sProperty < rhs.m_sProperty;

      return m_Node < rhs.m_Node;
    }

    bool operator==(const Prop& rhs) const
    {
      return m_Node == rhs.m_Node && m_sProperty == rhs.m_sProperty;
    }
  };

  ezMap<Prop, ezHybridArray<const ezAbstractGraphDiffOperation*, 2> > propChanges;
  ezSet<ezUuid> removed;
  ezMap<ezUuid, ezUInt32> added;
  for (const ezAbstractGraphDiffOperation& op : lhs)
  {
    if (op.m_Operation == ezAbstractGraphDiffOperation::Op::NodeRemoved)
    {
      removed.Insert(op.m_Node);
      out.PushBack(op);
    }
    else if (op.m_Operation == ezAbstractGraphDiffOperation::Op::NodeAdded)
    {
      added[op.m_Node] = out.GetCount();
      out.PushBack(op);
    }
    else if (op.m_Operation == ezAbstractGraphDiffOperation::Op::PropertyChanged)
    {
      auto it = propChanges.FindOrAdd(Prop(op.m_Node, op.m_sProperty));
      it.Value().PushBack(&op);
    }
  }
  for (const ezAbstractGraphDiffOperation& op : rhs)
  {
    if (op.m_Operation == ezAbstractGraphDiffOperation::Op::NodeRemoved)
    {
      if (!removed.Contains(op.m_Node))
        out.PushBack(op);
    }
    else if (op.m_Operation == ezAbstractGraphDiffOperation::Op::NodeAdded)
    {
      if (added.Contains(op.m_Node))
      {
        ezAbstractGraphDiffOperation& leftOp = out[added[op.m_Node]];
        leftOp.m_sProperty = op.m_sProperty; // Take type from rhs.
      }
      else
      {
        out.PushBack(op);
      }
    }
    else if (op.m_Operation == ezAbstractGraphDiffOperation::Op::PropertyChanged)
    {
      auto it = propChanges.FindOrAdd(Prop(op.m_Node, op.m_sProperty));
      it.Value().PushBack(&op);
    }
  }

  for (auto it = propChanges.GetIterator(); it.IsValid(); ++it)
  {
    const Prop& key = it.Key();
    const ezHybridArray<const ezAbstractGraphDiffOperation*, 2>& value = it.Value();

    if (value.GetCount() == 1)
    {
      out.PushBack(*value[0]);
    }
    else
    {
      const ezAbstractGraphDiffOperation& leftProp = *value[0];
      const ezAbstractGraphDiffOperation& rightProp = *value[1];

      if (leftProp.m_Value.GetType() == ezVariantType::VariantArray && rightProp.m_Value.GetType() == ezVariantType::VariantArray)
      {
        const ezVariantArray& leftArray = leftProp.m_Value.Get<ezVariantArray>();
        const ezVariantArray& rightArray = rightProp.m_Value.Get<ezVariantArray>();

        const ezAbstractObjectNode* pNode = GetNode(key.m_Node);
        if (pNode)
        {
          ezStringBuilder sTemp(key.m_sProperty);
          const ezAbstractObjectNode::Property* pProperty = pNode->FindProperty(sTemp);
          if (pProperty && pProperty->m_Value.GetType() == ezVariantType::VariantArray)
          {
            // Do 3-way array merge
            const ezVariantArray& baseArray = pProperty->m_Value.Get<ezVariantArray>();
            ezVariantArray res;
            MergeArrays(baseArray, leftArray, rightArray, res);
            out.PushBack(rightProp);
            out.PeekBack().m_Value = res;
          }
          else
          {
            out.PushBack(rightProp);
          }
        }
        else
        {
          out.PushBack(rightProp);
        }

      }
      else
      {
        out.PushBack(rightProp);
      }
    }
  }

}

void ezAbstractObjectGraph::RemapVariant(ezVariant& value, const ezMap<ezUuid, ezUuid>& guidMap)
{
  // if the property is a guid, we check if we need to remap it
  if (value.IsA<ezUuid>())
  {
    const ezUuid& guid = value.Get<ezUuid>();

    // if we find the guid in our map, replace it by the new guid
    auto it = guidMap.Find(guid);

    if (it.IsValid())
    {
      value = it.Value();
    }
  }
  // Arrays may be of uuids
  else if (value.IsA<ezVariantArray>())
  {
    const ezVariantArray& values = value.Get<ezVariantArray>();
    bool bNeedToRemap = false;
    for (auto& subValue : values)
    {
      if (subValue.IsA<ezUuid>() && guidMap.Contains(subValue.Get<ezUuid>()))
      {
        bNeedToRemap = true;
        break;
      }
      else if (subValue.IsA<ezVariantArray>())
      {
        bNeedToRemap = true;
        break;
      }
    }

    if (bNeedToRemap)
    {
      ezVariantArray newValues = values;
      for (auto& subValue : newValues)
      {
        RemapVariant(subValue, guidMap);
      }
      value = newValues;
    }
  }
}

void ezAbstractObjectGraph::MergeArrays(const ezDynamicArray<ezVariant>& baseArray, const ezDynamicArray<ezVariant>& leftArray, const ezDynamicArray<ezVariant>& rightArray, ezDynamicArray<ezVariant>& out)
{
  // Find element type.
  ezVariantType::Enum type = ezVariantType::Invalid;
  if (!baseArray.IsEmpty())
    type = baseArray[0].GetType();
  if (type != ezVariantType::Invalid && !leftArray.IsEmpty())
    type = leftArray[0].GetType();
  if (type != ezVariantType::Invalid && !rightArray.IsEmpty())
    type = rightArray[0].GetType();

  if (type == ezVariantType::Invalid)
    return;

  // For now, assume non-uuid types are arrays, uuids are sets.
  if (type != ezVariantType::Uuid)
  {
    // Any size changes?
    ezUInt32 uiSize = baseArray.GetCount();
    if (leftArray.GetCount() != baseArray.GetCount())
      uiSize = leftArray.GetCount();
    if (rightArray.GetCount() != baseArray.GetCount())
      uiSize = rightArray.GetCount();

    out.SetCount(uiSize);
    for (ezUInt32 i = 0; i < uiSize; i++)
    {
      if (i < baseArray.GetCount())
        out[i] = baseArray[i];
    }

    ezUInt32 uiCountLeft = ezMath::Min(uiSize, leftArray.GetCount());
    for (ezUInt32 i = 0; i < uiCountLeft; i++)
    {
      if (leftArray[i] != baseArray[i])
        out[i] = leftArray[i];
    }

    ezUInt32 uiCountRight = ezMath::Min(uiSize, rightArray.GetCount());
    for (ezUInt32 i = 0; i < uiCountRight; i++)
    {
      if (rightArray[i] != baseArray[i])
        out[i] = rightArray[i];
    }
    return;
  }

  // Move distance is NP-complete so try greedy algorithm
  struct Element
  {
    Element(const ezVariant* pValue = nullptr, ezInt32 iBaseIndex = -1, ezInt32 iLeftIndex = -1, ezInt32 iRightIndex = -1)
      : m_pValue(pValue), m_iBaseIndex(iBaseIndex), m_iLeftIndex(iLeftIndex), m_iRightIndex(iRightIndex), m_fIndex(ezMath::BasicType<float>::MaxValue()) {}
    bool IsDeleted() const
    {
      return m_iBaseIndex != -1 && (m_iLeftIndex == -1 || m_iRightIndex == -1);
    }
    bool operator < (const Element& rhs) const
    {
      return m_fIndex < rhs.m_fIndex;
    }

    const ezVariant* m_pValue;
    ezInt32 m_iBaseIndex;
    ezInt32 m_iLeftIndex;
    ezInt32 m_iRightIndex;
    float m_fIndex;
  };
  ezDynamicArray<Element> baseOrder;
  baseOrder.Reserve(leftArray.GetCount() + rightArray.GetCount());

  // First, add up all unique elements and their position in each array.
  for (ezInt32 i = 0; i < (ezInt32)baseArray.GetCount(); i++)
  {
    baseOrder.PushBack(Element(&baseArray[i], i));
    baseOrder.PeekBack().m_fIndex = (float)i;
  }

  ezDynamicArray<ezInt32> leftOrder;
  leftOrder.SetCount(leftArray.GetCount());
  for (ezInt32 i = 0; i < (ezInt32)leftArray.GetCount(); i++)
  {
    const ezVariant& val = leftArray[i];
    bool bFound = false;
    for (ezInt32 j = 0; j < (ezInt32)baseOrder.GetCount(); j++)
    {
      Element& elem = baseOrder[j];
      if (elem.m_iLeftIndex == -1 && *elem.m_pValue == val)
      {
        elem.m_iLeftIndex = i;
        leftOrder[i] = j;
        bFound = true;
        break;
      }
    }

    if (!bFound)
    {
      // Added element.
      leftOrder[i] = (ezInt32)baseOrder.GetCount();
      baseOrder.PushBack(Element(&leftArray[i], -1, i));
    }
  }

  ezDynamicArray<ezInt32> rightOrder;
  rightOrder.SetCount(rightArray.GetCount());
  for (ezInt32 i = 0; i < (ezInt32)rightArray.GetCount(); i++)
  {
    const ezVariant& val = rightArray[i];
    bool bFound = false;
    for (ezInt32 j = 0; j < (ezInt32)baseOrder.GetCount(); j++)
    {
      Element& elem = baseOrder[j];
      if (elem.m_iRightIndex == -1 && *elem.m_pValue == val)
      {
        elem.m_iRightIndex = i;
        rightOrder[i] = j;
        bFound = true;
        break;
      }
    }

    if (!bFound)
    {
      // Added element.
      rightOrder[i] = (ezInt32)baseOrder.GetCount();
      baseOrder.PushBack(Element(&rightArray[i], -1, -1, i));
    }
  }

  // Re-order greedy
  float fLastElement = -0.5f;
  for (ezInt32 i = 0; i < (ezInt32)leftOrder.GetCount(); i++)
  {
    Element& currentElem = baseOrder[leftOrder[i]];
    if (currentElem.IsDeleted())
      continue;

    float fLowestSubsequent = ezMath::BasicType<float>::MaxValue();
    for (ezInt32 j = i + 1; j < (ezInt32)leftOrder.GetCount(); j++)
    {
      Element& elem = baseOrder[leftOrder[j]];
      if (elem.IsDeleted())
        continue;

      if (elem.m_iBaseIndex < fLowestSubsequent)
      {
        fLowestSubsequent = (float)elem.m_iBaseIndex;
      }
    }

    if (currentElem.m_fIndex >= fLowestSubsequent)
    {
      currentElem.m_fIndex = (fLowestSubsequent + fLastElement) / 2.0f;
    }

    fLastElement = currentElem.m_fIndex;
  }

  fLastElement = -0.5f;
  for (ezInt32 i = 0; i < (ezInt32)rightOrder.GetCount(); i++)
  {
    Element& currentElem = baseOrder[rightOrder[i]];
    if (currentElem.IsDeleted())
      continue;

    float fLowestSubsequent = ezMath::BasicType<float>::MaxValue();
    for (ezInt32 j = i + 1; j < (ezInt32)rightOrder.GetCount(); j++)
    {
      Element& elem = baseOrder[rightOrder[j]];
      if (elem.IsDeleted())
        continue;

      if (elem.m_iBaseIndex < fLowestSubsequent)
      {
        fLowestSubsequent = (float)elem.m_iBaseIndex;
      }
    }

    if (currentElem.m_fIndex >= fLowestSubsequent)
    {
      currentElem.m_fIndex = (fLowestSubsequent + fLastElement) / 2.0f;
    }

    fLastElement = currentElem.m_fIndex;
  }


  // Sort
  baseOrder.Sort();
  out.Reserve(baseOrder.GetCount());
  for (const Element& elem : baseOrder)
  {
    if (!elem.IsDeleted())
    {
      out.PushBack(*elem.m_pValue);
    }
  }
}

EZ_STATICLINK_FILE(Foundation, Foundation_Serialization_Implementation_AbstractObjectGraph);


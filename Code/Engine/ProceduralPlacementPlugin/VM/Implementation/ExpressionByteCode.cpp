#include <PCH.h>
#include <ProceduralPlacementPlugin/VM/ExpressionByteCode.h>
#include <ProceduralPlacementPlugin/VM/ExpressionFunctions.h>
#include <Foundation/IO/ChunkStream.h>

ezExpressionByteCode::ezExpressionByteCode()
  : m_uiNumInstructions(0)
  , m_uiNumTempRegisters(0)
  , m_uiNumInputs(0)
  , m_uiNumOutputs(0)
{

}

ezExpressionByteCode::~ezExpressionByteCode()
{

}

namespace
{
  static const char* s_szOpCodeNames[] =
  {
    // Unary
    "",

    "Abs_R",
    "Sqrt_R",

    "Mov_R",
    "Mov_C",
    "Mov_I",
    "Mov_O",

    "",

    // Binary
    "",

    "Add_RR",
    "Add_CR",

    "Sub_RR",
    "Sub_CR",

    "Mul_RR",
    "Mul_CR",

    "Div_RR",
    "Div_CR",

    "Min_RR",
    "Min_CR",

    "Max_RR",
    "Max_CR",

    "",

    "Call",
  };

  EZ_CHECK_AT_COMPILETIME_MSG(EZ_ARRAY_SIZE(s_szOpCodeNames) == ezExpressionByteCode::OpCode::Count, "OpCode name array size does not match OpCode type count");

  static bool FirstArgIsConstant(ezExpressionByteCode::OpCode::Enum opCode)
  {
    return opCode == ezExpressionByteCode::OpCode::Mov_C ||
      opCode == ezExpressionByteCode::OpCode::Add_CR ||
      opCode == ezExpressionByteCode::OpCode::Sub_CR ||
      opCode == ezExpressionByteCode::OpCode::Mul_CR ||
      opCode == ezExpressionByteCode::OpCode::Div_CR ||
      opCode == ezExpressionByteCode::OpCode::Min_CR ||
      opCode == ezExpressionByteCode::OpCode::Max_CR;
  }
}

void ezExpressionByteCode::Disassemble(ezStringBuilder& out_sDisassembly) const
{
  out_sDisassembly.AppendFormat("// Inputs: {0}\n", m_uiNumInputs);
  out_sDisassembly.AppendFormat("// Outputs: {0}\n", m_uiNumOutputs);
  out_sDisassembly.AppendFormat("// Temp Registers: {0}\n", m_uiNumTempRegisters);
  out_sDisassembly.AppendFormat("// Instructions: {0}\n\n", m_uiNumInstructions);


  const StorageType* pByteCode = GetByteCode();
  const StorageType* pByteCodeEnd = GetByteCodeEnd();

  while (pByteCode < pByteCodeEnd)
  {
    OpCode::Enum opCode = GetOpCode(pByteCode);
    const char* szOpCode = s_szOpCodeNames[opCode];

    if (opCode > OpCode::FirstUnary && opCode < OpCode::LastUnary)
    {
      ezUInt32 r = GetRegisterIndex(pByteCode, 1);
      ezUInt32 x = GetRegisterIndex(pByteCode, 1);

      if (FirstArgIsConstant(opCode))
      {
        out_sDisassembly.AppendFormat("{0} r{1} {2}\n", szOpCode, r, ezArgF(*reinterpret_cast<float*>(&x), 6));
      }
      else
      {
        const char* szR = (opCode == OpCode::Mov_O) ? "o" : "r";
        const char* szX = (opCode == OpCode::Mov_I) ? "i" : "r";

        out_sDisassembly.AppendFormat("{0} {1}{2} {3}{4}\n", szOpCode, szR, r, szX, x);
      }
    }
    else if (opCode > OpCode::FirstBinary && opCode < OpCode::LastBinary)
    {
      ezUInt32 r = GetRegisterIndex(pByteCode, 1);
      ezUInt32 a = GetRegisterIndex(pByteCode, 1);
      ezUInt32 b = GetRegisterIndex(pByteCode, 1);

      if (FirstArgIsConstant(opCode))
      {
        out_sDisassembly.AppendFormat("{0} r{1} {2} r{3}\n", szOpCode, r, ezArgF(*reinterpret_cast<float*>(&a), 6), b);
      }
      else
      {
        out_sDisassembly.AppendFormat("{0} r{1} r{2} r{3}\n", szOpCode, r, a, b);
      }
    }
    else if (opCode == OpCode::Call)
    {
      ezUInt32 uiNameHash = GetFunctionNameHash(pByteCode);
      const char* szName = ezExpressionFunctionRegistry::GetName(uiNameHash);

      ezStringBuilder sName;
      if (ezStringUtils::IsNullOrEmpty(szName))
      {
        sName.Format("Unknown_{0}", ezArgU(uiNameHash, 8, true, 16));
      }
      else
      {
        sName = szName;
      }

      ezUInt32 r = GetRegisterIndex(pByteCode, 1);

      out_sDisassembly.AppendFormat("{0} {1} r{2}", szOpCode, sName, r);

      ezUInt32 uiNumArgs = GetFunctionArgCount(pByteCode);
      for (ezUInt32 uiArgIndex = 0; uiArgIndex < uiNumArgs; ++uiArgIndex)
      {
        ezUInt32 x = GetRegisterIndex(pByteCode, 1);
        out_sDisassembly.AppendFormat(" r{0}", x);
      }

      out_sDisassembly.Append("\n");
    }
    else
    {
      EZ_ASSERT_NOT_IMPLEMENTED;
    }
  }
}

void ezExpressionByteCode::Save(ezStreamWriter& stream) const
{
  ezChunkStreamWriter chunk(stream);

  chunk.BeginStream();

  {
    chunk.BeginChunk("MetaData", 1);

    chunk << m_uiNumInstructions;
    chunk << m_uiNumTempRegisters;
    chunk << m_uiNumInputs;
    chunk << m_uiNumOutputs;

    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("Code", 1);

    chunk << m_ByteCode.GetCount();
    chunk.WriteBytes(m_ByteCode.GetData(), m_ByteCode.GetCount() * sizeof(StorageType));

    chunk.EndChunk();
  }

  chunk.EndStream();
}

ezResult ezExpressionByteCode::Load(ezStreamReader& stream)
{
  ezChunkStreamReader chunk(stream);
  chunk.SetEndChunkFileMode(ezChunkStreamReader::EndChunkFileMode::SkipToEnd);

  chunk.BeginStream();

  while (chunk.GetCurrentChunk().m_bValid)
  {
    if (chunk.GetCurrentChunk().m_sChunkName == "MetaData")
    {
      chunk >> m_uiNumInstructions;
      chunk >> m_uiNumTempRegisters;
      chunk >> m_uiNumInputs;
      chunk >> m_uiNumOutputs;
    }
    else if (chunk.GetCurrentChunk().m_sChunkName == "Code")
    {
      ezUInt32 uiByteCodeCount = 0;
      chunk >> uiByteCodeCount;

      m_ByteCode.SetCountUninitialized(uiByteCodeCount);
      chunk.ReadBytes(m_ByteCode.GetData(), uiByteCodeCount * sizeof(StorageType));
    }

    chunk.NextChunk();
  }

  chunk.EndStream();

  return EZ_SUCCESS;
}
#include <PCH.h>

#include <Foundation/Types/ScopeExit.h>
#include <ParticlePlugin/Type/Quad/QuadParticleRenderer.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/RenderContext/RenderContext.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleQuadRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleQuadRenderer, 1, ezRTTIDefaultAllocator<ezParticleQuadRenderer>);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleQuadRenderer::ezParticleQuadRenderer() = default;

ezParticleQuadRenderer::~ezParticleQuadRenderer()
{
  DestroyParticleDataBuffer(m_hBaseDataBuffer);
  DestroyParticleDataBuffer(m_hBillboardDataBuffer);
  DestroyParticleDataBuffer(m_hTangentDataBuffer);
}

void ezParticleQuadRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types)
{
  types.PushBack(ezGetStaticRTTI<ezParticleQuadRenderData>());
}

void ezParticleQuadRenderer::CreateDataBuffer()
{
  CreateParticleDataBuffer(m_hBaseDataBuffer, sizeof(ezBaseParticleShaderData), s_uiParticlesPerBatch);
  CreateParticleDataBuffer(m_hBillboardDataBuffer, sizeof(ezBillboardQuadParticleShaderData), s_uiParticlesPerBatch);
  CreateParticleDataBuffer(m_hTangentDataBuffer, sizeof(ezTangentQuadParticleShaderData), s_uiParticlesPerBatch);
}

void ezParticleQuadRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, ezRenderPipelinePass* pPass,
                                         const ezRenderDataBatch& batch)
{
  ezRenderContext* pRenderContext = renderViewContext.m_pRenderContext;
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezGALContext* pGALContext = pRenderContext->GetGALContext();

  TempSystemCB systemConstants(pRenderContext);

  BindParticleShader(pRenderContext, "Shaders/Particles/QuadParticle.ezShader");

  // make sure our structured buffer is allocated and bound
  {
    CreateDataBuffer();
    pRenderContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles,
                                   s_uiParticlesPerBatch * 2);

    pRenderContext->BindBuffer("particleBaseData", pDevice->GetDefaultResourceView(m_hBaseDataBuffer));
    pRenderContext->BindBuffer("particleBillboardQuadData", pDevice->GetDefaultResourceView(m_hBillboardDataBuffer));
    pRenderContext->BindBuffer("particleTangentQuadData", pDevice->GetDefaultResourceView(m_hTangentDataBuffer));
  }

  // now render all particle effects of type Quad
  for (auto it = batch.GetIterator<ezParticleQuadRenderData>(0, batch.GetCount()); it.IsValid(); ++it)
  {
    const ezParticleQuadRenderData* pRenderData = it;

    const ezBaseParticleShaderData* pParticleBaseData = pRenderData->m_BaseParticleData.GetPtr();
    const ezBillboardQuadParticleShaderData* pParticleBillboardData = pRenderData->m_BillboardParticleData.GetPtr();
    const ezTangentQuadParticleShaderData* pParticleTangentData = pRenderData->m_TangentParticleData.GetPtr();

    ezUInt32 uiNumParticles = pRenderData->m_BaseParticleData.GetCount();

    pRenderContext->BindTexture2D("ParticleTexture", pRenderData->m_hTexture);

    ConfigureRenderMode(pRenderData, pRenderContext);

    systemConstants.SetGenericData(pRenderData->m_bApplyObjectTransform, pRenderData->m_GlobalTransform, pRenderData->m_uiNumVariationsX,
                                   pRenderData->m_uiNumVariationsY, pRenderData->m_uiNumFlipbookAnimationsX,
                                   pRenderData->m_uiNumFlipbookAnimationsY, pRenderData->m_fDistortionStrength);

    if (pParticleBillboardData != nullptr)
      pRenderContext->SetShaderPermutationVariable("PARTICLE_QUAD_MODE", "PARTICLE_QUAD_MODE_BILLBOARD");
    else if (pParticleTangentData != nullptr)
      pRenderContext->SetShaderPermutationVariable("PARTICLE_QUAD_MODE", "PARTICLE_QUAD_MODE_TANGENTS");

    while (uiNumParticles > 0)
    {
      // upload this batch of particle data
      const ezUInt32 uiNumParticlesInBatch = ezMath::Min<ezUInt32>(uiNumParticles, s_uiParticlesPerBatch);
      uiNumParticles -= uiNumParticlesInBatch;

      pGALContext->UpdateBuffer(m_hBaseDataBuffer, 0, ezMakeArrayPtr(pParticleBaseData, uiNumParticlesInBatch).ToByteArray());
      pParticleBaseData += uiNumParticlesInBatch;

      if (pParticleBillboardData != nullptr)
      {
        pGALContext->UpdateBuffer(m_hBillboardDataBuffer, 0, ezMakeArrayPtr(pParticleBillboardData, uiNumParticlesInBatch).ToByteArray());
        pParticleBillboardData += uiNumParticlesInBatch;
      }

      if (pParticleTangentData != nullptr)
      {
        pGALContext->UpdateBuffer(m_hTangentDataBuffer, 0, ezMakeArrayPtr(pParticleTangentData, uiNumParticlesInBatch).ToByteArray());
        pParticleTangentData += uiNumParticlesInBatch;
      }

      // do one drawcall
      renderViewContext.m_pRenderContext->DrawMeshBuffer(uiNumParticlesInBatch * 2);
    }
  }
}

void ezParticleQuadRenderer::ConfigureRenderMode(const ezParticleQuadRenderData* pRenderData, ezRenderContext* pRenderContext)
{
  switch (pRenderData->m_RenderMode)
  {
    case ezParticleTypeRenderMode::Additive:
      pRenderContext->SetShaderPermutationVariable("PARTICLE_RENDER_MODE", "PARTICLE_RENDER_MODE_ADDITIVE");
      break;
    case ezParticleTypeRenderMode::Blended:
      pRenderContext->SetShaderPermutationVariable("PARTICLE_RENDER_MODE", "PARTICLE_RENDER_MODE_BLENDED");
      break;
    case ezParticleTypeRenderMode::Opaque:
      pRenderContext->SetShaderPermutationVariable("PARTICLE_RENDER_MODE", "PARTICLE_RENDER_MODE_OPAQUE");
      break;
    case ezParticleTypeRenderMode::Distortion:
      pRenderContext->SetShaderPermutationVariable("PARTICLE_RENDER_MODE", "PARTICLE_RENDER_MODE_DISTORTION");
      pRenderContext->BindTexture2D("ParticleDistortionTexture", pRenderData->m_hDistortionTexture);
      break;
  }
}

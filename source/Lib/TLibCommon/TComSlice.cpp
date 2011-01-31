/* ====================================================================================================================

  The copyright in this software is being made available under the License included below.
  This software may be subject to other third party and   contributor rights, including patent rights, and no such
  rights are granted under this license.

  Copyright (c) 2010, SAMSUNG ELECTRONICS CO., LTD. and BRITISH BROADCASTING CORPORATION
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, are permitted only for
  the purpose of developing standards within the Joint Collaborative Team on Video Coding and for testing and
  promoting such standards. The following conditions are required to be met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
      the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
      the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of SAMSUNG ELECTRONICS CO., LTD. nor the name of the BRITISH BROADCASTING CORPORATION
      may be used to endorse or promote products derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 * ====================================================================================================================
*/

/** \file     TComSlice.cpp
    \brief    slice header and SPS class
*/

#include "CommonDef.h"
#include "TComSlice.h"
#include "TComPic.h"

TComSlice::TComSlice()
{
  m_iPOC                = 0;
  m_eSliceType          = I_SLICE;
  m_iSliceQp            = 0;
  m_iSymbolMode         = 1;
  m_aiNumRefIdx[0]      = 0;
  m_aiNumRefIdx[1]      = 0;
  m_bLoopFilterDisable  = false;
  
  m_bDRBFlag            = true;
  m_eERBIndex           = ERB_NONE;
  
  m_iSliceQpDelta       = 0;
  
  m_iDepth              = 0;
  
  m_pcPic               = NULL;
  m_bRefenced           = false;
#ifdef ROUNDING_CONTROL_BIPRED
  m_bRounding           = false;
#endif
  m_uiColDir = 0;
  
  initEqualRef();
#if MS_NO_BACK_PRED_IN_B0
  m_bNoBackPredFlag = false;
#endif
}

TComSlice::~TComSlice()
{
}


Void TComSlice::initSlice()
{
  m_aiNumRefIdx[0]      = 0;
  m_aiNumRefIdx[1]      = 0;
  
  m_bDRBFlag            = true;
  m_eERBIndex           = ERB_NONE;
  
  m_iInterpFilterType   = IPF_SAMSUNG_DIF_DEFAULT;
  
  m_uiColDir = 0;
  
  initEqualRef();
#if MS_NO_BACK_PRED_IN_B0
  m_bNoBackPredFlag = false;
#endif
}

Void  TComSlice::sortPicList        (TComList<TComPic*>& rcListPic)
{
  TComPic*    pcPicExtract;
  TComPic*    pcPicInsert;
  
  TComList<TComPic*>::iterator    iterPicExtract;
  TComList<TComPic*>::iterator    iterPicExtract_1;
  TComList<TComPic*>::iterator    iterPicInsert;
  
  for (Int i = 1; i < (Int)(rcListPic.size()); i++)
  {
    iterPicExtract = rcListPic.begin();
    for (Int j = 0; j < i; j++) iterPicExtract++;
    pcPicExtract = *(iterPicExtract);
    
    iterPicInsert = rcListPic.begin();
    while (iterPicInsert != iterPicExtract)
    {
      pcPicInsert = *(iterPicInsert);
      if (pcPicInsert->getPOC() >= pcPicExtract->getPOC())
      {
        break;
      }
      
      iterPicInsert++;
    }
    
    iterPicExtract_1 = iterPicExtract;    iterPicExtract_1++;
    
    //  swap iterPicExtract and iterPicInsert, iterPicExtract = curr. / iterPicInsert = insertion position
    rcListPic.insert (iterPicInsert, iterPicExtract, iterPicExtract_1);
    rcListPic.erase  (iterPicExtract);
  }
}

// [KSI] TEncTop::m_cListPic���� POCCurr�� TComPic�� �������� LIST0, LIST1�� ��� ��
// [KSI] ������ TComPic�� ã�´�. �� ��, ERB/DRB�� �� �� �ְ� NORMAL/LTR�� �����Ͽ� �˻� ������ ������ �� �ִ�.
// [KSI] �Է�: rcListPic, bDRBFlag, uiPOCCurr, eRefPicList, uiNthRefPic
// [KSI] ���: �˻� ���ǿ� �´� TComPic*, ������ NULL
TComPic* TComSlice::xGetRefPic (TComList<TComPic*>& rcListPic,
                                Bool                bDRBFlag,
                                ERBIndex            eERBIndex,
                                UInt                uiPOCCurr,
                                RefPicList          eRefPicList,
                                UInt                uiNthRefPic)
{
  //  find current position
  TComList<TComPic*>::iterator  iterPic = rcListPic.begin();
  TComPic*                      pcRefPic   = NULL;
  
  TComPic*                      pcPic = *(iterPic);
  while ( (pcPic->getPOC() != (Int)uiPOCCurr) && (iterPic != rcListPic.end()) )
  {
    iterPic++;
    pcPic = *(iterPic);
  }
  assert (pcPic->getPOC() == (Int)uiPOCCurr);
  
  //  find n-th reference picture with bDRBFlag and eERBIndex
  UInt  uiCount = 0;
  
  if( eRefPicList == REF_PIC_LIST_0 )
  {
	// [KSI] ���� POC�� TComPic���� �ڷ� �̵��ϸ鼭 ������ �������� ������.
    while(1)
    {
	  // [KSI] �� ó������ ���� Ż�� ����.
      if (iterPic == rcListPic.begin())
        break;
      
	  // [KSI] �ڷ� �̵��Ѵ�.
      iterPic--;
      pcPic = *(iterPic);

	  // [KSI] ���ſ� ENC/RECON�� �Ǿ��� bDRBFlag�� ���� ���̰� eERBIndex�� ������ �˻�.
	  // [KSI] �� ���� ������ ��� �����ϸ� �ش� �������� ����� �� �ִ�.
      if( ( !pcPic->getReconMark()                        ) ||
         ( bDRBFlag  != pcPic->getSlice()->getDRBFlag()  ) ||
         ( eERBIndex != pcPic->getSlice()->getERBIndex() ) )
        continue;
      
	  // [KSI] ���� ������ ����ص�, REF������ �������� �ʾ����� �ٸ� �������� �����Ѵ�.
      if( !pcPic->getSlice()->isReferenced() )
        continue;
      
	  // [KSI] ã�� �������� ������ ī��Ʈ �ϰ�, �� ���� ���ϴ� ��ġ�� ���ٸ� �����Ѵ�.
      uiCount++;
      if (uiCount == uiNthRefPic)
      {
        pcRefPic = pcPic;
        return  pcRefPic;
      }
    }
    
	// [KSI] ���� while loop���� �� ó������ �������� �ҵ��� ���� ��� ���⸦ �����Ѵ�.
	// [KSI] �� ���� Low Delay Coding�� ���� �Ǿ� ���� �ʾƾ� �Ѵ�.
    if ( !m_pcSPS->getUseLDC() )
    {
      // [KSI] �ٽ� ���� ��ġ�� ã�´�.
      iterPic = rcListPic.begin();
      pcPic = *(iterPic);
      while ( (pcPic->getPOC() != (Int)uiPOCCurr) && (iterPic != rcListPic.end()) )
      {
        iterPic++;
        pcPic = *(iterPic);
      }
      assert (pcPic->getPOC() == (Int)uiPOCCurr);
      
	  // [KSI] ������ POC�� TComPic���� ������ �̵��ϸ鼭 �̷��� �������� ������.
	  // [KSI] �ռ��� ���� ������ �������� ã�´�.
      while(1)
      {
        iterPic++;
        if (iterPic == rcListPic.end())
          break;
        
        pcPic = *(iterPic);
        if( ( !pcPic->getReconMark()                        ) ||
           ( bDRBFlag  != pcPic->getSlice()->getDRBFlag()  ) ||
           ( eERBIndex != pcPic->getSlice()->getERBIndex() ) )
          continue;
        
        if( !pcPic->getSlice()->isReferenced() )
          continue;
        
        uiCount++;
        if (uiCount == uiNthRefPic)
        {
          pcRefPic = pcPic;
          return  pcRefPic;
        }
      }
    }
	// [KSI] �׷��� ������ NULL�� �����Ѵ�.
  }
  else
  {
	// [KSI] ���⿡���� ���� ��Ȯ�� �ݴ� �������� ����Ʈ�� ������.
    while(1)
    {
      iterPic++;
      if (iterPic == rcListPic.end())
        break;
      
      pcPic = *(iterPic);
      if( ( !pcPic->getReconMark()                        ) ||
         ( bDRBFlag  != pcPic->getSlice()->getDRBFlag()  ) ||
         ( eERBIndex != pcPic->getSlice()->getERBIndex() ) )
        continue;
      
      if( !pcPic->getSlice()->isReferenced() )
        continue;
      
      uiCount++;
      if (uiCount == uiNthRefPic)
      {
        pcRefPic = pcPic;
        return  pcRefPic;
      }
    }
    
    iterPic = rcListPic.begin();
    pcPic = *(iterPic);
    while ( (pcPic->getPOC() != (Int)uiPOCCurr) && (iterPic != rcListPic.end()) )
    {
      iterPic++;
      pcPic = *(iterPic);
    }
    assert (pcPic->getPOC() == (Int)uiPOCCurr);
    
    while(1)
    {
      if (iterPic == rcListPic.begin())
        break;
      
      iterPic--;
      pcPic = *(iterPic);
      if( ( !pcPic->getReconMark()                        ) ||
         ( bDRBFlag  != pcPic->getSlice()->getDRBFlag()  ) ||
         ( eERBIndex != pcPic->getSlice()->getERBIndex() ) )
        continue;
      
      if( !pcPic->getSlice()->isReferenced() )
        continue;
      
      uiCount++;
      if (uiCount == uiNthRefPic)
      {
        pcRefPic = pcPic;
        return  pcRefPic;
      }
    }
  }
  
  return  pcRefPic;
}

Void TComSlice::setRefPOCList       ()
{
  for (Int iDir = 0; iDir < 2; iDir++)
  {
    for (Int iNumRefIdx = 0; iNumRefIdx < m_aiNumRefIdx[iDir]; iNumRefIdx++)
    {
      m_aiRefPOCList[iDir][iNumRefIdx] = m_apcRefPicList[iDir][iNumRefIdx]->getPOC();
    }
  }
}

// [KSI] ���⿡�� Multiview Referecne Picture�� pick up �� ���� RefL0/RefL1�� �����Ѵ�.
Void TComSlice::setRefPicList       ( TComList<TComPic*>& rcListPic )
{
  if (m_eSliceType == I_SLICE)
  {
    ::memset( m_apcRefPicList, 0, sizeof (m_apcRefPicList));
    ::memset( m_aiNumRefIdx,   0, sizeof ( m_aiNumRefIdx ));
    
    return;
  }
  
  // ����, Reference Picture List�� Item ������ ���� �Ѵ�.
  m_aiNumRefIdx[0] = Min ( m_aiNumRefIdx[0], (Int)(rcListPic.size())-1 );
  m_aiNumRefIdx[1] = Min ( m_aiNumRefIdx[1], (Int)(rcListPic.size())-1 );
  
  sortPicList(rcListPic);
  
  TComPic*  pcRefPic;
  for (Int n = 0; n < 2; n++)
  {
    RefPicList  eRefPicList = (RefPicList)n;
    
    UInt  uiOrderDRB  = 1;
    UInt  uiOrderERB  = 1;
    Int   iRefIdx     = 0;
    UInt  uiActualListSize = 0;
    
    if ( m_eSliceType == P_SLICE && eRefPicList == REF_PIC_LIST_1 )
    {
      m_aiNumRefIdx[eRefPicList] = 0;
      ::memset( m_apcRefPicList[eRefPicList], 0, sizeof(m_apcRefPicList[eRefPicList]));
      break;
    }
    
    //  First DRB
    pcRefPic = xGetRefPic(rcListPic, true, ERB_NONE, m_iPOC, eRefPicList, uiOrderDRB);
    if (pcRefPic != NULL)
    {
      m_apcRefPicList[eRefPicList][iRefIdx] = pcRefPic;
      
      pcRefPic->getPicYuvRec()->extendPicBorder();
      
      iRefIdx++;
      uiOrderDRB++;
      uiActualListSize++;
    }
    
    if ( (Int)uiActualListSize >= m_aiNumRefIdx[eRefPicList] )
    {
      m_aiNumRefIdx[eRefPicList] = uiActualListSize;
      continue;
    }
    
    // Long term reference
    // Should be enabled to support long term refernce
    //*
    //  First ERB
    pcRefPic = xGetRefPic(rcListPic, false, ERB_LTR, m_iPOC, eRefPicList, uiOrderERB);
    if (pcRefPic != NULL)
    {
      Bool  bChangeDrbErb = false;
      if      (iRefIdx > 0 && eRefPicList == REF_PIC_LIST_0 && pcRefPic->getPOC() > m_apcRefPicList[eRefPicList][iRefIdx-1]->getPOC())
      {
        bChangeDrbErb = true;
      }
      else if (iRefIdx > 0 && eRefPicList == REF_PIC_LIST_1 && pcRefPic->getPOC() < m_apcRefPicList[eRefPicList][iRefIdx-1]->getPOC())
      {
        bChangeDrbErb = true;
      }
      
      if ( bChangeDrbErb)
      {
        m_apcRefPicList[eRefPicList][iRefIdx]   = m_apcRefPicList[eRefPicList][iRefIdx-1];
        m_apcRefPicList[eRefPicList][iRefIdx-1] = pcRefPic;
      }
      else
      {
        m_apcRefPicList[eRefPicList][iRefIdx] = pcRefPic;
      }
      
      pcRefPic->getPicYuvRec()->extendPicBorder();
      
      iRefIdx++;
      uiOrderERB++;
      uiActualListSize++;
    }
    //*/
    
    // Short term reference
    //  Residue DRB
    UInt  uiBreakCount = 17 - iRefIdx;
    while (1)
    {
      uiBreakCount--;
      if ( (Int)uiActualListSize >= m_aiNumRefIdx[eRefPicList] || uiBreakCount == 0 )
      {
        break;
      }
      
      pcRefPic = xGetRefPic(rcListPic, true, ERB_NONE, m_iPOC, eRefPicList, uiOrderDRB);
      if (pcRefPic != NULL)
      {
        uiOrderDRB++;
      }
      
      if (pcRefPic != NULL)
      {
        m_apcRefPicList[eRefPicList][iRefIdx] = pcRefPic;
        
        pcRefPic->getPicYuvRec()->extendPicBorder();
        
        iRefIdx++;
        uiActualListSize++;
      }
    }
    
    m_aiNumRefIdx[eRefPicList] = uiActualListSize;
  }
}

Void TComSlice::initEqualRef()
{
  for (Int iDir = 0; iDir < 2; iDir++)
  {
    for (Int iRefIdx1 = 0; iRefIdx1 < MAX_NUM_REF; iRefIdx1++)
    {
      for (Int iRefIdx2 = iRefIdx1; iRefIdx2 < MAX_NUM_REF; iRefIdx2++)
      {
        m_abEqualRef[iDir][iRefIdx1][iRefIdx2] = m_abEqualRef[iDir][iRefIdx2][iRefIdx1] = (iRefIdx1 == iRefIdx2? true : false);
      }
    }
  }
}

// ------------------------------------------------------------------------------------------------
// Sequence parameter set (SPS)
// ------------------------------------------------------------------------------------------------

TComSPS::TComSPS()
{
  // Structure
  m_uiWidth       = 352;
  m_uiHeight      = 288;
  m_uiMaxCUWidth  = 32;
  m_uiMaxCUHeight = 32;
  m_uiMaxCUDepth  = 3;
  m_uiMinTrDepth  = 0;
  m_uiMaxTrDepth  = 1;
  m_uiMaxTrSize   = 32;
  
  // Tool list
  m_bUseALF       = false;
  m_bUseDQP       = false;
  
#if HHI_MRG
  m_bUseMRG      = false; // SOPH:
#endif
  
  // AMVP parameter
  ::memset( m_aeAMVPMode, 0, sizeof( m_aeAMVPMode ) );

  // [KSI] Multiview
  //{
  m_bMVC                    = false;                          // flag for using MVC
  m_uiNumViewsMinusOne      = 0;                              // number of view minus one
  m_auiViewOrder            = NULL;                           // i : view object index, a[i] : view object id
  m_auiNumAnchorRefsL0      = NULL;                           // i : view object index, a[i] : number of anchor view objects at L0
  m_auiNumAnchorRefsL1      = NULL;                           // i : view object index, a[i] : number of anchor view objects at L1
  m_aauiAnchorRefL0         = NULL;                           // i : view object index, j : reference order, a[i][j] : anchor view object id at L0
  m_aauiAnchorRefL1         = NULL;                           // i : view object index, j : reference order, a[i][j] : anchor view object id at L1
  m_auiNumNonAnchorRefsL0   = NULL;                           // i : view object index, a[i] : number of non-anchor view objects at L0
  m_auiNumNonAnchorRefsL1   = NULL;                           // i : view object index, a[i] : number of non-anchor view objects at L1
  m_aauiNonAnchorRefL0      = NULL;                           // i : view object index, j : reference order, a[i][j] : non-anchor view object id at L0
  m_aauiNonAnchorRefL1      = NULL;                           // i : view object index, j : reference order, a[i][j] : non-anchor view object id at L1
  //}
}

TComSPS::~TComSPS()
{
}

TComPPS::TComPPS()
{
}

TComPPS::~TComPPS()
{
}


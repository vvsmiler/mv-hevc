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

/** \file     TDecTop.cpp
    \brief    decoder class
*/

#include "TDecTop.h"

TDecTop::TDecTop()
{
  m_iGopSize      = 0;
  m_bGopSizeSet   = false;
  m_iMaxRefPicNum = 0;
  m_uiValidPS = 0;
  //{ [KSI] - MVC
  m_acListPic = NULL;
  //} [KSI] - ~MVC
#if ENC_DEC_TRACE
  g_hTrace = fopen( "TraceDec.txt", "wb" );
  g_bJustDoIt = g_bEncDecTraceDisable;
  g_nSymbolCounter = 0;
#endif
}

TDecTop::~TDecTop()
{
#if ENC_DEC_TRACE
  fclose( g_hTrace );
#endif
}

Void TDecTop::create()
{
  m_cGopDecoder.create();
  m_apcSlicePilot = new TComSlice;
}

Void TDecTop::destroy()
{
  m_cGopDecoder.destroy();
  
  delete m_apcSlicePilot;
  m_apcSlicePilot = NULL;
  
  m_cSliceDecoder.destroy();

  //{ [KSI] - MVC
  if ( m_acListPic != NULL ) delete [] m_acListPic;
  //} [KSI] - ~MVC
}

Void TDecTop::init()
{
  // initialize ROM
  initROM();
  
  m_cGopDecoder.  init( &m_cEntropyDecoder, &m_cSbacDecoder, &m_cBinCABAC, &m_cCavlcDecoder, &m_cSliceDecoder, &m_cLoopFilter, &m_cAdaptiveLoopFilter );
  m_cSliceDecoder.init( &m_cEntropyDecoder, &m_cCuDecoder );
  m_cEntropyDecoder.init(&m_cPrediction);
}

Void TDecTop::deletePicBuffer ( )
{
  //{ [KSI] - MVC
  for ( UInt idx = 0; idx <= m_cSubsetSPS.getNumViewsMinusOne(); idx++ )
  {
    TComList<TComPic*>::iterator  iterPic   = m_acListPic[idx].begin();
	Int iSize = Int( m_acListPic[idx].size() );
	
	for (Int i = 0; i < iSize; i++ )
	{
	  TComPic* pcPic = *(iterPic++);
	  pcPic->destroy();
	  
	  delete pcPic;
	  pcPic = NULL;
    }
  }
  //} [KSI] - ~MVC

  // destroy ALF temporary buffers
  m_cAdaptiveLoopFilter.destroy();
  
  m_cLoopFilter.        destroy();
  
  // destroy ROM
  destroyROM();
}

Void TDecTop::xUpdateGopSize (TComSlice* pcSlice)
{
  if ( (pcSlice->getPOC()!=0) && pcSlice->isIntra() && !m_bGopSizeSet)
  {
    m_iGopSize    = pcSlice->getPOC();
    m_bGopSizeSet = true;
    
    m_cGopDecoder.setGopSize(m_iGopSize);

	//{ [KSI] - MVC
	m_cMultiView.setGOPSize(m_iGopSize);
	//}
  }
}

//{ [KSI] - MVC
UInt TDecTop::xGetViewIndex(TComSlice* pcSlice)
{
	UInt i;
	for ( i = 0; i <= pcSlice->getSPS()->getNumViewsMinusOne(); i++ )
	{
		if ( pcSlice->getSPS()->getViewOrder()[i] == pcSlice->getViewId() )
			break;
	}
	return i;
}
//} [KSI] - ~MVC

//{ [KSI] - MVC
Void TDecTop::xPrepareInterViewPrediction(TComSlice* pcSlice)
{
	if ( pcSlice->getSPS()->getMVC() && pcSlice->getInterViewFlag() )
	{
		UInt uiViewIndex = xGetViewIndex(pcSlice);
		if ( pcSlice->getAnchorPicFlag() )
		{
			pcSlice->setNumRefIdx(REF_PIC_LIST_0, (pcSlice->getNumRefIdx(REF_PIC_LIST_0) - pcSlice->getSPS()->getNumAnchorRefsL0()[uiViewIndex]));
			assert(pcSlice->getNumRefIdx(REF_PIC_LIST_0) >= 0);
			pcSlice->setNumRefIdx(REF_PIC_LIST_1, (pcSlice->getNumRefIdx(REF_PIC_LIST_1) - pcSlice->getSPS()->getNumAnchorRefsL1()[uiViewIndex]));
			assert(pcSlice->getNumRefIdx(REF_PIC_LIST_1) >= 0);
		}
		else
		{
			pcSlice->setNumRefIdx(REF_PIC_LIST_0, (pcSlice->getNumRefIdx(REF_PIC_LIST_0) - pcSlice->getSPS()->getNumNonAnchorRefsL0()[uiViewIndex]));
			assert(pcSlice->getNumRefIdx(REF_PIC_LIST_0) >= 0);
			pcSlice->setNumRefIdx(REF_PIC_LIST_1, (pcSlice->getNumRefIdx(REF_PIC_LIST_1) - pcSlice->getSPS()->getNumNonAnchorRefsL1()[uiViewIndex]));
			assert(pcSlice->getNumRefIdx(REF_PIC_LIST_1) >= 0);
		}
	}
}
//} [KSI] - ~MVC

//{ [KSI] - MVC
Void TDecTop::xSetInterViewRefPicList(TComSlice* pcSlice)
{
	if ( pcSlice->getSPS()->getMVC() && pcSlice->getInterViewFlag() )
	{
		UInt uiViewIndex = xGetViewIndex(pcSlice);
		UInt uiNumRefsL0 = pcSlice->getAnchorPicFlag() ? pcSlice->getSPS()->getNumAnchorRefsL0()[uiViewIndex] : pcSlice->getSPS()->getNumNonAnchorRefsL0()[uiViewIndex];
		UInt uiNumRefsL1 = pcSlice->getAnchorPicFlag() ? pcSlice->getSPS()->getNumAnchorRefsL1()[uiViewIndex] : pcSlice->getSPS()->getNumNonAnchorRefsL1()[uiViewIndex];

		if ( pcSlice->getAnchorPicFlag() )
		{
			if ( pcSlice->getNumRefIdx(REF_PIC_LIST_0) != 0 ) pcSlice->setNumRefIdx(REF_PIC_LIST_0, 0);
			if ( pcSlice->getNumRefIdx(REF_PIC_LIST_1) != 0 ) pcSlice->setNumRefIdx(REF_PIC_LIST_1, 0);
		}

		for ( UInt j = 0; j < uiNumRefsL0; j++ )
		{
			UInt idx;
			for ( idx = 0; idx <= pcSlice->getSPS()->getNumViewsMinusOne(); idx++ )
			{
				UInt uiRefViewID = pcSlice->getAnchorPicFlag() ? pcSlice->getSPS()->getAnchorRefL0()[uiViewIndex][j] : pcSlice->getSPS()->getNonAnchorRefL0()[uiViewIndex][j];
				if ( pcSlice->getSPS()->getViewOrder()[idx] == uiRefViewID )
					break;
			}

			pcSlice->setRefPic(m_cMultiView.getMultiViewPicture(idx, pcSlice->getPOC()), REF_PIC_LIST_0, pcSlice->getNumRefIdx(REF_PIC_LIST_0));
			pcSlice->setNumRefIdx(REF_PIC_LIST_0, pcSlice->getNumRefIdx(REF_PIC_LIST_0)+1);
		}

		for ( UInt j = 0; j < uiNumRefsL1; j++ )
		{
			UInt idx;
			for ( idx = 0; idx <= pcSlice->getSPS()->getNumViewsMinusOne(); idx++ )
			{
				UInt uiRefViewID = pcSlice->getAnchorPicFlag() ? pcSlice->getSPS()->getAnchorRefL1()[uiViewIndex][j] : pcSlice->getSPS()->getNonAnchorRefL1()[uiViewIndex][j];
				if ( pcSlice->getSPS()->getViewOrder()[idx] == uiRefViewID )
					break;
			}

			pcSlice->setRefPic(m_cMultiView.getMultiViewPicture(idx, pcSlice->getPOC()), REF_PIC_LIST_1, pcSlice->getNumRefIdx(REF_PIC_LIST_1));
			pcSlice->setNumRefIdx(REF_PIC_LIST_1, pcSlice->getNumRefIdx(REF_PIC_LIST_1)+1);
		}

	}
}
//} [KSI] ~MVC

Void TDecTop::xGetNewPicBuffer ( TComSlice* pcSlice, TComPic*& rpcPic )
{
  xUpdateGopSize(pcSlice);

  //{ [KSI] - MVC
  UInt uiViewIndex = xGetViewIndex(pcSlice);
  //} [KSI] - ~MVC
  m_iMaxRefPicNum = Max(m_iMaxRefPicNum, Max(Max(2, pcSlice->getNumRefIdx(REF_PIC_LIST_0)+1), m_iGopSize/2 + 2 + pcSlice->getNumRefIdx(REF_PIC_LIST_0)));
  
  if (m_acListPic[uiViewIndex].size() < (UInt)m_iMaxRefPicNum)
  {
    rpcPic = new TComPic;
    
    rpcPic->create ( pcSlice->getSPS()->getWidth(), pcSlice->getSPS()->getHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth, true);
    m_acListPic[uiViewIndex].pushBack( rpcPic );
    
    return;
  }
  
  Bool bBufferIsAvailable = false;
  TComList<TComPic*>::iterator  iterPic   = m_acListPic[uiViewIndex].begin();
  while (iterPic != m_acListPic[uiViewIndex].end())
  {
    rpcPic = *(iterPic++);
    if ( rpcPic->getReconMark() == false )
    {
      bBufferIsAvailable = true;
      break;
    }
  }
  
  if ( !bBufferIsAvailable )
  {
    pcSlice->sortPicList(m_acListPic[uiViewIndex]);
    iterPic = m_acListPic[uiViewIndex].begin();
    rpcPic = *(iterPic);
    rpcPic->setReconMark(false);
    
    // mark it should be extended
    rpcPic->getPicYuvRec()->setBorderExtension(false);
  }
}

Void TDecTop::decode (Bool bEos, TComBitstream* pcBitstream, UInt& ruiPOC, TComList<TComPic*>*& rpcListPic)
{
  rpcListPic = NULL;
  TComPic*    pcPic = NULL;
  
  // Initialize entropy decoder
  m_cEntropyDecoder.setEntropyDecoder (&m_cCavlcDecoder);
  m_cEntropyDecoder.setBitstream      (pcBitstream);
  
#if HHI_NAL_UNIT_SYNTAX
  // don't feel like adding the whole chain of interface crap just to access the first byte in the buffer
  const UChar* pucBuffer = reinterpret_cast<const UChar*>(pcBitstream->getStartStream());
  const NalUnitType eNalUnitType = NalUnitType(pucBuffer[0]&31); 
  const bool bDecodeSPS   = ( NAL_UNIT_SPS == eNalUnitType );
  const bool bDecodePPS   = ( NAL_UNIT_PPS == eNalUnitType );
  const bool bDecodeSlice = ( NAL_UNIT_CODED_SLICE == eNalUnitType ) || (NAL_UNIT_CODED_SLICE_PREFIX == eNalUnitType) || (NAL_UNIT_CODED_SLICE_LAYER_EXTENSION == eNalUnitType);
  //{ [KSI] - MVC
  const bool bDecodeSubsetSPS              = (NAL_UNIT_SUBSET_SPS == eNalUnitType);
  const bool bDecodeSlicePrefix            = (NAL_UNIT_CODED_SLICE_PREFIX == eNalUnitType);
  const bool bDecodeSliceLayerExtension    = (NAL_UNIT_CODED_SLICE_LAYER_EXTENSION == eNalUnitType);
  //} [KSI] - ~MVC
#else
  const bool bDecodeSlice = true;
  bool bDecodeSPS   = false;
  bool bDecodePPS   = false;
  if( 0 == m_uiValidPS )
  {
    bDecodeSPS = bDecodePPS = true;
  }
#endif
  
  if( bDecodeSPS )
  {
    m_cEntropyDecoder.decodeSPS( &m_cSPS );
    
    // initialize DIF
    m_cPrediction.setDIFTap ( m_cSPS.getDIFTap () );
    // create ALF temporary buffer
    m_cAdaptiveLoopFilter.create( m_cSPS.getWidth(), m_cSPS.getHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
    
    m_cLoopFilter.        create( g_uiMaxCUDepth );
    m_uiValidPS |= 1;
  }

  //{ [KSI] - MVC
  if( bDecodeSubsetSPS )
  {
	m_cEntropyDecoder.decodeSubsetSPS_MVC( &m_cSubsetSPS );
	m_cMultiView.openMultiView( m_cSubsetSPS.getNumViewsMinusOne()+1, 0, m_cSubsetSPS.getWidth(), m_cSubsetSPS.getHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
	m_acListPic = new TComList<TComPic*>[m_cSubsetSPS.getNumViewsMinusOne()+1];
	m_uiValidPS |= 4;
  }
  //} [KSI] - ~MVC
  
  if( bDecodePPS )
  {
    m_cEntropyDecoder.decodePPS( &m_cPPS );
    m_uiValidPS |= 2;
  }

  if( false == bDecodeSlice )
  {
    return;
  }
 
  // make sure we already received both parameter sets
  //{ [KSI] - MVC
  if(m_cSubsetSPS.getMVC()) assert( 7 == m_uiValidPS );
  //} [KSI] - ~MVC
  else                      assert( 3 == m_uiValidPS );
  
  m_apcSlicePilot->initSlice();
  
  //  Read slice header
  //{ [KSI] - MVC
  if(m_cSubsetSPS.getMVC()) m_apcSlicePilot->setSPS( &m_cSubsetSPS );
  //} [KSI] - ~MVC
  else                      m_apcSlicePilot->setSPS( &m_cSPS );

  m_apcSlicePilot->setPPS( &m_cPPS );

  //{ [KSI] - MVC
  if( bDecodeSlicePrefix )
  {
	  m_cEntropyDecoder.decodePrefix(m_apcSlicePilot);
	  return;
  }
  //} [KSI] - ~MVC

  //{ [KSI] - MVC
  if( bDecodeSliceLayerExtension ) m_cEntropyDecoder.decodeSliceExtensionHeader(m_apcSlicePilot);
  //} [KSI] - ~MVC
  else                             m_cEntropyDecoder.decodeSliceHeader (m_apcSlicePilot);

  // Buffer initialize for prediction.
  m_cPrediction.initTempBuff();
  //  Get a new picture buffer
  xGetNewPicBuffer (m_apcSlicePilot, pcPic);
  
  // Recursive structure
  m_cCuDecoder.create ( g_uiMaxCUDepth, g_uiMaxCUWidth, g_uiMaxCUHeight );
  m_cCuDecoder.init   ( &m_cEntropyDecoder, &m_cTrQuant, &m_cPrediction );
  m_cTrQuant.init     ( g_uiMaxCUWidth, g_uiMaxCUHeight, m_apcSlicePilot->getSPS()->getMaxTrSize());
  
  m_cSliceDecoder.create( m_apcSlicePilot, m_apcSlicePilot->getSPS()->getWidth(), m_apcSlicePilot->getSPS()->getHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
  
  //  Set picture slice pointer
  TComSlice*  pcSlice = m_apcSlicePilot;
  m_apcSlicePilot = pcPic->getPicSym()->getSlice();
  pcPic->getPicSym()->setSlice(pcSlice);
  
  // Set reference list
  //{ [KSI] - MVC
  UInt uiViewIndex = xGetViewIndex(pcSlice);
  xPrepareInterViewPrediction(pcSlice);
  pcSlice->setRefPicList(m_acListPic[uiViewIndex]);
  xSetInterViewRefPicList(pcSlice);
  //} [KSI] - ~MVC
  
  // HierP + GPB case
  if ( m_cSPS.getUseLDC() && pcSlice->isInterB() )
  {
    Int iNumRefIdx = pcSlice->getNumRefIdx(REF_PIC_LIST_0);
    pcSlice->setNumRefIdx( REF_PIC_LIST_1, iNumRefIdx );
    
    for (Int iRefIdx = 0; iRefIdx < iNumRefIdx; iRefIdx++)
    {
      pcSlice->setRefPic(pcSlice->getRefPic(REF_PIC_LIST_0, iRefIdx), REF_PIC_LIST_1, iRefIdx);
    }
  }
  
  // For generalized B
  // note: maybe not existed case (always L0 is copied to L1 if L1 is empty)
  if (pcSlice->isInterB() && pcSlice->getNumRefIdx(REF_PIC_LIST_1) == 0)
  {
    Int iNumRefIdx = pcSlice->getNumRefIdx(REF_PIC_LIST_0);
    pcSlice->setNumRefIdx        ( REF_PIC_LIST_1, iNumRefIdx );
    
    for (Int iRefIdx = 0; iRefIdx < iNumRefIdx; iRefIdx++)
    {
      pcSlice->setRefPic(pcSlice->getRefPic(REF_PIC_LIST_0, iRefIdx), REF_PIC_LIST_1, iRefIdx);
    }
  }
  
  //---------------
  pcSlice->setRefPOCList();
  
#if MS_NO_BACK_PRED_IN_B0
  pcSlice->setNoBackPredFlag( false );
  if ( pcSlice->getSliceType() == B_SLICE )
  {
    if ( pcSlice->getNumRefIdx(RefPicList( 0 ) ) == pcSlice->getNumRefIdx(RefPicList( 1 ) ) )
    {
      pcSlice->setNoBackPredFlag( true );
      int i;
      for ( i=0; i < pcSlice->getNumRefIdx(RefPicList( 1 ) ); i++ )
      {
        if ( pcSlice->getRefPOC(RefPicList(1), i) != pcSlice->getRefPOC(RefPicList(0), i) ) 
        {
          pcSlice->setNoBackPredFlag( false );
          break;
        }
      }
    }
  }
#endif
  
  //  Decode a picture
  m_cGopDecoder.decompressGop ( bEos, pcBitstream, pcPic );
  
  pcSlice->sortPicList(m_acListPic[uiViewIndex]);       //  sorting for application output
  
  ruiPOC = pcPic->getSlice()->getPOC();
  
  //{ [KSI] - MVC
  rpcListPic = &m_acListPic[uiViewIndex];
  m_cMultiView.addMultiViewPicture(uiViewIndex, pcPic->getPicYuvRec(), pcPic->getPOC(), false);
  //} [KSI] - ~MVC
  
  m_cCuDecoder.destroy();
  
  return;
}


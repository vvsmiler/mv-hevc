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

/** \file     TEncTop.cpp
    \brief    encoder class
*/

#include "../TLibCommon/CommonDef.h"
#include "TEncTop.h"

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TEncTop::TEncTop()
{
  m_iPOCLast          = -1;
  m_iNumPicRcvd       =  0;
  m_uiNumAllPicCoded  =  0;
  m_pppcRDSbacCoder   =  NULL;
  m_pppcBinCoderCABAC =  NULL;
  m_cRDGoOnSbacCoder.init( &m_cRDGoOnBinCoderCABAC );
#if ENC_DEC_TRACE
  g_hTrace = fopen( "TraceEnc.txt", "wb" );
  g_bJustDoIt = g_bEncDecTraceDisable;
  g_nSymbolCounter = 0;
#endif
}

TEncTop::~TEncTop()
{
#if ENC_DEC_TRACE
  fclose( g_hTrace );
#endif
}

Void TEncTop::create ()
{
  // initialize global variables
  initROM();
  
  // create processing unit classes
  m_cGOPEncoder.        create();
  m_cSliceEncoder.      create( getSourceWidth(), getSourceHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
  m_cCuEncoder.         create( g_uiMaxCUDepth, g_uiMaxCUWidth, g_uiMaxCUHeight );
  m_cAdaptiveLoopFilter.create( getSourceWidth(), getSourceHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
  m_cLoopFilter.        create( g_uiMaxCUDepth );
  
  // if SBAC-based RD optimization is used
  if( m_bUseSBACRD )
  {
    m_pppcRDSbacCoder = new TEncSbac** [g_uiMaxCUDepth+1];
    m_pppcBinCoderCABAC = new TEncBinCABAC** [g_uiMaxCUDepth+1];
    
    for ( Int iDepth = 0; iDepth < g_uiMaxCUDepth+1; iDepth++ )
    {
      m_pppcRDSbacCoder[iDepth] = new TEncSbac* [CI_NUM];
      m_pppcBinCoderCABAC[iDepth] = new TEncBinCABAC* [CI_NUM];
      
      for (Int iCIIdx = 0; iCIIdx < CI_NUM; iCIIdx ++ )
      {
        m_pppcRDSbacCoder[iDepth][iCIIdx] = new TEncSbac;
        m_pppcBinCoderCABAC [iDepth][iCIIdx] = new TEncBinCABAC;
        m_pppcRDSbacCoder   [iDepth][iCIIdx]->init( m_pppcBinCoderCABAC [iDepth][iCIIdx] );
      }
    }
  }
}

Void TEncTop::destroy ()
{
  // destroy processing unit classes
  m_cGOPEncoder.        destroy();
  m_cSliceEncoder.      destroy();
  m_cCuEncoder.         destroy();
  m_cAdaptiveLoopFilter.destroy();
  m_cLoopFilter.        destroy();
  
  // SBAC RD
  if( m_bUseSBACRD )
  {
    Int iDepth;
    for ( iDepth = 0; iDepth < g_uiMaxCUDepth+1; iDepth++ )
    {
      for (Int iCIIdx = 0; iCIIdx < CI_NUM; iCIIdx ++ )
      {
        delete m_pppcRDSbacCoder[iDepth][iCIIdx];
        delete m_pppcBinCoderCABAC[iDepth][iCIIdx];
      }
    }
    
    for ( iDepth = 0; iDepth < g_uiMaxCUDepth+1; iDepth++ )
    {
      delete [] m_pppcRDSbacCoder[iDepth];
      delete [] m_pppcBinCoderCABAC[iDepth];
    }
    
    delete [] m_pppcRDSbacCoder;
    delete [] m_pppcBinCoderCABAC;
  }
  
  // destroy ROM
  destroyROM();
  
  return;
}

Void TEncTop::init()
{
  UInt *aTable4=NULL, *aTable8=NULL;
  // initialize SPS
  xInitSPS();
  
  // initialize processing unit classes
  m_cGOPEncoder.  init( this );
  m_cSliceEncoder.init( this );
  m_cCuEncoder.   init( this );
  
  // initialize DIF
  m_cSearch.setDIFTap ( m_cSPS.getDIFTap () );
  
  // initialize transform & quantization class
  m_pcCavlcCoder = getCavlcCoder();
  aTable8 = m_pcCavlcCoder->GetLP8Table();
  aTable4 = m_pcCavlcCoder->GetLP4Table();
  m_cTrQuant.init( g_uiMaxCUWidth, g_uiMaxCUHeight, 1 << m_uiQuadtreeTULog2MaxSize, m_iSymbolMode, aTable4, aTable8, m_bUseRDOQ, true );
  
  // initialize encoder search class
  m_cSearch.init( this, &m_cTrQuant, m_iSearchRange, m_iFastSearch, 0, &m_cEntropyCoder, &m_cRdCost, getRDSbacCoder(), getRDGoOnSbacCoder() );

  //{ [KSI] - MVC
  // initialize multiview reference list
  // [KSI] Multiview reference list를 초기화 한다.
  m_cMultiView.openMultiView( m_uiNumViewsMinusOne+1, m_iGOPSize, m_iSourceWidth, m_iSourceHeight, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
  //} [KSI] - ~MVC
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

Void TEncTop::deletePicBuffer()
{
  TComList<TComPic*>::iterator iterPic = m_cListPic.begin();
  Int iSize = Int( m_cListPic.size() );
  
  for ( Int i = 0; i < iSize; i++ )
  {
    TComPic* pcPic = *(iterPic++);
    
    pcPic->destroy();
    delete pcPic;
    pcPic = NULL;
  }
}

/**
 - Application has picture buffer list with size of GOP + 1
 - Picture buffer list acts like as ring buffer
 - End of the list has the latest picture
 .
 \param   bEos                true if end-of-sequence is reached
 \param   pcPicYuvOrg         original YUV picture
 \param   rcListFwdViews      Fwd multiview reference list of current picture
 \param   rcListBwdViews      Bwd multiview reference list of current picture
 \retval  rcListPicYuvRecOut  list of reconstruction YUV pictures
 \retval  rcListBitstreamOut  list of output bitstreams
 \retval  iNumEncoded         number of encoded pictures
 */
Void TEncTop::encode( bool bEos, TComPicYuv* pcPicYuvOrg,
				//{ [KSI] - MVC
					  TComList<TComPicYuv*>& rcListFwdViews,
					  TComList<TComPicYuv*>& rcListBwdViews,
			    //} [KSI] - ~MVC
					  TComList<TComPicYuv*>& rcListPicYuvRecOut,
					  TComList<TComBitstream*>& rcListBitstreamOut,
					  Int& iNumEncoded )
{
  TComPic* pcPicCurr = NULL;
  
  // get original YUV
  // [KSI] TEncTop::m_cPicList에서 적절히 초기화해서 준비 된 TComPic을 하나 얻어 온다. 없으면 만든다.
  // [KSI] 여기에서 m_iPOCLast 변수가 증가한다.
  // [KSI] 여기에서 m_iNumPicRcvd 변수가 증가한다.
  xGetNewPicBuffer( pcPicCurr );
  // [KSI] 위에서 얻어온 TComPic에 TComPicYuv를 복사한다. 그리고, 그 다음부터는 pcPicYuvOrg의 쓸모는 끝난다.
  pcPicYuvOrg->copyToPic( pcPicCurr->getPicYuvOrg() );

  //{ [KSI] - MVC
  // [KSI] Inter-view prediction을 위해 기 ENC된 View Component의 RECON 파일에서 읽어온 프레임을 Multiview reference list에 설정한다.
  // [KSI] SPS의 설정에 따라, 여러 view를 reference할 수 있으므로, Multiview reference list는 조금 복잡한 편이다.
  if ( m_bMVC ) xAddMultiView(rcListFwdViews, rcListBwdViews);
  //} [KSI] - ~MVC
  
  // [KSI] 첫 Picture가 아니고, m_iNumPicRcvd가 GOP와 다르고, GOP는 0이 아니며, EOS가 아닐 때는 그냥 리턴한다.
  // [KSI] GOP 단위로 m_cPicList에 원본을 쌓은다음 GOP 만큼을 한번에 Encode하기 위함이다.
  // [KSI] m_cPicList의 각 Element는 Picture의 원본/RECON/Metadata를 모두 보관하고 있다. --> Decoded Picture Buffer 로 쓸 수 있는 이유.
  // [KSI] 첫 Picture는 바로 Encode하기 때문에, GOP 만큼 읽어오면 다음 I Picture 대상 까지 읽어오게 된다.
  // [KSI] 자연스럽게 Open GOP 구조의 Hierarchical B Encoding Structure를 구성하게 된다.
  if ( m_iPOCLast != 0 && ( m_iNumPicRcvd != m_iGOPSize && m_iGOPSize ) && !bEos )
  {
    iNumEncoded = 0;
    return;
  }
  
  // compress GOP
  // [KSI] 본 프로젝트의 목적인 MVC의 경우에 한정하고, GOP == 8 로 설정 된 상태를 가정하면 다음과 같은 상태임을 추정할 수 있다.
  // [KSI] 첫 Picture가 아니라면, 여기까지 진행 했을 때, m_cListPic는 다음과 같은 모습일 것이다.
  // [KSI] [현 GOP의 I] [b] [B] [b] [B] [b] [B] [b] [다음 GOP의 I]
  // [KSI] 이 때, 현 GOP의 I는 이미 Encoding이 완료 된 상태로 존재 할 것이며, 가장 먼저 Encoding 될 대상은 다음 GOP의 I이다.
  m_cGOPEncoder.compressGOP( m_iPOCLast, m_iNumPicRcvd, m_cListPic, rcListPicYuvRecOut, rcListBitstreamOut );
  
  iNumEncoded         = m_iNumPicRcvd;
  m_iNumPicRcvd       = 0;
  m_uiNumAllPicCoded += iNumEncoded;
  
  if (bEos)
  {
    m_cGOPEncoder.printOutSummary (m_uiNumAllPicCoded);
  }
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

/**
 - Application has picture buffer list with size of GOP + 1
 - Picture buffer list acts like as ring buffer
 - End of the list has the latest picture
 .
 \retval rpcPic obtained picture buffer
 */
Void TEncTop::xGetNewPicBuffer ( TComPic*& rpcPic )
{
  TComSlice::sortPicList(m_cListPic);
  
  // bug-fix - erase frame memory (previous GOP) which is not used for reference any more
  if (m_cListPic.size() >= (UInt)(m_iGOPSize + 2 * getNumOfReference() + 1) )  // 2)   //  K. Lee bug fix - for multiple reference > 2
  {
    rpcPic = m_cListPic.popFront();
    
    // is it necessary without long-term reference?
    if ( rpcPic->getERBIndex() > 0 && abs(rpcPic->getPOC() - m_iPOCLast) <= 0 )
    {
      m_cListPic.pushFront(rpcPic);
      
      TComList<TComPic*>::iterator iterPic  = m_cListPic.begin();
      rpcPic = *(++iterPic);
      if ( abs(rpcPic->getPOC() - m_iPOCLast) <= m_iGOPSize )
      {
        rpcPic = new TComPic;
        rpcPic->create( m_iSourceWidth, m_iSourceHeight, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
      }
      else
      {
        m_cListPic.erase( iterPic );
        TComSlice::sortPicList( m_cListPic );
      }
    }
  }
  else
  {
    rpcPic = new TComPic;
    rpcPic->create( m_iSourceWidth, m_iSourceHeight, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
  }
  
  m_cListPic.pushBack( rpcPic );
  rpcPic->setReconMark (false);
  
  m_iPOCLast++;
  m_iNumPicRcvd++;
  
  rpcPic->getSlice()->setPOC( m_iPOCLast );
  
  // mark it should be extended
  rpcPic->getPicYuvRec()->setBorderExtension(false);
}

//{ [KSI] - MVC
Void  TEncTop::xAddMultiView( TComList<TComPicYuv*>& rcListFwdViews, TComList<TComPicYuv*>& rcListBwdViews )
{
  UInt uiCurrentViewIndex;
  Bool bAnchor;
  TComList<TComPicYuv*>::iterator itor;
  for ( uiCurrentViewIndex = 0; uiCurrentViewIndex <= m_uiNumViewsMinusOne; uiCurrentViewIndex++ )
  {
	  if ( m_auiViewOrder[uiCurrentViewIndex] == m_uiCurrentViewID )
		  break;
  }

  bAnchor = (m_iPOCLast == 0 || m_iPOCLast % m_uiIntraPeriod == 0 ) ? true : false;

  if ( bAnchor )
  {
	  itor = rcListFwdViews.begin();
	  for ( UInt i = 0; (i < m_auiNumAnchorRefsL0[uiCurrentViewIndex]) && (itor != rcListFwdViews.end()); i++, ++itor )
	  {
		  UInt uiRefViewIndex;
		  for ( uiRefViewIndex = 0; uiRefViewIndex <= m_uiNumViewsMinusOne; uiRefViewIndex++ )
		  {
			  if ( m_auiViewOrder[uiRefViewIndex] == m_aauiAnchorRefL0[uiCurrentViewIndex][i] )
				  break;
		  }
		  m_cMultiView.addMultiViewPicture(uiRefViewIndex, *itor, m_iPOCLast);
	  }

	  itor = rcListBwdViews.begin();
	  for ( UInt i = 0; (i < m_auiNumAnchorRefsL1[uiCurrentViewIndex]) && (itor != rcListBwdViews.end()); i++, ++itor )
	  {
		  UInt uiRefViewIndex;
		  for ( uiRefViewIndex = 0; uiRefViewIndex <= m_uiNumViewsMinusOne; uiRefViewIndex++ )
		  {
			  if ( m_auiViewOrder[uiRefViewIndex] == m_aauiAnchorRefL1[uiCurrentViewIndex][i] )
				  break;
		  }
		  m_cMultiView.addMultiViewPicture(uiRefViewIndex, *itor, m_iPOCLast);
	  }
  }
  else
  {
	  itor = rcListFwdViews.begin();
	  for ( UInt i = 0; (i < m_auiNumNonAnchorRefsL0[uiCurrentViewIndex]) && (itor != rcListFwdViews.end()); i++, ++itor )
	  {
		  UInt uiRefViewIndex;
		  for ( uiRefViewIndex = 0; uiRefViewIndex <= m_uiNumViewsMinusOne; uiRefViewIndex++ )
		  {
			  if ( m_auiViewOrder[uiRefViewIndex] == m_aauiNonAnchorRefL0[uiCurrentViewIndex][i] )
				  break;
		  }
		  m_cMultiView.addMultiViewPicture(uiRefViewIndex, *itor, m_iPOCLast);
	  }

	  itor = rcListBwdViews.begin();
	  for ( UInt i = 0; (i < m_auiNumNonAnchorRefsL1[uiCurrentViewIndex]) && (itor != rcListBwdViews.end()); i++, ++itor )
	  {
		  UInt uiRefViewIndex;
		  for ( uiRefViewIndex = 0; uiRefViewIndex <= m_uiNumViewsMinusOne; uiRefViewIndex++ )
		  {
			  if ( m_auiViewOrder[uiRefViewIndex] == m_aauiNonAnchorRefL1[uiCurrentViewIndex][i] )
				  break;
		  }
		  m_cMultiView.addMultiViewPicture(uiRefViewIndex, *itor, m_iPOCLast);
	  }
  }
}
//} [KSI] - MVC


Void TEncTop::xInitSPS()
{
  m_cSPS.setWidth         ( m_iSourceWidth      );
  m_cSPS.setHeight        ( m_iSourceHeight     );
  m_cSPS.setPad           ( m_aiPad             );
  m_cSPS.setMaxCUWidth    ( g_uiMaxCUWidth      );
  m_cSPS.setMaxCUHeight   ( g_uiMaxCUHeight     );
  m_cSPS.setMaxCUDepth    ( g_uiMaxCUDepth      );
  m_cSPS.setMinTrDepth    ( 0                   );
  m_cSPS.setMaxTrDepth    ( 1                   );
  
  m_cSPS.setUseALF        ( m_bUseALF           );
  
  m_cSPS.setQuadtreeTULog2MaxSize( m_uiQuadtreeTULog2MaxSize );
  m_cSPS.setQuadtreeTULog2MinSize( m_uiQuadtreeTULog2MinSize );
  m_cSPS.setQuadtreeTUMaxDepthInter( m_uiQuadtreeTUMaxDepthInter    );
  m_cSPS.setQuadtreeTUMaxDepthIntra( m_uiQuadtreeTUMaxDepthIntra    );
  
  m_cSPS.setUseDQP        ( m_iMaxDeltaQP != 0  );
  m_cSPS.setUseLDC        ( m_bUseLDC           );
  m_cSPS.setUsePAD        ( m_bUsePAD           );
  
#if HHI_MRG
  m_cSPS.setUseMRG        ( m_bUseMRG           ); // SOPH:
#endif
  m_cSPS.setDIFTap        ( m_iDIFTap           );
  
  m_cSPS.setMaxTrSize   ( 1 << m_uiQuadtreeTULog2MaxSize );
  
  Int i;
#if HHI_AMVP_OFF
  for ( i = 0; i < g_uiMaxCUDepth; i++ )
  {
    m_cSPS.setAMVPMode( i, AM_NONE );
  }
#else
  for ( i = 0; i < g_uiMaxCUDepth; i++ )
  {
    m_cSPS.setAMVPMode( i, AM_EXPL );
  }
#endif
  
  
#if HHI_RMP_SWITCH
  m_cSPS.setUseRMP( m_bUseRMP );
#endif
  
  m_cSPS.setBitDepth    ( g_uiBitDepth        );
  m_cSPS.setBitIncrement( g_uiBitIncrement    );

  //{ [KSI] - MVC
  m_cSPS.setMVC                          (m_bMVC);
  m_cSPS.setCurrentViewID                (m_uiCurrentViewID);
  m_cSPS.setNumViewsMinusOne             (m_uiNumViewsMinusOne);
  m_cSPS.setViewOrder                    (m_auiViewOrder);
  m_cSPS.setNumAnchorRefsL0              (m_auiNumAnchorRefsL0);
  m_cSPS.setNumAnchorRefsL1              (m_auiNumAnchorRefsL1);
  m_cSPS.setAnchorRefL0                  (m_aauiAnchorRefL0);
  m_cSPS.setAnchorRefL1                  (m_aauiAnchorRefL1);
  m_cSPS.setNumNonAnchorRefsL0           (m_auiNumNonAnchorRefsL0);
  m_cSPS.setNumNonAnchorRefsL1           (m_auiNumNonAnchorRefsL1);
  m_cSPS.setNonAnchorRefL0               (m_aauiNonAnchorRefL0);
  m_cSPS.setNonAnchorRefL1               (m_aauiNonAnchorRefL1);
  //} [KSI] - ~MVC
}


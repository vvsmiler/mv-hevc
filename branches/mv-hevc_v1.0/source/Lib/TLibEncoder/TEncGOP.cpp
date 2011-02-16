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

/** \file     TEncGOP.cpp
    \brief    GOP encoder class
*/

#include "TEncTop.h"
#include "TEncGOP.h"
#include "TEncAnalyze.h"

#include <time.h>

// ====================================================================================================================
// Constructor / destructor / initialization / destroy
// ====================================================================================================================

TEncGOP::TEncGOP()
{
  m_iHrchDepth          = 0;
  m_iGopSize            = 0;
  m_iNumPicCoded        = 0; //Niko
  m_bFirst              = true;
  
  m_pcCfg               = NULL;
  m_pcSliceEncoder      = NULL;
  m_pcListPic           = NULL;
  
  m_pcEntropyCoder      = NULL;
  m_pcCavlcCoder        = NULL;
  m_pcSbacCoder         = NULL;
  m_pcBinCABAC          = NULL;
  
  m_bSeqFirst           = true;
  
  return;
}

TEncGOP::~TEncGOP()
{
}

Void  TEncGOP::create()
{
}

Void  TEncGOP::destroy()
{
}

Void TEncGOP::init ( TEncTop* pcTEncTop )
{
  m_pcEncTop	         = pcTEncTop;
  m_pcCfg                = pcTEncTop;
  m_pcSliceEncoder       = pcTEncTop->getSliceEncoder();
  m_pcListPic            = pcTEncTop->getListPic();
  
  m_pcEntropyCoder       = pcTEncTop->getEntropyCoder();
  m_pcCavlcCoder         = pcTEncTop->getCavlcCoder();
  m_pcSbacCoder          = pcTEncTop->getSbacCoder();
  m_pcBinCABAC           = pcTEncTop->getBinCABAC();
  m_pcLoopFilter         = pcTEncTop->getLoopFilter();
  m_pcBitCounter         = pcTEncTop->getBitCounter();
  
  // Adaptive Loop filter
  m_pcAdaptiveLoopFilter = pcTEncTop->getAdaptiveLoopFilter();
  //--Adaptive Loop filter
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

Void TEncGOP::compressGOP( Int iPOCLast, Int iNumPicRcvd, TComList<TComPic*>& rcListPic, TComList<TComPicYuv*>& rcListPicYuvRecOut, TComList<TComBitstream*> rcListBitstreamOut )
{
  TComPic*        pcPic;
  TComPicYuv*     pcPicYuvRecOut;
  TComBitstream*  pcBitstreamOut;
  TComPicYuv      cPicOrg;
  //stats
  // [KSI] 이 변수는 쓸데 없이 존재 한다. 지우고 싶으나, 일단 놔둔다.
  TComBitstream*  pcOut = new TComBitstream;
  pcOut->create( 500000 );
  
  // [KSI] 이 함수는 hierarchical B의 depth(m_iHrchDepth)만 결정한다.
  // [KSI] 특이한점은 m_iHrchDepth에 실제 depth+1을 설정한다.
  // [KSI] 아래의 parameter중 iPOCLast만 사용한다. 나머지는 쓸데 없이 존재 한다. 지우고 싶으나, 일단 놔둔다.
  xInitGOP( iPOCLast, iNumPicRcvd, rcListPic, rcListPicYuvRecOut );
  
  m_iNumPicCoded = 0;
  for ( Int iDepth = 0; iDepth < m_iHrchDepth; iDepth++ )
  {
    Int iTimeOffset = ( 1 << (m_iHrchDepth - 1 - iDepth) ); // 예1) GOP가 8인, Depth가 0인 경우 iTimeOffset = ( 1 << (4 - 1 - 0) ) = 8
	                                                        // 예2) GOP가 8인, Depth가 1인 경우 iTimeOffset = ( 1 << (4 - 1 - 1) ) = 4
    Int iStep       = iTimeOffset << 1;                     // 예1) 위의 경우, 16
	                                                        // 예2) 위의 경우, 8
    
    // generalized B info.
	// [KSI] MVC는 해당 없음.
    if ( (m_pcCfg->getHierarchicalCoding() == false) && (iDepth != 0) )
    {
      iTimeOffset   = 1;
      iStep         = 1;
    }
    
    UInt uiColDir = 1;
    
    for ( ; iTimeOffset <= iNumPicRcvd; iTimeOffset += iStep )
    {
      //-- For time output for each slice
      long iBeforeTime = clock();
      
      // generalized B info.
	  // [KSI] MVC는 해당 없음.
      if ( (m_pcCfg->getHierarchicalCoding() == false) && (iDepth != 0) && (iTimeOffset == m_iGopSize) && (iPOCLast != 0) )
      {
        continue;
      }
      
      /////////////////////////////////////////////////////////////////////////////////////////////////// Initial to start encoding
      UInt  uiPOCCurr = iPOCLast - (iNumPicRcvd - iTimeOffset); // 예1) iPOCLast는 8, iNumPicRcvd는 8, iTimeOffset은 8 POCCurr = 8 - (8 - 8) = 8
	                                                            // 예2) iPOCLast는 8, iNumPicRcvd는 8, iTimeOffset은 4 POCCurr = 8 - (8 - 4) = 4
      
	  // [KSI] TEncTop에 정의된 PictureList(참조 영상과 현재 입력 영상으로 구성된 리스트)
	  // [KSI] TAppEncTop에 정의된 ReconPictureList, BitstreamList에서 currentPOC의 ENCODING 결과를 보관하기 위해 할당된 item들을 찾아 하나씩 할당한다.

	  // [KSI] 입력 : TEncTop::m_cListPic, TAppEncTop::m_cListPicYuvRec, TAppEncTop::m_cListBitstream
	  //            : iNumPicRcvd, iTimeOffset, uiPOCCurr
	  // [KSI] 출력 : pcPic; uiPOCCurr에 해당하는 원본/RECON/Bitstream을 보관하고 있는 객체.
	  //            : pcPicYuvRecOut; uiPOCCurr에 해당하는 RECON Bitmap을 보관 할 객체.
	  //            : pcBitstreamOut; uiPOCCurr에 해당하는 Encoded Bitstream을 보관 할 객체.
      xGetBuffer( rcListPic, rcListPicYuvRecOut, rcListBitstreamOut, iNumPicRcvd, iTimeOffset,  pcPic, pcPicYuvRecOut, pcBitstreamOut, uiPOCCurr );
      
      // save original picture
	  // [KSI] Encoding 도중 원본 Bitmap에 여러 처리를 하기 때문에, 원본이 손상된다.
	  // [KSI] 나중에 원본을 복원하기 위하여 임시 저장소에 보관해 둔다.
      cPicOrg.create( pcPic->getPicYuvOrg()->getWidth(), pcPic->getPicYuvOrg()->getHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
      pcPic->getPicYuvOrg()->copyToPic( &cPicOrg );
      
      // scaling of picture
      if ( g_uiBitIncrement )
      {
        xScalePic( pcPic );
      }
      
      //  Bitstream reset
      pcBitstreamOut->resetBits();
      pcBitstreamOut->rewindStreamPacket();
      
      //  Slice data initialization
      TComSlice*      pcSlice;
	  // [KSI] 입력 : pcPic, iPOCLast, uiPOCCurr, iNumPicRcvd, iTimeOffset, iDepth
	  // [KSI] 출력 : pcSlice; pcPic에서 할당한 TComSlice를 현재 상황에 맞게 설정.
	  //            :          여기에서 SliceType을 설정하므로, Multiview의 BaseViewComponent가 아닌 경우
	  //            :          Anchor Frame의 Type을 I에서 P/B로 강제 설정하는 처리를 추가 해야한다. 중요 중요 중요.
      m_pcSliceEncoder->initEncSlice ( pcPic, iPOCLast, uiPOCCurr, iNumPicRcvd, iTimeOffset, iDepth, pcSlice );
      
      //  Set SPS
      pcSlice->setSPS( m_pcEncTop->getSPS() );
      pcSlice->setPPS( m_pcEncTop->getPPS() );
      
      //  Set reference list
	  // [KSI] TEncTop::m_cPicList의 내용을 기반으로 현재 POC의 TComPic을 중심으로 과거, 미래의 Reference Picture들을 찾는다.
	  // [KSI] 찾은 Reference Picture들의
	  // [KSI]      1. TComPic객체의 포인터를 TComSlice::m_apcRefPicList에 등록.
	  // [KSI]      2. TComSlice::m_aiRefPOCList에 등록된 TComPic객체의 POC를 등록.
	  // [KSI]      3. TComSlice::m_aiNumRefIdx에 등록된 TComPic객체의 갯수를 등록.
	  // [KSI] 이 과정에 추가로 MVC 설정에 따라 Anchor/NonAnchor Picture들의 Pointer/POC/갯수를 등록 해 둔다.
	  // [KSI] 현재 MVC 규칙상, 다른 View의 같은 POC를 가져올 것이기 때문에, TComPic에 따로 정보가 추가 될 필요가 없다.
	  // [KSI] ViewID와 같은 정보는 이미 SPS에 있고, 디코딩 할 때에도 역시 SPS를 참조 하면 되기 때문이다.
	  pcSlice->setRefPicList ( rcListPic );

	  //{ [KSI] - MVC
	  // [KSI] Reference List의 맨 마지막에 Inter-view용 reference picture를 설정한다.
	  if ( m_pcCfg->getMVC() )
	  {
		  Bool bAnchor = (uiPOCCurr == 0 || uiPOCCurr % m_pcCfg->getIntraPeriod() == 0 ) ? true : false;
		  pcSlice->setInterviewRefPicList( m_pcEncTop->getMultiView(), uiPOCCurr, REF_PIC_LIST_0, bAnchor);
		  pcSlice->setInterviewRefPicList( m_pcEncTop->getMultiView(), uiPOCCurr, REF_PIC_LIST_1, bAnchor);
	  }
	  //} [KSI] - ~MVC
	  
      //  Slice info. refinement
      if ( (pcSlice->getSliceType() == B_SLICE) && (pcSlice->getNumRefIdx(REF_PIC_LIST_1) == 0) )
      {
        pcSlice->setSliceType ( P_SLICE );
        pcSlice->setDRBFlag   ( true );
      }
      
      // Generalized B
	  // [KSI] MVC는 해당 없음.
      if ( m_pcCfg->getUseGPB() )
      {
        if (pcSlice->getSliceType() == P_SLICE)
        {
          pcSlice->setSliceType( B_SLICE ); // Change slice type by force
          
          Int iNumRefIdx = pcSlice->getNumRefIdx(REF_PIC_LIST_0);
          pcSlice->setNumRefIdx( REF_PIC_LIST_1, iNumRefIdx );
          
          for (Int iRefIdx = 0; iRefIdx < iNumRefIdx; iRefIdx++)
          {
            pcSlice->setRefPic(pcSlice->getRefPic(REF_PIC_LIST_0, iRefIdx), REF_PIC_LIST_1, iRefIdx);
          }
        }
      }
      
      if (pcSlice->getSliceType() == B_SLICE)
      {
        pcSlice->setColDir(uiColDir);
      }
      
      uiColDir = 1-uiColDir;
      
      //-------------------------------------------------------------
	  // [KSI] 왜인지는 모르겠지만, 여기에서 TComSlice::m_aiRefPOCList를 설정한다.
      pcSlice->setRefPOCList();
      
#if MS_NO_BACK_PRED_IN_B0 // disable backward prediction when list1 == list0, and disable list1 search, JCTVC-C278
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
      
      /////////////////////////////////////////////////////////////////////////////////////////////////// Compress a slice
      //  Slice compression
      if (m_pcCfg->getUseASR())
      {
        m_pcSliceEncoder->setSearchRange(pcSlice);
      }
#ifdef ROUNDING_CONTROL_BIPRED
      Bool b = true;
      if (m_pcCfg->getUseRoundingControlBipred())
      {
        if (m_pcCfg->getGOPSize()==1)
          b = ((pcSlice->getPOC()&1)==0);
        else
          b = (pcSlice->isReferenced() == 0);
      }

      pcSlice->setRounding(b);
#endif
	  // [KSI] 여기에서 Syntax로 표현하기 위한 시나리오들을 모두 만들어낸다.
      m_pcSliceEncoder->precompressSlice( pcPic );
      m_pcSliceEncoder->compressSlice   ( pcPic );
      
      //-- Loop filter
      m_pcLoopFilter->setCfg(pcSlice->getLoopFilterDisable(), m_pcCfg->getLoopFilterAlphaC0Offget(), m_pcCfg->getLoopFilterBetaOffget());
      m_pcLoopFilter->loopFilterPic( pcPic );
      
      /////////////////////////////////////////////////////////////////////////////////////////////////// File writing
      // Set entropy coder
      m_pcEntropyCoder->setEntropyCoder   ( m_pcCavlcCoder, pcSlice );
      m_pcEntropyCoder->setBitstream      ( pcBitstreamOut          );
      
      // write SPS
      if ( m_bSeqFirst )
      {
        m_pcEntropyCoder->encodeSPS( pcSlice->getSPS() );
#if HHI_NAL_UNIT_SYNTAX
        pcBitstreamOut->write( 1, 1 );
        pcBitstreamOut->writeAlignZero();
        // generate start code
        pcBitstreamOut->write( 1, 32);
#endif
        
        m_pcEntropyCoder->encodePPS( pcSlice->getPPS() );
#if HHI_NAL_UNIT_SYNTAX
        pcBitstreamOut->write( 1, 1 );
        pcBitstreamOut->writeAlignZero();
        // generate start code
        pcBitstreamOut->write( 1, 32);
#endif
        m_bSeqFirst = false;
      }
      
#if HHI_NAL_UNIT_SYNTAX
      UInt uiPosBefore = pcBitstreamOut->getNumberOfWrittenBits()>>3;
#endif
      
      // write SliceHeader
      m_pcEntropyCoder->encodeSliceHeader ( pcSlice                 );
      
      // is it needed?
      if ( pcSlice->getSymbolMode() )
      {
        m_pcSbacCoder->init( (TEncBinIf*)m_pcBinCABAC );
        m_pcEntropyCoder->setEntropyCoder ( m_pcSbacCoder, pcPic->getSlice() );
        m_pcEntropyCoder->resetEntropy    ();
      }
      
      /////////////////////////////////////////////////////////////////////////////////////////////////// Reconstructed image output
      TComPicYuv pcPicD;
      pcPicD.create( pcPic->getPicYuvOrg()->getWidth(), pcPic->getPicYuvOrg()->getHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
      
      // adaptive loop filter
      if ( pcSlice->getSPS()->getUseALF() )
      {
        ALFParam cAlfParam;
#if TSB_ALF_HEADER
        m_pcAdaptiveLoopFilter->setNumCUsInFrame(pcPic);
#endif
        m_pcAdaptiveLoopFilter->allocALFParam(&cAlfParam);
        
        // set entropy coder for RD
        if ( pcSlice->getSymbolMode() )
        {
          m_pcEntropyCoder->setEntropyCoder ( m_pcEncTop->getRDGoOnSbacCoder(), pcPic->getSlice() );
        }
        else
        {
          m_pcEntropyCoder->setEntropyCoder ( m_pcCavlcCoder, pcPic->getSlice() );
        }
        
        m_pcEntropyCoder->resetEntropy    ();
        m_pcEntropyCoder->setBitstream    ( m_pcBitCounter );
        
        m_pcAdaptiveLoopFilter->startALFEnc(pcPic, m_pcEntropyCoder );
        
        UInt uiMaxAlfCtrlDepth;
        
        UInt64 uiDist, uiBits;
        m_pcAdaptiveLoopFilter->ALFProcess( &cAlfParam, pcPic->getSlice()->getLambda(), uiDist, uiBits, uiMaxAlfCtrlDepth );
        m_pcAdaptiveLoopFilter->endALFEnc();
        
        // set entropy coder for writing
        m_pcSbacCoder->init( (TEncBinIf*)m_pcBinCABAC );
        
        if ( pcSlice->getSymbolMode() )
        {
          m_pcEntropyCoder->setEntropyCoder ( m_pcSbacCoder, pcPic->getSlice() );
        }
        else
        {
          m_pcEntropyCoder->setEntropyCoder ( m_pcCavlcCoder, pcPic->getSlice() );
        }
        
        m_pcEntropyCoder->resetEntropy    ();
        m_pcEntropyCoder->setBitstream    ( pcBitstreamOut );
        if (cAlfParam.cu_control_flag)
        {
          m_pcEntropyCoder->setAlfCtrl( true );
          m_pcEntropyCoder->setMaxAlfCtrlDepth(uiMaxAlfCtrlDepth);
          if (pcSlice->getSymbolMode() == 0)
          {
            m_pcCavlcCoder->setAlfCtrl(true);
            m_pcCavlcCoder->setMaxAlfCtrlDepth(uiMaxAlfCtrlDepth); //D0201
          }
        }
        else
        {
          m_pcEntropyCoder->setAlfCtrl(false);
        }
#if LCEC_STAT
        if (pcSlice->getSymbolMode() == 0)
        {          
          m_pcCavlcCoder->setAdaptFlag( true );
        }
#endif
        m_pcEntropyCoder->encodeAlfParam(&cAlfParam);
        
#if TSB_ALF_HEADER
        if(cAlfParam.cu_control_flag)
        {
          m_pcEntropyCoder->encodeAlfCtrlParam(&cAlfParam);
        }
#endif
        
        m_pcAdaptiveLoopFilter->freeALFParam(&cAlfParam);
      }
      
      // File writing
      m_pcSliceEncoder->encodeSlice( pcPic, pcBitstreamOut );
      
      //  End of bitstream & byte align
#if ! HHI_NAL_UNIT_SYNTAX
      if( pcSlice->getSymbolMode() )
#endif
      {
        pcBitstreamOut->write( 1, 1 );
        pcBitstreamOut->writeAlignZero();
      }
      
      pcBitstreamOut->flushBuffer();
#if HHI_NAL_UNIT_SYNTAX
      pcBitstreamOut->convertRBSPToPayload( uiPosBefore );
#endif
      // de-scaling of picture
      xDeScalePic( pcPic, &pcPicD );
      
      // save original picture
      cPicOrg.copyToPic( pcPic->getPicYuvOrg() );
      
      //-- For time output for each slice
      Double dEncTime = (double)(clock()-iBeforeTime) / CLOCKS_PER_SEC;
      
      xCalculateAddPSNR( pcPic, &pcPicD, pcBitstreamOut->getNumberOfWrittenBits(), dEncTime );
      
      // free original picture
      cPicOrg.destroy();
      
      //  Reconstruction buffer update
      pcPicD.copyToPic(pcPicYuvRecOut);
      
      pcPicD.destroy();
      
      pcPic->setReconMark   ( true );
      
      m_bFirst = false;
      m_iNumPicCoded++;
    }
    
    // generalized B info.
    if ( m_pcCfg->getHierarchicalCoding() == false && iDepth != 0 )
      break;
  }
  
  pcOut->destroy();
  delete pcOut;
  
  assert ( m_iNumPicCoded == iNumPicRcvd );
}

Void TEncGOP::printOutSummary(UInt uiNumAllPicCoded)
{
  assert (uiNumAllPicCoded == m_gcAnalyzeAll.getNumPic());
  
  
#if LCEC_STAT
  //m_pcCavlcCoder->statistics (0,2);
#endif
  
  //--CFG_KDY
  m_gcAnalyzeAll.setFrmRate( m_pcCfg->getFrameRate() );
  m_gcAnalyzeI.setFrmRate( m_pcCfg->getFrameRate() );
  m_gcAnalyzeP.setFrmRate( m_pcCfg->getFrameRate() );
  m_gcAnalyzeB.setFrmRate( m_pcCfg->getFrameRate() );
  
  //-- all
  printf( "\n\nSUMMARY --------------------------------------------------------\n" );
  m_gcAnalyzeAll.printOut('a');
  
  printf( "\n\nI Slices--------------------------------------------------------\n" );
  m_gcAnalyzeI.printOut('i');
  
  printf( "\n\nP Slices--------------------------------------------------------\n" );
  m_gcAnalyzeP.printOut('p');
  
  printf( "\n\nB Slices--------------------------------------------------------\n" );
  m_gcAnalyzeB.printOut('b');
  
#if _SUMMARY_OUT_
  m_gcAnalyzeAll.printSummaryOut();
#endif
#if _SUMMARY_PIC_
  m_gcAnalyzeI.printSummary('I');
  m_gcAnalyzeP.printSummary('P');
  m_gcAnalyzeB.printSummary('B');
#endif
}

Void TEncGOP::preLoopFilterPicAll( TComPic* pcPic, UInt64& ruiDist, UInt64& ruiBits )
{
  TComSlice* pcSlice = pcPic->getSlice();
  Bool bCalcDist = false;
  
  m_pcLoopFilter->setCfg(pcSlice->getLoopFilterDisable(), m_pcCfg->getLoopFilterAlphaC0Offget(), m_pcCfg->getLoopFilterBetaOffget());
  m_pcLoopFilter->loopFilterPic( pcPic );
  
  m_pcEntropyCoder->setEntropyCoder ( m_pcEncTop->getRDGoOnSbacCoder(), pcSlice );
  m_pcEntropyCoder->resetEntropy    ();
  m_pcEntropyCoder->setBitstream    ( m_pcBitCounter );
  
  // Adaptive Loop filter
  if( pcSlice->getSPS()->getUseALF() )
  {
    ALFParam cAlfParam;
#if TSB_ALF_HEADER
    m_pcAdaptiveLoopFilter->setNumCUsInFrame(pcPic);
#endif
    m_pcAdaptiveLoopFilter->allocALFParam(&cAlfParam);
    
    m_pcAdaptiveLoopFilter->startALFEnc(pcPic, m_pcEntropyCoder);
    
    UInt uiMaxAlfCtrlDepth;
    m_pcAdaptiveLoopFilter->ALFProcess(&cAlfParam, pcPic->getSlice()->getLambda(), ruiDist, ruiBits, uiMaxAlfCtrlDepth );
    m_pcAdaptiveLoopFilter->endALFEnc();
    m_pcAdaptiveLoopFilter->freeALFParam(&cAlfParam);
  }
  
  m_pcEntropyCoder->resetEntropy    ();
  ruiBits += m_pcEntropyCoder->getNumberOfWrittenBits();
  
  if (!bCalcDist)
    ruiDist = xFindDistortionFrame(pcPic->getPicYuvOrg(), pcPic->getPicYuvRec());
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

Void TEncGOP::xInitGOP( Int iPOCLast, Int iNumPicRcvd, TComList<TComPic*>& rcListPic, TComList<TComPicYuv*>& rcListPicYuvRecOut )
{
  assert( iNumPicRcvd > 0 );
  Int i;
  
  //  Set hierarchical B info.
  m_iGopSize    = m_pcCfg->getGOPSize();
  for( i=1 ; ; i++)
  {
    m_iHrchDepth = i;
    if((m_iGopSize >> i)==0)
    {
      break;
    }
  }
  
  //  Exception for the first frame
  if ( iPOCLast == 0 )
  {
    m_iGopSize    = 1;
    m_iHrchDepth  = 1;
  }
  
  if (m_iGopSize == 0)
  {
    m_iHrchDepth = 1;
  }
  
  return;
}

Void TEncGOP::xGetBuffer( TComList<TComPic*>&       rcListPic,
                         TComList<TComPicYuv*>&    rcListPicYuvRecOut,
                         TComList<TComBitstream*>& rcListBitstreamOut,
                         Int                       iNumPicRcvd,
                         Int                       iTimeOffset,
                         TComPic*&                 rpcPic,
                         TComPicYuv*&              rpcPicYuvRecOut,
                         TComBitstream*&           rpcBitstreamOut,
                         UInt                      uiPOCCurr )
{
  Int i;
  //  Rec. output
  TComList<TComPicYuv*>::iterator     iterPicYuvRec = rcListPicYuvRecOut.end();
  for ( i = 0; i < iNumPicRcvd - iTimeOffset + 1; i++ )
  {
    iterPicYuvRec--;
  }
  
  rpcPicYuvRecOut = *(iterPicYuvRec);
  
  //  Bitstream output
  TComList<TComBitstream*>::iterator  iterBitstream = rcListBitstreamOut.begin();
  for ( i = 0; i < m_iNumPicCoded; i++ )
  {
    iterBitstream++;
  }
  rpcBitstreamOut = *(iterBitstream);
  
  //  Current pic.
  TComList<TComPic*>::iterator        iterPic       = rcListPic.begin();
  while (iterPic != rcListPic.end())
  {
    rpcPic = *(iterPic);
    if (rpcPic->getPOC() == (Int)uiPOCCurr)
    {
      break;
    }
    iterPic++;
  }
  
  assert (rpcPic->getPOC() == (Int)uiPOCCurr);
  
  return;
}

Void TEncGOP::xScalePic( TComPic* pcPic )
{
  Int     x, y;
  
  //===== calculate PSNR =====
  Pel*  pRec    = pcPic->getPicYuvOrg()->getLumaAddr();
  Int   iStride = pcPic->getStride();
  Int   iWidth  = pcPic->getPicYuvOrg()->getWidth();
  Int   iHeight = pcPic->getPicYuvOrg()->getHeight();
  
  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      pRec[x] <<= g_uiBitIncrement;
    }
    pRec += iStride;
  }
  
  iHeight >>= 1;
  iWidth  >>= 1;
  iStride >>= 1;
  pRec  = pcPic->getPicYuvOrg()->getCbAddr();
  
  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      pRec[x] <<= g_uiBitIncrement;
    }
    pRec += iStride;
  }
  
  pRec  = pcPic->getPicYuvOrg()->getCrAddr();
  
  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      pRec[x] <<= g_uiBitIncrement;
    }
    pRec += iStride;
  }
}

Void TEncGOP::xDeScalePic( TComPic* pcPic, TComPicYuv* pcPicD )
{
  Int     x, y;
  Int     offset = (g_uiBitIncrement>0)?(1<<(g_uiBitIncrement-1)):0;
  
  //===== calculate PSNR =====
  Pel*  pRecD   = pcPicD->getLumaAddr();
  Pel*  pRec    = pcPic->getPicYuvRec()->getLumaAddr();
  Int   iStride = pcPic->getStride();
  Int   iWidth  = pcPic->getPicYuvOrg()->getWidth();
  Int   iHeight = pcPic->getPicYuvOrg()->getHeight();
  
  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
#if IBDI_NOCLIP_RANGE
      pRecD[x] = ( pRec[x] + offset ) >> g_uiBitIncrement;
#else
      pRecD[x] = ClipMax( ( pRec[x] + offset ) >> g_uiBitIncrement );
#endif
    }
    pRecD += iStride;
    pRec += iStride;
  }
  
  iHeight >>= 1;
  iWidth  >>= 1;
  iStride >>= 1;
  pRecD = pcPicD->getCbAddr();
  pRec  = pcPic->getPicYuvRec()->getCbAddr();
  
  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
#if IBDI_NOCLIP_RANGE
      pRecD[x] = ( pRec[x] + offset ) >> g_uiBitIncrement;
#else
      pRecD[x] = ClipMax( ( pRec[x] + offset ) >> g_uiBitIncrement );
#endif
    }
    pRecD += iStride;
    pRec += iStride;
  }
  
  pRecD = pcPicD->getCrAddr();
  pRec  = pcPic->getPicYuvRec()->getCrAddr();
  
  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
#if IBDI_NOCLIP_RANGE
      pRecD[x] = ( pRec[x] + offset ) >> g_uiBitIncrement;
#else
      pRecD[x] = ClipMax( ( pRec[x] + offset ) >> g_uiBitIncrement );
#endif
    }
    pRecD += iStride;
    pRec += iStride;
  }
}

UInt64 TEncGOP::xFindDistortionFrame (TComPicYuv* pcPic0, TComPicYuv* pcPic1)
{
  Int     x, y;
  Pel*  pSrc0   = pcPic0 ->getLumaAddr();
  Pel*  pSrc1   = pcPic1 ->getLumaAddr();
  UInt  uiShift = g_uiBitIncrement<<1;
  Int   iTemp;
  
  Int   iStride = pcPic0->getStride();
  Int   iWidth  = pcPic0->getWidth();
  Int   iHeight = pcPic0->getHeight();
  
  UInt64  uiTotalDiff = 0;
  
  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      iTemp = pSrc0[x] - pSrc1[x]; uiTotalDiff += (iTemp*iTemp) >> uiShift;
    }
    pSrc0 += iStride;
    pSrc1 += iStride;
  }
  
  iHeight >>= 1;
  iWidth  >>= 1;
  iStride >>= 1;
  
  pSrc0  = pcPic0->getCbAddr();
  pSrc1  = pcPic1->getCbAddr();
  
  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      iTemp = pSrc0[x] - pSrc1[x]; uiTotalDiff += (iTemp*iTemp) >> uiShift;
    }
    pSrc0 += iStride;
    pSrc1 += iStride;
  }
  
  pSrc0  = pcPic0->getCrAddr();
  pSrc1  = pcPic1->getCrAddr();
  
  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      iTemp = pSrc0[x] - pSrc1[x]; uiTotalDiff += (iTemp*iTemp) >> uiShift;
    }
    pSrc0 += iStride;
    pSrc1 += iStride;
  }
  
  return uiTotalDiff;
}

Void TEncGOP::xCalculateAddPSNR( TComPic* pcPic, TComPicYuv* pcPicD, UInt uibits, Double dEncTime )
{
  Int     x, y;
  UInt    uiSSDY  = 0;
  UInt    uiSSDU  = 0;
  UInt    uiSSDV  = 0;
  
  Double  dYPSNR  = 0.0;
  Double  dUPSNR  = 0.0;
  Double  dVPSNR  = 0.0;
  
  //===== calculate PSNR =====
  Pel*  pOrg    = pcPic ->getPicYuvOrg()->getLumaAddr();
  Pel*  pRec    = pcPicD->getLumaAddr();
  Int   iStride = pcPicD->getStride();
  
  Int   iWidth;
  Int   iHeight;
  
  iWidth  = pcPicD->getWidth () - m_pcEncTop->getPad(0);
  iHeight = pcPicD->getHeight() - m_pcEncTop->getPad(1);
  
  Int   iSize   = iWidth*iHeight;
  
  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      Int iDiff = (Int)( pOrg[x] - pRec[x] );
      uiSSDY   += iDiff * iDiff;
    }
    pOrg += iStride;
    pRec += iStride;
  }
  
  iHeight >>= 1;
  iWidth  >>= 1;
  iStride >>= 1;
  pOrg  = pcPic ->getPicYuvOrg()->getCbAddr();
  pRec  = pcPicD->getCbAddr();
  
  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      Int iDiff = (Int)( pOrg[x] - pRec[x] );
      uiSSDU   += iDiff * iDiff;
    }
    pOrg += iStride;
    pRec += iStride;
  }
  
  pOrg  = pcPic ->getPicYuvOrg()->getCrAddr();
  pRec  = pcPicD->getCrAddr();
  
  for( y = 0; y < iHeight; y++ )
  {
    for( x = 0; x < iWidth; x++ )
    {
      Int iDiff = (Int)( pOrg[x] - pRec[x] );
      uiSSDV   += iDiff * iDiff;
    }
    pOrg += iStride;
    pRec += iStride;
  }
  
  Double fRefValueY = 255.0 * 255.0 * (Double)iSize;
  Double fRefValueC = fRefValueY / 4.0;
  dYPSNR            = ( uiSSDY ? 10.0 * log10( fRefValueY / (Double)uiSSDY ) : 99.99 );
  dUPSNR            = ( uiSSDU ? 10.0 * log10( fRefValueC / (Double)uiSSDU ) : 99.99 );
  dVPSNR            = ( uiSSDV ? 10.0 * log10( fRefValueC / (Double)uiSSDV ) : 99.99 );
  
  // fix: total bits should consider slice size bits (32bit)
  uibits += 32;
  
  //===== add PSNR =====
  m_gcAnalyzeAll.addResult (dYPSNR, dUPSNR, dVPSNR, (Double)uibits);
  if (pcPic->getSlice()->isIntra())
  {
    m_gcAnalyzeI.addResult (dYPSNR, dUPSNR, dVPSNR, (Double)uibits);
  }
  if (pcPic->getSlice()->isInterP())
  {
    m_gcAnalyzeP.addResult (dYPSNR, dUPSNR, dVPSNR, (Double)uibits);
  }
  if (pcPic->getSlice()->isInterB())
  {
    m_gcAnalyzeB.addResult (dYPSNR, dUPSNR, dVPSNR, (Double)uibits);
  }
  
  //===== output =====
  TComSlice*  pcSlice = pcPic->getSlice();
  printf("\nPOC %4d ( %c-SLICE, QP %d ) %10d bits ",
         pcSlice->getPOC(),
         pcSlice->isIntra() ? 'I' : pcSlice->isInterP() ? 'P' : 'B',
         pcSlice->getSliceQp(),
         uibits );
  printf( "[Y %6.4lf dB    U %6.4lf dB    V %6.4lf dB]  ", dYPSNR, dUPSNR, dVPSNR );
  printf ("[ET %5.0f ] ", dEncTime );
  
  for (Int iRefList = 0; iRefList < 2; iRefList++)
  {
    printf ("[L%d ", iRefList);
    for (Int iRefIndex = 0; iRefIndex < pcSlice->getNumRefIdx(RefPicList(iRefList)); iRefIndex++)
    {
      printf ("%d ", pcSlice->getRefPOC(RefPicList(iRefList), iRefIndex));
    }
    printf ("] ");
  }
  
  fflush(stdout);
}


#include <string>
#include "TAppEncMultiView.h"
#include "../../Lib/TLibCommon/TComPicYuv.h"
#include "../../Lib/TLibEncoder/TEncCfg.h"
#include "../../Lib/TLibVideoIO/TVideoIOYuv.h"

TAppEncMultiView::TAppEncMultiView( Void )
:m_pcEncCfg(NULL)
{}

TAppEncMultiView::~TAppEncMultiView( Void )
{
	closeMultiView();
}

Void TAppEncMultiView::openMultiView( char* pchFileNamePrefix, TEncCfg* pcEncCfg, Direction eDirection )
{
	m_pcEncCfg = pcEncCfg;

	UInt uiCurViewIndex;
	for ( uiCurViewIndex = 0; uiCurViewIndex <= m_pcEncCfg->getNumViewsMinusOne(); uiCurViewIndex++ )
	{
		if ( m_pcEncCfg->getViewOrder()[uiCurViewIndex] == m_pcEncCfg->getCurrentViewID() )
			break;
	}

#define OPEN_FILE(TYPE, LIST)																	\
	for ( UInt i = 0; i < m_pcEncCfg->getNum##TYPE##Refs##LIST##()[uiCurViewIndex]; i++ )		\
	{																							\
		std::string filename;																	\
		filename += pchFileNamePrefix;															\
		filename += '_';																		\
		filename += _itoa(m_pcEncCfg->get##TYPE##Ref##LIST##()[uiCurViewIndex][i], pos, 10);	\
		filename += ".yuv";																		\
		TVideoIOYuv* pFile = new TVideoIOYuv;													\
		pFile->open( const_cast<char*>(filename.c_str()), false );								\
		m_cList##TYPE##ViewFiles.pushBack( pFile );												\
	}

	// [KSI] uiCurViewIndex == 0 이면, BaseView 이므로 Inter-view prediction하지 않는다.
	if ( uiCurViewIndex != 0 )
	{
		char pos[10];
		if ( eDirection == FWD )
		{
			OPEN_FILE(Anchor, L0)
			OPEN_FILE(NonAnchor, L0)
		}
		else if ( eDirection == BWD )
		{
			OPEN_FILE(Anchor, L1)
			OPEN_FILE(NonAnchor, L1)
		}
	}
#undef OPEN_FILE
}

Void TAppEncMultiView::closeMultiView( Void )
{
	TComList<TVideoIOYuv*>::iterator iter;

#define CLOSE_FILE(LIST)							\
	iter = m_cList##LIST##ViewFiles.begin();		\
	while( iter != m_cList##LIST##ViewFiles.end() )	\
	{												\
		(*iter)->close();							\
		delete (*iter);								\
		(*iter) = NULL;								\
		iter++;										\
	}

	CLOSE_FILE(Anchor)
	CLOSE_FILE(NonAnchor)

#undef CLOSE_FILE
}

Void TAppEncMultiView::generateMultiViewList( TComList<TComPicYuv*>& rcListMultiView, Bool bAnchor )
{
	if ( !rcListMultiView.empty() ) destroyMultiViewList( rcListMultiView );

	TComList<TVideoIOYuv*>::iterator iter;

	iter = m_cListAnchorViewFiles.begin();
	while( iter != m_cListAnchorViewFiles.end() )
	{
		Int aiPad[2];
		aiPad[0] = m_pcEncCfg->getPad(0);
		aiPad[1] = m_pcEncCfg->getPad(1);
		TComPicYuv* pcPic = new TComPicYuv;
		pcPic->create( m_pcEncCfg->getSourceWidth(), m_pcEncCfg->getSourceHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
		(*iter)->read( pcPic, aiPad	); // TODO : [KSI] read를 실패하는지 성공하는지 판단 할 방법이 없다.
		if ( bAnchor ) rcListMultiView.pushBack(pcPic);
		else           { pcPic->destroy(); delete pcPic; }
		iter++;
	}
	
	iter = m_cListNonAnchorViewFiles.begin();
	while( iter != m_cListNonAnchorViewFiles.end() )
	{
		Int aiPad[2];
		aiPad[0] = m_pcEncCfg->getPad(0);
		aiPad[1] = m_pcEncCfg->getPad(1);
		TComPicYuv* pcPic = new TComPicYuv;
		pcPic->create( m_pcEncCfg->getSourceWidth(), m_pcEncCfg->getSourceHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
		(*iter)->read( pcPic, aiPad	); // TODO : [KSI] read를 실패하는지 성공하는지 판단 할 방법이 없다.
		if ( !bAnchor ) rcListMultiView.pushBack(pcPic);
		else            { pcPic->destroy(); delete pcPic; }
		iter++;
	}

}

Void TAppEncMultiView::destroyMultiViewList( TComList<TComPicYuv*>& rcListMultiView )
{
	TComList<TComPicYuv*>::iterator iter;

	iter = rcListMultiView.begin();
	while( iter != rcListMultiView.end() )
	{
		(*iter)->destroy();
		delete (*iter);
		(*iter) = NULL;
		iter++;
	}

	rcListMultiView.clear();
}


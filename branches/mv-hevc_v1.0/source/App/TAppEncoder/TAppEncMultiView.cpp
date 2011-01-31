#include <string>
#include "TAppEncMultiView.h"

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
	
	if ( uiCurViewIndex != 0 ) // [KSI] uiCurViewIndex == 0 이면, BaseView 이므로 Inter-view prediction하지 않는다.
	{
		char pos[10];
		if ( eDirection == FWD )
		{
			for ( UInt i = 0; i < m_pcEncCfg->getNumAnchorRefsL0()[uiCurViewIndex]; i++ )
			{
				std::string filename;
				filename += pchFileNamePrefix;
				filename += '_';
				filename += _itoa(m_pcEncCfg->getAnchorRefL0()[uiCurViewIndex][i], pos, 10);
				filename += ".yuv";
				TVideoIOYuv* pFile = new TVideoIOYuv;
				pFile->open( const_cast<char*>(filename.c_str()), false );
				m_cListAnchorViewFiles.pushBack( pFile );
			}

			for ( UInt i = 0; i < m_pcEncCfg->getNumNonAnchorRefsL0()[uiCurViewIndex]; i++ )
			{
				std::string filename;
				filename += pchFileNamePrefix;
				filename += '_';
				filename += _itoa(m_pcEncCfg->getNonAnchorRefL0()[uiCurViewIndex][i], pos, 10);
				filename += ".yuv";
				TVideoIOYuv* pFile = new TVideoIOYuv;
				pFile->open( const_cast<char*>(filename.c_str()), false );
				m_cListNonAnchorViewFiles.pushBack( pFile );
			}
		}
		else if ( eDirection == BWD )
		{
			for ( UInt i = 0; i < m_pcEncCfg->getNumAnchorRefsL1()[uiCurViewIndex]; i++ )
			{
				std::string filename;
				filename += pchFileNamePrefix;
				filename += '_';
				filename += _itoa(m_pcEncCfg->getAnchorRefL1()[uiCurViewIndex][i], pos, 10);
				filename += ".yuv";
				TVideoIOYuv* pFile = new TVideoIOYuv;
				pFile->open( const_cast<char*>(filename.c_str()), false );
				m_cListAnchorViewFiles.pushBack( pFile );
			}

			for ( UInt i = 0; i < m_pcEncCfg->getNumNonAnchorRefsL1()[uiCurViewIndex]; i++ )
			{
				std::string filename;
				filename += pchFileNamePrefix;
				filename += '_';
				filename += _itoa(m_pcEncCfg->getNonAnchorRefL1()[uiCurViewIndex][i], pos, 10);
				filename += ".yuv";
				TVideoIOYuv* pFile = new TVideoIOYuv;
				pFile->open( const_cast<char*>(filename.c_str()), false );
				m_cListNonAnchorViewFiles.pushBack( pFile );
			}
		}
	}
}

Void TAppEncMultiView::closeMultiView( Void )
{
	TComList<TVideoIOYuv*>::iterator iter;

	iter = m_cListAnchorViewFiles.begin();
	while( iter != m_cListAnchorViewFiles.end() )
	{
		(*iter)->close();
		delete (*iter);
		(*iter) = NULL;
		iter++;
	}

	iter = m_cListNonAnchorViewFiles.begin();
	while( iter != m_cListNonAnchorViewFiles.end() )
	{
		(*iter)->close();
		delete (*iter);
		(*iter) = NULL;
		iter++;
	}
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
		(*iter)->read( pcPic, aiPad	);
		if ( bAnchor ) rcListMultiView.pushBack(pcPic);
		else           delete pcPic;
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
		(*iter)->read( pcPic, aiPad	);
		if ( !bAnchor ) rcListMultiView.pushBack(pcPic);
		else            delete pcPic;
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


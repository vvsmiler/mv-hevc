#include "TAppExtractor.h"
#include "TAppExtractorCfg.h"

/// maximum bitstream buffer Size per 1 picture (1920*1080*1.5)
/** \todo fix this value according to suitable value
 */
#define BITS_BUF_SIZE     3110400

TAppExtractor::TAppExtractor(TAppExtractorCfg* pcAppAssemCfg)
:m_pcAppAssemCfg(pcAppAssemCfg)
{}

TAppExtractor::~TAppExtractor(Void)
{
	for ( UInt i = 0; i < m_pcAppAssemCfg->getNumViews(); i++ )
	{
		m_apcOutputBitstreamFile[i].closeBits();
	}
	m_cInputBitstreamFile.closeBits();
}

Bool TAppExtractor::go(Void)
{
#if HHI_NAL_UNIT_SYNTAX
	m_apcOutputBitstreamFile = new TVideoIOBitsStartCode[m_pcAppAssemCfg->getNumViews()];
#else
	m_apcOutputBitstreamFile = new TVideoIOBits[m_pcAppAssemCfg->getNumViews()];
#endif

	for ( UInt i = 0; i < m_pcAppAssemCfg->getNumViews(); i++ )
	{
		m_apcOutputBitstreamFile[i].openBits(const_cast<char*>(m_pcAppAssemCfg->getOutputFileName(i).c_str()), true); // write
	}
	m_cInputBitstreamFile.openBits(const_cast<char*>(m_pcAppAssemCfg->getInputFileName().c_str()), false); // read
	return xExtract();
}

Bool TAppExtractor::xExtract(Void)
{
	UInt uiViewIndex = 0;
	Bool bEos = false;
	TComBitstream* pcBitstream = new TComBitstream;
	pcBitstream->create(BITS_BUF_SIZE);

	bEos = m_cInputBitstreamFile.readBits(pcBitstream);
	while ( !bEos )
	{
		pcBitstream->convertRBSPToPayload(0);
		switch ( char(pcBitstream->getStartStream()[0])&0x1F )
		{
		case NAL_UNIT_SPS:
		case NAL_UNIT_PPS:
		case NAL_UNIT_SUBSET_SPS:
			{
				for ( UInt i = 0; i < m_pcAppAssemCfg->getNumViews(); i++ )
				{
					m_apcOutputBitstreamFile[i].writeBits(pcBitstream);
				}
				break;
			}
		case NAL_UNIT_CODED_SLICE_PREFIX:
			{
				m_apcOutputBitstreamFile[uiViewIndex].writeBits(pcBitstream);
				break;
			}
		case NAL_UNIT_CODED_SLICE:
		case NAL_UNIT_CODED_SLICE_IDR:
		case NAL_UNIT_CODED_SLICE_LAYER_EXTENSION:
			{
				m_apcOutputBitstreamFile[uiViewIndex].writeBits(pcBitstream);
				uiViewIndex = ++uiViewIndex%m_pcAppAssemCfg->getNumViews();
				break;
			}
		}
		
		bEos = m_cInputBitstreamFile.readBits(pcBitstream);
	}

	delete pcBitstream;



	return false;
}
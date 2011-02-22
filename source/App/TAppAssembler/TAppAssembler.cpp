#include "TAppAssembler.h"
#include "TAppAssemCfg.h"

/// maximum bitstream buffer Size per 1 picture (1920*1080*1.5)
/** \todo fix this value according to suitable value
 */
#define BITS_BUF_SIZE     3110400

TAppAssembler::TAppAssembler(TAppAssemCfg* pcAppAssemCfg)
:m_pcAppAssemCfg(pcAppAssemCfg)
{}

TAppAssembler::~TAppAssembler(Void)
{
	for ( UInt i = 0; i < m_pcAppAssemCfg->getNumViews(); i++ )
	{
		m_apcInputBitstreamFile[i].closeBits();
	}
	m_cOutputBitstreamFile.closeBits();
}

Bool TAppAssembler::go(Void)
{
#if HHI_NAL_UNIT_SYNTAX
	m_apcInputBitstreamFile = new TVideoIOBitsStartCode[m_pcAppAssemCfg->getNumViews()];
#else
	m_apcInputBitstreamFile = new TVideoIOBits[m_pcAppAssemCfg->getNumViews()];
#endif

	for ( UInt i = 0; i < m_pcAppAssemCfg->getNumViews(); i++ )
	{
		m_apcInputBitstreamFile[i].openBits(const_cast<char*>(m_pcAppAssemCfg->getInputFileName(i).c_str()), false); // read
	}
	m_cOutputBitstreamFile.openBits(const_cast<char*>(m_pcAppAssemCfg->getOutputFileName().c_str()), true); // read
	return xAssemble();
}

Bool TAppAssembler::xAssemble(Void)
{
	UInt uiViewIndex = 0;
	Bool bEos = false;
	TComBitstream* pcBitstream = new TComBitstream;
	pcBitstream->create(BITS_BUF_SIZE);

	bEos = m_apcInputBitstreamFile[uiViewIndex].readBits(pcBitstream);
	while ( !bEos )
	{
		pcBitstream->convertRBSPToPayload(0);
		switch ( char(pcBitstream->getStartStream()[0])&0x1F )
		{
		case NAL_UNIT_SPS:
		case NAL_UNIT_PPS:
		case NAL_UNIT_SUBSET_SPS:
			{
				if ( uiViewIndex == 0)
					m_cOutputBitstreamFile.writeBits(pcBitstream);
				uiViewIndex = ++uiViewIndex%m_pcAppAssemCfg->getNumViews();
				break;
			}
		case NAL_UNIT_CODED_SLICE_PREFIX:
			{
				m_cOutputBitstreamFile.writeBits(pcBitstream);
				break;
			}
		case NAL_UNIT_CODED_SLICE:
		case NAL_UNIT_CODED_SLICE_IDR:
		case NAL_UNIT_CODED_SLICE_LAYER_EXTENSION:
			{
				m_cOutputBitstreamFile.writeBits(pcBitstream);
				uiViewIndex = ++uiViewIndex%m_pcAppAssemCfg->getNumViews();
				break;
			}
		}
		
		bEos = m_apcInputBitstreamFile[uiViewIndex].readBits(pcBitstream);
	}

	delete pcBitstream;



	return false;
}
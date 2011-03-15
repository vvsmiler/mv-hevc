#include "TAppExtractorCfg.h"
#include "TAppExtractor.h"

int main(int argc, char** argv)
{
	TAppExtractorCfg cfg;
	cfg.parseCfg(argc, argv);

	TAppExtractor extractor(&cfg);
	extractor.go();

	return 0;
}
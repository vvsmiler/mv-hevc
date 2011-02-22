#include "TAppAssemCfg.h"
#include "TAppAssembler.h"

int main(int argc, char** argv)
{
	TAppAssemCfg cfg;
	cfg.parseCfg(argc, argv);

	TAppAssembler assembler(&cfg);
	assembler.go();

	return 0;
}
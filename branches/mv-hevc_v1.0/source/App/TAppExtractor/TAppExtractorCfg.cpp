#include "TAppExtractorCfg.h"
#include "../../App/TAppCommon/program_options_lite.h"

namespace po = df::program_options_lite;

class OptionSpecificExtractor;
struct OptionsExtractor: public po::Options
{
	OptionsExtractor(TAppExtractorCfg& AppAssemCfg_) : AppExtractorCfg(AppAssemCfg_) {}
	OptionSpecificExtractor addOptionsExtract();
	TAppExtractorCfg& AppExtractorCfg;
};

struct OptionFuncExtract : public po::OptionBase
{
	typedef void (Func)(OptionFuncExtract&, const std::string&);

	OptionFuncExtract(const std::string& name, OptionsExtractor& parent_, Func *func_, const std::string& desc)
		: po::OptionBase(name, desc), func(func_), parent(parent_)
	{}

	void parse(const std::string& arg)
	{
		func(*this, arg);
	}

	void setDefault()
	{
		return;
	}

	void (*func)(OptionFuncExtract&, const std::string&);
	OptionsExtractor& parent;
};

class OptionSpecificExtractor
{
public:
	OptionSpecificExtractor(OptionsExtractor& parent_) : parent(parent_) {}
public:
	template<typename T>
	OptionSpecificExtractor& operator()(const std::string& name, T& storage, T default_val, const std::string& desc = "")
	{
		parent.addOption(new po::Option<T>(name, storage, default_val, desc));
		return *this;
	}
	OptionSpecificExtractor& operator()(const std::string& name, OptionFuncExtract::Func *func, const std::string& desc = "")
	{
		parent.addOption(new OptionFuncExtract(name, parent, func, desc));
		return *this;
	}
private:
	OptionsExtractor&	parent;
};

OptionSpecificExtractor OptionsExtractor::addOptionsExtract()
{
	return OptionSpecificExtractor(*this);
}

void parseOutputFileName(OptionFuncExtract& opt, const std::string& val)
{
	opt.parent.AppExtractorCfg.m_vecOutputFileName.push_back(val);
}

TAppExtractorCfg::TAppExtractorCfg(Void)
:m_uiNumViews(0)
{
}

TAppExtractorCfg::~TAppExtractorCfg(Void)
{
}

Void TAppExtractorCfg::create()
{
}

Void TAppExtractorCfg::destroy()
{
}

Bool TAppExtractorCfg::parseCfg( Int argc, Char* argv[] )
{
	OptionsExtractor opts(*this);
	opts.addOptions()
		("c", po::parseConfigFile, "configuration file name");

	opts.addOptionsExtract()
		("NumberOfViews", m_uiNumViews, 0u, "Number of views")
		("OutputFile", parseOutputFileName, "Output file name" )
		("InputFile", m_cInputFileName, std::string(""), "Input file name");

	po::setDefaults(opts);
	po::scanArgv(opts, argc, (const char**) argv);

	return true;
}
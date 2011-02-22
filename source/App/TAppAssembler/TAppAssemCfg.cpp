#include "TAppAssemCfg.h"
#include "../../App/TAppCommon/program_options_lite.h"

namespace po = df::program_options_lite;

class OptionSpecificAssemble;
struct OptionsAssemble: public po::Options
{
	OptionsAssemble(TAppAssemCfg& AppAssemCfg_) : AppAssemCfg(AppAssemCfg_) {}
	OptionSpecificAssemble addOptionsAssemble();
	TAppAssemCfg& AppAssemCfg;
};

struct OptionFuncAssemble : public po::OptionBase
{
	typedef void (Func)(OptionFuncAssemble&, const std::string&);

	OptionFuncAssemble(const std::string& name, OptionsAssemble& parent_, Func *func_, const std::string& desc)
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

	void (*func)(OptionFuncAssemble&, const std::string&);
	OptionsAssemble& parent;
};

class OptionSpecificAssemble
{
public:
	OptionSpecificAssemble(OptionsAssemble& parent_) : parent(parent_) {}
public:
	template<typename T>
	OptionSpecificAssemble& operator()(const std::string& name, T& storage, T default_val, const std::string& desc = "")
	{
		parent.addOption(new po::Option<T>(name, storage, default_val, desc));
		return *this;
	}
	OptionSpecificAssemble& operator()(const std::string& name, OptionFuncAssemble::Func *func, const std::string& desc = "")
	{
		parent.addOption(new OptionFuncAssemble(name, parent, func, desc));
		return *this;
	}
private:
	OptionsAssemble&	parent;
};

OptionSpecificAssemble OptionsAssemble::addOptionsAssemble()
{
	return OptionSpecificAssemble(*this);
}

void parseInputFileName(OptionFuncAssemble& opt, const std::string& val)
{
	opt.parent.AppAssemCfg.m_vecInputFileName.push_back(val);
}

TAppAssemCfg::TAppAssemCfg(Void)
:m_uiNumViews(0)
{
}

TAppAssemCfg::~TAppAssemCfg(Void)
{
}

Void TAppAssemCfg::create()
{
}

Void TAppAssemCfg::destroy()
{
}

Bool TAppAssemCfg::parseCfg( Int argc, Char* argv[] )
{
	OptionsAssemble opts(*this);
	opts.addOptions()
	("c", po::parseConfigFile, "configuration file name");
	
	opts.addOptionsAssemble()
	("NumberOfViews", m_uiNumViews, 0u, "Number of views")
	("InputFile", parseInputFileName, "Input file name" )
	("OutputFile", m_cOutputFileName, std::string(""), "Output file name");
	
	po::setDefaults(opts);
	po::scanArgv(opts, argc, (const char**) argv);

	return true;
}
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

/** \file     TAppExtractorCfg.h
    \brief    Handle extractor configuration parameters (header)
*/

#ifndef __TAPPEXTRACTORCFG__
#define __TAPPEXTRACTORCFG__

#include "../../Lib/TLibCommon/CommonDef.h"
#include <string>
#include <vector>

struct OptionFuncExtract;
void parseOutputFileName(OptionFuncExtract& opt, const std::string& val);
class TAppExtractorCfg
{
	friend void parseOutputFileName(OptionFuncExtract& opt, const std::string& val);
public:
	TAppExtractorCfg(Void);
	~TAppExtractorCfg(Void);
public:
	Void  create(Void);                                     ///< create option handling class
	Void  destroy(Void);                                    ///< destroy option handling class
	Bool  parseCfg(Int argc, Char* argv[]);                 ///< parse configuration file to fill member variables
public:
	const std::string& getOutputFileName(UInt uiViewIndex) const    { return m_vecOutputFileName[uiViewIndex]; }
	const std::string& getInputFileName(Void) const                 { return m_cInputFileName; }
	const UInt getNumViews(Void) const                              { return m_uiNumViews; }
protected:
	UInt						m_uiNumViews;
	std::vector<std::string>	m_vecOutputFileName;
	std::string					m_cInputFileName;
};

#endif
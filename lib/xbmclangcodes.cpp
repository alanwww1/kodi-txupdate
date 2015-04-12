/*
 *      Copyright (C) 2005-2014 Team Kodi
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <string>
#include <stdio.h>
#include "xbmclangcodes.h"
#include "Log.h"
#include "HTTPUtils.h"
#include "JSONHandler.h"
#include "TinyXML/tinyxml.h"

using namespace std;

CLCodeHandler g_LCodeHandler;

CLCodeHandler::CLCodeHandler()
{}

CLCodeHandler::~CLCodeHandler()
{}

void CLCodeHandler::Init(std::string strURL)
{
  g_HTTPHandler.Cleanup();
  g_HTTPHandler.ReInit();
  std::string strtemp = g_HTTPHandler.GetURLToSTR(strURL);
  if (strtemp.empty())
    CLog::Log(logERROR, "XBMCLangCode::Init: error getting available language list from URL %s", strURL.c_str());

  m_mapLCodes = g_Json.ParseTransifexLanguageDatabase(strtemp);

  CLog::Log(logINFO, "LCodeHandler: Succesfully fetched %i language codes from URL %s", m_mapLCodes.size(), strURL.c_str());
}

int CLCodeHandler::GetnPlurals(std::string LangCode)
{
  if (m_mapLCodes.find(LangCode) != m_mapLCodes.end())
    return m_mapLCodes[LangCode].nplurals;
  CLog::Log(logERROR, "LangCodes: GetnPlurals: unable to find langcode: %s", LangCode.c_str());
  return 0;
}

std::string CLCodeHandler::GetPlurForm(std::string LangCode)
{
  if (m_mapLCodes.find(LangCode) != m_mapLCodes.end())
    return m_mapLCodes[LangCode].Pluralform;
  CLog::Log(logERROR, "LangCodes: GetPlurForm: unable to find langcode: %s", LangCode.c_str());
  return "(n != 1)";
}

std::string CLCodeHandler::GetLangFromLCode(std::string LangCode, std::string AliasForm)
{
  if (m_mapLCodes.find(LangCode) != m_mapLCodes.end() &&
      m_mapLCodes[LangCode].mapLangdata.find(AliasForm) != m_mapLCodes[LangCode].mapLangdata.end())
    return m_mapLCodes[LangCode].mapLangdata[AliasForm];
  CLog::Log(logERROR, "LangCodes:GetLangFromLCode: unable to find language for langcode: %s", LangCode.c_str());
  return "UNKNOWN";
}

std::string CLCodeHandler::GetLangCodeFromAlias(std::string Alias, std::string AliasForm)
{
  if (Alias == "")
    return "UNKNOWN";

  for (itmapLCodes = m_mapLCodes.begin(); itmapLCodes != m_mapLCodes.end() ; itmapLCodes++)
  {
    std::map<std::string, std::string> mapLangdata = itmapLCodes->second.mapLangdata;
    if (itmapLCodes->second.mapLangdata.find(AliasForm) != itmapLCodes->second.mapLangdata.end() &&
        Alias == itmapLCodes->second.mapLangdata[AliasForm])
      return itmapLCodes->first;
  }
  CLog::Log(logERROR, "LangCodes:GetLangCodeFromAlias unable to find langcode for alias: %s", Alias.c_str());
  return "UNKNOWN";
}

std::string CLCodeHandler::VerifyLangCode(std::string LangCode)
{
  std::string strOldCode = LangCode;

  // common mistakes, we correct them on the fly
  if (LangCode == "kr") LangCode = "ko";
  if (LangCode == "cr") LangCode = "hr";
  if (LangCode == "cz") LangCode = "cs";

  if (strOldCode != LangCode)
    CLog::Log(logWARNING, "LangCodes: problematic language code: %s was corrected to %s", strOldCode.c_str(), LangCode.c_str());

  if (m_mapLCodes.find(LangCode) != m_mapLCodes.end())
    return LangCode;
  CLog::Log(logINFO, "LangCodes::VerifyLangCode: unable to find language code: %s", LangCode.c_str());
  return "UNKNOWN";
}

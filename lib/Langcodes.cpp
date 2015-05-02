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
#include "Langcodes.h"
#include "Settings.h"
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

void CLCodeHandler::Init()
{
  std::string strURL = g_Settings.GetLangDatabaseURL();
  g_HTTPHandler.Cleanup();
  g_HTTPHandler.ReInit(); 

  printf("-----------------------------\n");
  printf("DOWNLOADING LANGUAGE DATABASE\n");
  printf("-----------------------------\n");

  // We get the version of the language database files here
  std::string strGitHubURL = g_HTTPHandler.GetGitHUBAPIURL(strURL.substr(0,strURL.find_last_of("/")+1));
  printf("Langdatabaseversion");
  std::string strtemp = g_HTTPHandler.GetURLToSTR(strGitHubURL);
  if (strtemp.empty())
    CLog::Log(logERROR, "CLCodeHandler::Init: error getting language file version from github.com with URL: %s", strURL.c_str());

  g_Json.ParseLangDatabaseVersion(strtemp, strURL);

  printf(" Langdatabase");
  strtemp = g_HTTPHandler.GetURLToSTR(strURL);
  if (strtemp.empty())
    CLog::Log(logERROR, "LangCode::Init: error getting available language list from URL %s", strURL.c_str());

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
  CleanLangform(AliasForm);
  if (m_mapLCodes.find(LangCode) != m_mapLCodes.end() &&
      m_mapLCodes[LangCode].mapLangdata.find(AliasForm) != m_mapLCodes[LangCode].mapLangdata.end())
    return m_mapLCodes[LangCode].mapLangdata[AliasForm];
  return "";
}

std::string CLCodeHandler::GetLangCodeFromAlias(std::string Alias, std::string AliasForm)
{
  if (Alias == "")
    return "";

  CleanLangform(AliasForm);

  for (itmapLCodes = m_mapLCodes.begin(); itmapLCodes != m_mapLCodes.end() ; itmapLCodes++)
  {
    std::map<std::string, std::string> mapLangdata = itmapLCodes->second.mapLangdata;
    if (itmapLCodes->second.mapLangdata.find(AliasForm) != itmapLCodes->second.mapLangdata.end() &&
        Alias == itmapLCodes->second.mapLangdata[AliasForm])
      return itmapLCodes->first;
  }
  return "";
}

std::string CLCodeHandler::VerifyLangCode(std::string LangCode, const std::string &strLangformat)
{
  if (strLangformat == "$(OLDLCODE)")
  {
    std::string strOldCode = LangCode;

    // common mistakes, we correct them on the fly
    if (LangCode == "kr") LangCode = "ko";
    if (LangCode == "cr") LangCode = "hr";
    if (LangCode == "cz") LangCode = "cs";

    if (strOldCode != LangCode)
      CLog::Log(logWARNING, "LangCodes: problematic language code: %s was corrected to %s", strOldCode.c_str(), LangCode.c_str());
  }

  if ((LangCode = GetLangCodeFromAlias(LangCode, strLangformat)) != "")
    return LangCode;
  return "";
}

void CLCodeHandler::CleanLangform (std::string &strLangform)
{
  size_t pos1, pos2;
  pos1 = strLangform.find_first_not_of("$(");
  pos2 = strLangform.find_last_not_of(")");
  strLangform = strLangform.substr(pos1, pos2-pos1+1);
}

std::map<std::string, std::string>  CLCodeHandler::GetTranslatorsDatabase(std::string strContributorType)
{
  std::map<std::string, std::string> mapOfContributors;

  g_HTTPHandler.Cleanup();
  g_HTTPHandler.ReInit(); 

  for (itmapLCodes = m_mapLCodes.begin(); itmapLCodes != m_mapLCodes.end() ; itmapLCodes++)
  {
    std::string strLangCode = itmapLCodes->first;
    std::string strTXLformat = g_Settings.GetDefaultTXLFormat();

    std::string strJson = g_HTTPHandler.GetURLToSTR("https://www.transifex.com/api/2/project/"+ g_Settings.GetProjectname() + "/language/" +
                                                     GetLangFromLCode(strLangCode, strTXLformat) + "/" + strContributorType + "/");
    if (strJson.empty())
      CLog::Log(logERROR, "CLCodeHandler::GetTranslatorsDatabase: error getting translator groups list for project: %s", g_Settings.GetProjectname().c_str());

    Json::Value root;   // will contains the root value after parsing.
    Json::Reader reader;

    bool parsingSuccessful = reader.parse(strJson, root );
    if ( !parsingSuccessful )
      CLog::Log(logERROR, "CLCodeHandler::GetTranslatorsDatabase: no valid JSON data downloaded from Transifex");

    const Json::Value JRoot = root;
    const Json::Value JNames = JRoot[strContributorType];

    printf ("\n%s%s%s", KMAG, strLangCode.c_str(), RESET);

    std::list<std::string> listNames;

    for(Json::ValueIterator itr = JNames.begin() ; itr !=JNames.end() ; itr++)
    {
      Json::Value JValu = *itr;
      std::string strName =JValu.asString();

      if (strName == "")
        CLog::Log(logERROR, "CJSONHandler::ParseTranslatorsDatabase: no valid JSON data downloaded from Github");

      printf ("%s%s%s ", KCYN, strName.c_str(), RESET);
      listNames.push_back(strName);
    }

    if (!listNames.empty())
      mapOfContributors[strLangCode] = strJson;
  };
  return mapOfContributors;
}

void  CLCodeHandler::UploadTranslatorsDatabase(std::map<std::string, std::string> &mapOfContributors,
                                               std::string strContributorType)
{

  g_HTTPHandler.Cleanup();
  g_HTTPHandler.ReInit();

  std::string strTXLformat = g_Settings.GetTargetTXLFormat();

  for (std::map<std::string, std::string>::iterator itmap = mapOfContributors.begin(); itmap !=mapOfContributors.end(); itmap++)
  {
    std::string strLangCode = itmap->first;
    std::string strURLSource = "https://www.transifex.com/api/2/project/"+ g_Settings.GetProjectname() + "/language/" +
                         GetLangFromLCode(strLangCode, strTXLformat) + "/" + strContributorType + "/";

    std::string strURLTarget = "https://www.transifex.com/api/2/project/"+ g_Settings.GetTargetProjectname() + "/language/" +
                         GetLangFromLCode(strLangCode, g_Settings.GetTargetTXLFormat()) + "/" + strContributorType + "/";

    std::string strCacheFile = g_HTTPHandler.CacheFileNameFromURL(strURLSource);
    strCacheFile = g_HTTPHandler.GetCacheDir() + "GET/" + strCacheFile;

    bool bCacheFileExists = g_File.FileExist(strCacheFile);

    if (!bCacheFileExists)
      CLog::Log(logERROR, "CLCodeHandler::UploadTranslatorsDatabase No previous cachefile exeists for file: %s", strCacheFile.c_str());


    printf ("%s%s%s ", KMAG, strLangCode.c_str(), RESET);
    size_t stradded, strupd;
    long result = g_HTTPHandler.curlPUTPOFileToURL(strCacheFile, strURLTarget, stradded, strupd, false);
    if (result < 200 || result >= 400)
    {
      CLog::Log(logERROR, "CLCodeHandler::UploadTranslatorsDatabase File upload was unsuccessful, http errorcode: %i", result);
    }
  }
}
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
#include "Log.h"
#include "HTTPUtils.h"
#include "JSONHandler.h"
#include "TinyXML/tinyxml.h"
#include "algorithm"

using namespace std;

CLCodeHandler g_LCodeHandler;

CLCodeHandler::CLCodeHandler()
{}

CLCodeHandler::~CLCodeHandler()
{}

void CLCodeHandler::Init(const std::string strLangDatabaseURL, const CXMLResdata& XMLResData)
{
  g_HTTPHandler.Cleanup();
  g_HTTPHandler.ReInit(); 

  printf("-----------------------------\n");
  printf("DOWNLOADING LANGUAGE DATABASE\n");
  printf("-----------------------------\n");

  // We get the version of the language database files here
  std::string strGitHubURL = g_HTTPHandler.GetGitHUBAPIURL(strLangDatabaseURL.substr(0,strLangDatabaseURL.find_last_of("/")+1));
  printf("Langdatabaseversion");
  std::string strtemp = g_HTTPHandler.GetURLToSTR(strGitHubURL);
  if (strtemp.empty())
    CLog::Log(logERROR, "CLCodeHandler::Init: error getting language file version from github.com with URL: %s", strLangDatabaseURL.c_str());

  g_Json.ParseLangDatabaseVersion(strtemp, strLangDatabaseURL);

  printf(" Langdatabase");
  strtemp = g_HTTPHandler.GetURLToSTR(strLangDatabaseURL);
  if (strtemp.empty())
    CLog::Log(logERROR, "LangCode::Init: error getting available language list from URL %s", strLangDatabaseURL.c_str());

  m_mapLCodes = ParseTransifexLanguageDatabase(strtemp, XMLResData);

  CLog::Log(logINFO, "LCodeHandler: Succesfully fetched %i language codes from URL %s", m_mapLCodes.size(), strLangDatabaseURL.c_str());
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

std::map<std::string, std::string>  CLCodeHandler::GetTranslatorsDatabase(const std::string& strContributorType, const std::string& strProjectName,
                                                                          const CXMLResdata& XMLResData)
{
  std::map<std::string, std::string> mapOfContributors;

  g_HTTPHandler.Cleanup();
  g_HTTPHandler.ReInit(); 

  for (itmapLCodes = m_mapLCodes.begin(); itmapLCodes != m_mapLCodes.end() ; itmapLCodes++)
  {
    std::string strLangCode = itmapLCodes->first;
    std::string strTXLformat = XMLResData.strDefTXLFormat;

    std::string strJson = g_HTTPHandler.GetURLToSTR("https://www.transifex.com/api/2/project/"+ strProjectName + "/language/" +
                                                     GetLangFromLCode(strLangCode, strTXLformat) + "/" + strContributorType + "/");
    if (strJson.empty())
      CLog::Log(logERROR, "CLCodeHandler::GetTranslatorsDatabase: error getting translator groups list for project: %s", strProjectName.c_str());

    Json::Value root;   // will contains the root value after parsing.
    Json::Reader reader;

    bool parsingSuccessful = reader.parse(strJson, root );
    if ( !parsingSuccessful )
      CLog::Log(logERROR, "CLCodeHandler::GetTranslatorsDatabase: no valid JSON data downloaded from Transifex");

    const Json::Value JRoot = root;
    const Json::Value JNames = JRoot[strContributorType];

    printf ("\n%s%s%s ", KMAG, strLangCode.c_str(), RESET);

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
      mapOfContributors[strLangCode] = g_CharsetUtils.CleanTranslatorlist(strJson);
  };
  return mapOfContributors;
}

void  CLCodeHandler::UploadTranslatorsDatabase(std::map<std::string, std::string> &mapOfCoordinators,
                                               std::map<std::string, std::string> &mapOfReviewers,
                                               std::map<std::string, std::string> &mapOfTranslators,
                                               const std::string& strTargetProjectName, const std::string& strTargetTXLFormat)
{
  g_HTTPHandler.Cleanup();
  g_HTTPHandler.ReInit();

  std::string strURL = "https://www.transifex.com/api/2/project/"+ strTargetProjectName + "/languages/?skip_invalid_username";

  for (std::map<std::string, std::string>::iterator itmap = mapOfCoordinators.begin(); itmap !=mapOfCoordinators.end(); itmap++)
  {
    std::string strLangCode = itmap->first;
    std::string strJson = "{\"language_code\": \"" + g_LCodeHandler.GetLangFromLCode(strLangCode, strTargetTXLFormat) + "\",";
    strJson += mapOfCoordinators[strLangCode];

    if (mapOfReviewers.find(strLangCode) != mapOfReviewers.end())
      strJson += "," + mapOfReviewers[strLangCode];

    if (mapOfTranslators.find(strLangCode) != mapOfTranslators.end())
      strJson += "," + mapOfTranslators[strLangCode];

    strJson += "}";

    printf ("%s%s%s ", KMAG, strLangCode.c_str(), RESET);
    printf("strjson: %s\nstrurl: %s\n\n\n", strJson.c_str(), strURL.c_str());

    g_HTTPHandler.UploadTranslatorsDatabase(strJson, strURL);

  }
}

std::map<std::string, CLangcodes> CLCodeHandler::ParseTransifexLanguageDatabase(std::string strJSON, const CXMLResdata& XMLResData)
{
  Json::Value root;   // will contains the root value after parsing.
  Json::Reader reader;
  std::string lang;

  bool parsingSuccessful = reader.parse(strJSON, root );
  if ( !parsingSuccessful )
  {
    CLog::Log(logERROR, "JSONHandler: ParseTXLanguageDB: no valid JSON data");
  }

  std::map<std::string, CLangcodes> mapTXLangs;

  const Json::Value JRoot = root;
  const Json::Value JLangs =  JRoot["fixtures"];
  
  for (Json::ValueIterator itrlangs = JLangs.begin() ; itrlangs !=JLangs.end() ; itrlangs++)
  {
    Json::Value JValu = *itrlangs;
    const Json::Value JAliases =JValu.get("aliases", "unknown");

    CLangcodes LangData;
    std::string strLCode;

    for (Json::ValueIterator itralias = JAliases.begin(); itralias !=JAliases.end() ; itralias++)
    {
      std::string langstrKey = itralias.key().asString();
      std::string langstrName = (*itralias).asString();
      LangData.mapLangdata[langstrKey] = langstrName;
      if ( "$(" + langstrKey + ")" == XMLResData.strBaseLCode)
        strLCode = langstrName;
    }

    if (strLCode.empty())
      CLog::Log(logERROR, "JSONHandler: ParseTXLanguageDB: Missing base langcode key in language database aliases");

    LangData.Pluralform = JValu.get("pluralequation", "unknown").asString();
    LangData.nplurals = JValu.get("nplurals", 0).asInt();

    if (!LangData.mapLangdata.empty() && LangData.Pluralform != "unknown" && LangData.nplurals != 0)
      mapTXLangs[strLCode] = LangData;
    else
      CLog::Log(logWARNING, "JSONHandler: ParseTXLanguageDB: corrupt JSON data found while parsing Language Database");
  };

  const Json::Value JRules =  JRoot.get("rules", "unknown");
  const Json::Value JRulesGen =  JRules.get("general","unknown");
  const Json::Value JRulesCust =  JRules["custom"];

  for (Json::ValueIterator itrules = JRulesGen.begin() ; itrules !=JRulesGen.end() ; itrules++)
  {
    std::string strLeft = itrules.key().asString();
    std::string strRight = (*itrules).asString();
    AddGeneralRule(mapTXLangs, strLeft, strRight);
  }

  for (Json::ValueIterator itrules = JRulesCust.begin() ; itrules !=JRulesCust.end() ; itrules++)
  {
    std::string strLangformat = itrules.key().asString();
    const Json::Value JRulesCustR = (*itrules);

    for (Json::ValueIterator itrulesR = JRulesCustR.begin() ; itrulesR !=JRulesCustR.end() ; itrulesR++)
    {
      std::string strLeft = itrulesR.key().asString(); //= itrulesR.key().asString();
      std::string strRight = (*itrulesR).asString();
      AddCustomRule(mapTXLangs, strLangformat, strLeft, strRight);
    }
  }

  return mapTXLangs;
};

void CLCodeHandler::AddGeneralRule(std::map<std::string, CLangcodes> &mapTXLangs, const std::string &strLeft,
                                  std::string strRight)
{
  std::string strModifier;
  size_t pos1, pos2;
  if ((pos1 = strRight.find("(")) != std::string::npos || pos1 == 0) //we have a modifier
  {
    pos2 = strRight.find(")");
    strModifier = strRight.substr(1, pos2-1);
    strRight = strRight.substr(pos2+1,strRight.size()-pos2);
  }

  std::map<std::string, CLangcodes>::iterator itmapTXLangs;
  for (itmapTXLangs = mapTXLangs.begin(); itmapTXLangs != mapTXLangs.end(); itmapTXLangs++)
  {
    std::string strLangnametoAdd;
    if (strModifier == "lcase")
    {
      strLangnametoAdd = itmapTXLangs->second.mapLangdata[strRight];
      std::transform(strLangnametoAdd.begin(), strLangnametoAdd.end(), strLangnametoAdd.begin(), ::tolower);
    }
    else
      strLangnametoAdd = itmapTXLangs->second.mapLangdata[strRight];

    itmapTXLangs->second.mapLangdata[strLeft] = strLangnametoAdd;
  }
}

void CLCodeHandler::AddCustomRule(std::map<std::string, CLangcodes> &mapTXLangs, const std::string &strLangformat,
                                 const std::string &strLeft, const std::string &strRight)
{
  mapTXLangs[strLeft].mapLangdata[strLangformat] = strRight;
}

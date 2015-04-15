/*
 *      Copyright (C) 2014 Team Kodi
 *      http://www.xbmc.org
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

#include "JSONHandler.h"
#include "Log.h"
#include <list>
#include <stdlib.h>
#include "Settings.h"
#include "Fileversioning.h"
#include "CharsetUtils/CharsetUtils.h"

CJSONHandler g_Json;

using namespace std;

CJSONHandler::CJSONHandler()
{};

CJSONHandler::~CJSONHandler()
{};

std::list<std::string> CJSONHandler::ParseResources(std::string strJSON)
{
  Json::Value root;   // will contains the root value after parsing.
  Json::Reader reader;
  std::string resName;
  std::list<std::string> listResources;

  bool parsingSuccessful = reader.parse(strJSON, root );
  if ( !parsingSuccessful )
  {
    CLog::Log(logERROR, "JSONHandler: Parse resource: no valid JSON data");
    return listResources;
  }

  for(Json::ValueIterator itr = root.begin() ; itr != root.end() ; itr++)
  {
    Json::Value valu = *itr;
    resName = valu.get("slug", "unknown").asString();

    if (resName.size() == 0 || resName == "unknown")
      CLog::Log(logERROR, "JSONHandler: Parse resource: no valid JSON data while iterating");
    listResources.push_back(resName);
    CLog::Log(logINFO, "JSONHandler: found resource on Transifex server: %s", resName.c_str());
  };
  CLog::Log(logINFO, "JSONHandler: Found %i resources at Transifex server", listResources.size());
  return listResources;
};

std::list<std::string> CJSONHandler::ParseAvailLanguagesTX(std::string strJSON, bool bIsKODICore,
                                                           const std::string &strURL, const std::string &strTXLangformat)
{
  Json::Value root;   // will contains the root value after parsing.
  Json::Reader reader;
  std::string LCode;
  std::list<std::string> listLangs;

  bool parsingSuccessful = reader.parse(strJSON, root );
  if ( !parsingSuccessful )
  {
    CLog::Log(logERROR, "CJSONHandler::ParseAvailLanguagesTX: no valid JSON data");
    return listLangs;
  }

  const Json::Value langs = root;
  std::string strLangsToFetch;
  std::string strLangsToDrop;

  for(Json::ValueIterator itr = langs.begin() ; itr != langs.end() ; itr++)
  {
    LCode = itr.key().asString();
    if (LCode == "unknown")
      CLog::Log(logERROR, "JSONHandler: ParseLangs: no language code in json data. json string:\n %s", strJSON.c_str());

    if (g_LCodeHandler.VerifyLangCode(LCode, strTXLangformat) == "UNKNOWN")
      continue;

    Json::Value valu = *itr;
    std::string strCompletedPerc = valu.get("completed", "unknown").asString();
    std::string strModTime = valu.get("last_update", "unknown").asString();

    // we only add language codes to the list which has a minimum ready percentage defined in the xml file
    // we make an exception with all English derived languages, as they can have only a few srings changed
    if (LCode.find("en_") != std::string::npos || strtol(&strCompletedPerc[0], NULL, 10) > g_Settings.GetMinCompletion()-1 || !bIsKODICore)
    {
      strLangsToFetch += LCode + ": " + strCompletedPerc + ", ";
      listLangs.push_back(LCode);
      g_Fileversion.SetVersionForURL(strURL + "translation/" + g_LCodeHandler.GetLangFromLCode(LCode,strTXLangformat) + "/?file", strModTime);
    }
    else
      strLangsToDrop += LCode + ": " + strCompletedPerc + ", ";
  };
  CLog::Log(logINFO, "JSONHandler: ParseAvailLangs: Languages to be Fetcehed: %s", strLangsToFetch.c_str());
  CLog::Log(logINFO, "JSONHandler: ParseAvailLangs: Languages to be Dropped (not enough completion): %s", strLangsToDrop.c_str());
  return listLangs;
};

std::list<std::string> CJSONHandler::ParseAvailLanguagesGITHUB(std::string strJSON, std::string strURL, std::string strLangformat)
{
  Json::Value root;   // will contains the root value after parsing.
  Json::Reader reader;
  std::string lang, strVersion;
  std::list<std::string> listLangs;

  bool parsingSuccessful = reader.parse(strJSON, root );
  if ( !parsingSuccessful )
    CLog::Log(logERROR, "CJSONHandler::ParseAvailLanguagesGITHUB: no valid JSON data downloaded from Github");

  const Json::Value JLangs = root;

  for(Json::ValueIterator itr = JLangs.begin() ; itr !=JLangs.end() ; itr++)
  {
    Json::Value JValu = *itr;
    std::string strType =JValu.get("type", "unknown").asString();
    if (strType == "unknown")
      CLog::Log(logERROR, "CJSONHandler::ParseAvailLanguagesGITHUB: no valid JSON data downloaded from Github");
    else if (strType != "dir")
    {
      CLog::Log(logWARNING, "CJSONHandler::ParseAvailLanguagesGITHUB: unknown file found in language directory");
      continue;
    }
    lang =JValu.get("name", "unknown").asString();
    if (lang == "unknown")
      CLog::Log(logERROR, "CJSONHandler::ParseAvailLanguagesGITHUB: no valid JSON data downloaded from Github");

    strVersion =JValu.get("sha", "unknown").asString();
    if (strVersion == "unknown")
      CLog::Log(logERROR, "CJSONHandler::ParseAvailLanguagesGITHUB: no valid sha JSON data downloaded from Github");

    std::string strMatchedLangalias = g_CharsetUtils.GetLangnameFromURL(lang, strURL, strLangformat);
    std::string strFoundLangCode = g_LCodeHandler.GetLangCodeFromAlias(strMatchedLangalias, strLangformat);
    if (strFoundLangCode != "UNKNOWN")
    {
      listLangs.push_back(strFoundLangCode);
      std::string strURLforFile = strURL;
      g_CharsetUtils.replaceAllStrParts(&strURLforFile, strLangformat, strMatchedLangalias);
      g_Fileversion.SetVersionForURL(strURLforFile, strVersion);
    }
  };

  return listLangs;
};

std::map<std::string, CLangcodes> CJSONHandler::ParseTransifexLanguageDatabase(std::string strJSON)
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
      if ( "$(" + langstrKey + ")" == g_Settings.GetBaseLCode())
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

//TODO error checking
void CJSONHandler::AddGeneralRule(std::map<std::string, CLangcodes> &mapTXLangs, const std::string &strLeft,
                                  const std::string &strRight)
{
  std::map<std::string, CLangcodes>::iterator itmapTXLangs;
  for (itmapTXLangs = mapTXLangs.begin(); itmapTXLangs != mapTXLangs.end(); itmapTXLangs++)
  {
    itmapTXLangs->second.mapLangdata[strLeft] = itmapTXLangs->second.mapLangdata[strRight];
  }
}

void CJSONHandler::AddCustomRule(std::map<std::string, CLangcodes> &mapTXLangs, const std::string &strLangformat,
                                  const std::string &strLeft, const std::string &strRight)
{
  mapTXLangs[strLeft].mapLangdata[strLangformat] = strRight;
}

std::string CJSONHandler::CreateJSONStrFromPOStr(std::string const &strPO)
{
  Json::Value root;
  root["content"] = strPO;
  root["mimetype"] = std::string("text/x-po");
  Json::StyledWriter writer;
  std::string strJSON = writer.write(root);
  return strJSON;
};

std::string CJSONHandler::CreateNewresJSONStrFromPOStr(std::string strTXResname, std::string const &strPO)
{
  Json::Value root;
  root["content"] = strPO;
  root["slug"] = std::string(strTXResname);
  root["name"] = std::string(strTXResname);
  root["i18n_type"] = std::string("PO");
  Json::StyledWriter writer;
  std::string strJSON = writer.write(root);
  return strJSON;
};

void CJSONHandler::ParseUploadedStringsData(std::string const &strJSON, size_t &stradded, size_t &strupd)
{
  Json::Value root;   // will contains the root value after parsing.
  Json::Reader reader;

  bool parsingSuccessful = reader.parse(strJSON, root );
  if ( !parsingSuccessful )
  {
    CLog::Log(logERROR, "JSONHandler::ParseUploadedStringsData: Parse upload tx server response: no valid JSON data");
    return;
  }

  stradded = root.get("strings_added", 0).asInt();
  strupd = root.get("strings_updated", 0).asInt();
  return;
};

void CJSONHandler::ParseUploadedStrForNewRes(std::string const &strJSON, size_t &stradded)
{
  Json::Value root;   // will contains the root value after parsing.
  Json::Reader reader;

  bool parsingSuccessful = reader.parse(strJSON, root );
  if ( !parsingSuccessful )
  {
    CLog::Log(logERROR, "JSONHandler::ParseUploadedStrForNewRes: Parse upload tx server response: no valid JSON data");
    return;
  }

  stradded = root[0].asInt();

  return;
};

std::string CJSONHandler::ParseLongProjectName(std::string const &strJSON)
{
  Json::Value root;   // will contains the root value after parsing.
  Json::Reader reader;

  bool parsingSuccessful = reader.parse(strJSON, root );
  if ( !parsingSuccessful )
  {
    CLog::Log(logERROR, "JSONHandler::ParseLongProjectName: no valid JSON data");
    return "";
  }

  return root.get("name", "").asString();
}

void CJSONHandler::ParseAddonXMLVersionGITHUB(const std::string &strJSON, const std::string &strURL, const std::string &strAddXMLFilename, const std::string &strChlogname)
{
  Json::Value root;   // will contains the root value after parsing.
  Json::Reader reader;
  std::string strName, strVersion;

  bool parsingSuccessful = reader.parse(strJSON, root );
  if ( !parsingSuccessful )
    CLog::Log(logERROR, "CJSONHandler::ParseAddonXMLVersionGITHUB: no valid JSON data downloaded from Github");

  const Json::Value JLangs = root;

  for(Json::ValueIterator itr = JLangs.begin() ; itr !=JLangs.end() ; itr++)
  {
    Json::Value JValu = *itr;
    std::string strType =JValu.get("type", "unknown").asString();

    if (strType == "unknown")
      CLog::Log(logERROR, "CJSONHandler::ParseAddonXMLVersionGITHUB: no valid JSON data downloaded from Github");

    strName =JValu.get("name", "unknown").asString();

    if (strName == "unknown")
      CLog::Log(logERROR, "CJSONHandler::ParseAddonXMLVersionGITHUB: no valid JSON data downloaded from Github");

    if (strType == "file" && ((!strAddXMLFilename.empty() && strName == strAddXMLFilename) ||
        (!strChlogname.empty() && strName == strChlogname)))
    {
      strVersion =JValu.get("sha", "unknown").asString();

      if (strVersion == "unknown")
        CLog::Log(logERROR, "CJSONHandler::ParseAddonXMLVersionGITHUB: no valid sha JSON data downloaded from Github");

      g_Fileversion.SetVersionForURL(strURL + strName, strVersion);
    }
  };
};

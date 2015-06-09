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

#include "ResourceHandler.h"
#include <list>
#include <algorithm>
#include "HTTPUtils.h"
#include "Langcodes.h"
#include "Fileversioning.h"
#include "jsoncpp/json/json.h"
#include "Log.h"
#include "CharsetUtils/CharsetUtils.h"
#include "FileUtils/FileUtils.h"

using namespace std;

CResourceHandler::CResourceHandler()
{};

CResourceHandler::CResourceHandler(const CXMLResdata& XMLResdata) : m_XMLResData(XMLResdata)
{};

CResourceHandler::~CResourceHandler()
{};

CPOHandler* CResourceHandler::GetPOData(std::string strLang)
{
  if (m_mapPOFiles.empty())
    return NULL;
  if (m_mapPOFiles.find(strLang) != m_mapPOFiles.end())
    return &m_mapPOFiles[strLang];
  return NULL;
}

// Download from Transifex related functions

bool CResourceHandler::FetchPOFilesTXToMem(const CXMLResdata &XMLResdata, std::string strURL)
{
  g_HTTPHandler.Cleanup();
  g_HTTPHandler.ReInit();
  CLog::Log(logINFO, "ResHandler: Starting to load resource from TX URL: %s into memory",strURL.c_str());
  printf(" Langlist");

  std::string strtemp = g_HTTPHandler.GetURLToSTR(strURL + "stats/");
  if (strtemp.empty())
    CLog::Log(logERROR, "ResHandler::FetchPOFilesTXToMem: error getting po file list from transifex.net");

  char cstrtemp[strtemp.size()];
  strcpy(cstrtemp, strtemp.c_str());

  std::list<std::string> listLangsTX = ParseAvailLanguagesTX(strtemp, strURL, m_XMLResData.strDefTXLFormat, m_XMLResData);

  CPOHandler POHandler(m_XMLResData);

  for (std::list<std::string>::iterator it = listLangsTX.begin(); it != listLangsTX.end(); it++)
  {
    printf (" %s", it->c_str());
    m_mapPOFiles[*it] = POHandler;
    CPOHandler * pPOHandler = &m_mapPOFiles[*it];
    pPOHandler->FetchPOURLToMem(strURL + "translation/" + g_LCodeHandler.GetLangFromLCode(*it, m_XMLResData.strDefTXLFormat) + "/?file", false);
    pPOHandler->SetIfIsSourceLang(*it == m_XMLResData.strSourceLcode);
    std::string strLang = *it;
    CLog::LogTable(logINFO, "txfetch", "\t\t\t%s\t\t%i", strLang.c_str(),
                            pPOHandler->GetClassEntriesCount());
  }
  CLog::LogTable(logADDTABLEHEADER, "txfetch", "--------------------------------------------------------------\n");
  CLog::LogTable(logADDTABLEHEADER, "txfetch", "FetchPOFilesTX:\tLang\t\tIDEntry\t\tClassEntry\n");
  CLog::LogTable(logADDTABLEHEADER, "txfetch", "--------------------------------------------------------------\n");
  CLog::LogTable(logCLOSETABLE, "txfetch", "");
  return true;
}

bool CResourceHandler::FetchPOFilesUpstreamToMem(const CXMLResdata &XMLResdata)
{
  g_HTTPHandler.Cleanup();
  g_HTTPHandler.ReInit();
  CLog::Log(logINFO, "ResHandler: Starting to load resource from Upsream URL: %s into memory",XMLResdata.strUPSAddonURL.c_str());

  m_bIsLangAddon = !XMLResdata.strUPSAddonLangFormat.empty();

  std::string strLangdirPrefix, strGitHubURL, strtemp;

  if (!XMLResdata.strUPSAddonURL.empty() && XMLResdata.strUPSAddonLangFormat.empty()) // kodi language-addons have individual addon.xml files
  {
    // We get the version of the addon.xml and changelog.txt files here
    strGitHubURL = g_HTTPHandler.GetGitHUBAPIURL(XMLResdata.strUPSAddonURLRoot);
    printf(" Dir");
    strtemp = g_HTTPHandler.GetURLToSTR(strGitHubURL);
    if (strtemp.empty())
      CLog::Log(logERROR, "ResHandler::FetchPOFilesUpstreamToMem: error getting addon.xml file version from github.com");

//TODO separate addon.xml and changelog.txt version parsing as they can be in a different place
    ParseAddonXMLVersionGITHUB(strtemp, XMLResdata.strUPSAddonURLRoot, XMLResdata.strUPSAddonXMLFilename, XMLResdata.strUPSChangelogName);

    printf(" Addxml");
    m_AddonXMLHandler.FetchAddonXMLFileUpstr(XMLResdata);
    if (!XMLResdata.strUPSChangelogURL.empty())
    {
      printf(" Chlog");
      m_AddonXMLHandler.FetchAddonChangelogFile(XMLResdata.strUPSChangelogURL);
    }

    if (XMLResdata.strUPSLangURL.empty())
      return true;
  }

  std::list<std::string> listLangs, listGithubLangs, listGithubSRCLangs;

  printf(" Langlist");
  strtemp.clear();
  strGitHubURL.clear();
  strGitHubURL = g_HTTPHandler.GetGitHUBAPIURL(XMLResdata.strUPSLangURLRoot);

  strtemp = g_HTTPHandler.GetURLToSTR(strGitHubURL);
  if (strtemp.empty())
    CLog::Log(logERROR, "ResHandler::FetchPOFilesUpstreamToMem: error getting po file list from github.com");

  listGithubLangs = ParseAvailLanguagesGITHUB(strtemp, XMLResdata.strUPSLangURL, XMLResdata.strUPSLangFormat,
                                                     XMLResdata.strUPSAddonURL, XMLResdata.bIsLanguageAddon);

  listLangs = listGithubLangs;

  if (!XMLResdata.strUPSSourceLangURL.empty()) // we have a language-addon with different SRC language upstream URL
  {
    printf(" LanglistSRC");
    strtemp.clear();
    strGitHubURL.clear();
//TODO Rather use the language directory itself for language addons to have a chance to get the changes of the individual addon.xml files

    strGitHubURL = g_HTTPHandler.GetGitHUBAPIURL(g_CharsetUtils.GetRoot(XMLResdata.strUPSSourceLangURL, XMLResdata.strUPSAddonLangFormat));
    strtemp = g_HTTPHandler.GetURLToSTR(strGitHubURL);
    if (strtemp.empty())
      CLog::Log(logERROR, "ResHandler::FetchPOFilesUpstreamToMem: error getting source language file list from github.com");

    listGithubLangs = ParseAvailLanguagesGITHUB(strtemp, XMLResdata.strUPSSourceLangURL, XMLResdata.strUPSLangFormat,
                                                       XMLResdata.strUPSSourceLangAddonURL, XMLResdata.bIsLanguageAddon);
    if (listGithubLangs.size() == 1 && listGithubLangs.front() == XMLResdata.strSourceLcode)
      listLangs.push_back(XMLResdata.strSourceLcode);
    else
      CLog::Log(logERROR, "ResHandler::FetchPOFilesUpstreamToMem: found non source language at source language repository for addon: %s", XMLResdata.strResName.c_str());
  }
  listLangs.sort();

  bool bResult;

  for (std::list<std::string>::iterator it = listLangs.begin(); it != listLangs.end(); it++)
  {
    CPOHandler POHandler(m_XMLResData);
    bool bIsSourceLang = *it == XMLResdata.strSourceLcode;
    POHandler.SetIfIsSourceLang(bIsSourceLang);
    printf (" %s", it->c_str());

    if (XMLResdata.bIsLanguageAddon)
    {
      std::string strLangAddonXMLDloadURL;
      if (!bIsSourceLang)
        strLangAddonXMLDloadURL = g_CharsetUtils.ReplaceLanginURL (XMLResdata.strUPSAddonURL, XMLResdata.strUPSAddonLangFormat, *it);
      else
        strLangAddonXMLDloadURL = g_CharsetUtils.ReplaceLanginURL (XMLResdata.strUPSSourceLangAddonURL, XMLResdata.strUPSAddonLangFormat, *it);

      POHandler.FetchLangAddonXML(strLangAddonXMLDloadURL);
    }

    std::string strDloadURL;
    if (bIsSourceLang && !XMLResdata.strUPSSourceLangURL.empty()) // If we have a different URL for source language, use that for download
      strDloadURL = g_CharsetUtils.ReplaceLanginURL(XMLResdata.strUPSSourceLangURL, XMLResdata.strUPSLangFormat, *it);
    else
      strDloadURL = g_CharsetUtils.ReplaceLanginURL(XMLResdata.strUPSLangURL, XMLResdata.strUPSLangFormat, *it);

//    if (XMLResdata.strUPSLangFileName == "strings.xml")
//      bResult = POHandler.FetchXMLURLToMem(strDloadURL);
//    else
      bResult = POHandler.FetchPOURLToMem(strDloadURL,false);
    if (bResult)
    {
      m_mapPOFiles[*it] = POHandler;
      std::string strLang = *it;
      CLog::LogTable(logINFO, "upstrFetch", "\t\t\t%s\t\t%i\t\t%i", strLang.c_str(),
              POHandler.GetClassEntriesCount(), POHandler.GetCommntEntriesCount());
    }
  }
  CLog::LogTable(logADDTABLEHEADER, "upstrFetch", "-----------------------------------------------------------------------------\n");
  CLog::LogTable(logADDTABLEHEADER, "upstrFetch", "FetchPOFilesUpstr:\tLang\t\tIDEntry\t\tClassEntry\tCommEntry\n");
  CLog::LogTable(logADDTABLEHEADER, "upstrFetch", "-----------------------------------------------------------------------------\n");
  CLog::LogTable(logCLOSETABLE, "upstrFetch", "");
  return true;
}

bool CResourceHandler::WritePOToFiles(std::string strProjRootDir, std::string strPrefixDir, std::string strResname, CXMLResdata XMLResdata, bool bTXUpdFile)
{
  XMLResdata = m_XMLResData;
  std::string strPath, strLangFormat, strAddonXMLPath;
  if (!bTXUpdFile)
  {
    strPath = strProjRootDir + strPrefixDir + DirSepChar + XMLResdata.strLOCLangPath;
    strLangFormat = XMLResdata.strLOCLangFormat;
    if (XMLResdata.bIsLanguageAddon)
      strAddonXMLPath = strProjRootDir + strPrefixDir + DirSepChar + XMLResdata.strLOCAddonPath;
  }
  else
  {
    strPath = strProjRootDir + strPrefixDir + DirSepChar + XMLResdata.strResName + DirSepChar + XMLResdata.strBaseLCode + DirSepChar + "strings.po";
    strLangFormat = XMLResdata.strBaseLCode;
  }

  if (bTXUpdFile && !m_mapPOFiles.empty())
    printf("Languages to update from upstream to upload to Transifex:");
  size_t counter = 0;

  printf ("%s", KCYN);

  for (T_itmapPOFiles itmapPOFiles = m_mapPOFiles.begin(); itmapPOFiles != m_mapPOFiles.end(); itmapPOFiles++)
  {
    std::string strPODir, strAddonDir;
    strPODir = g_CharsetUtils.ReplaceLanginURL(strPath, strLangFormat, itmapPOFiles->first);

    if (bTXUpdFile && counter < 15)
      printf (" %s", itmapPOFiles->first.c_str());
    if ((bTXUpdFile && counter == 14) && m_mapPOFiles.size() != 15)
      printf ("+%i Langs", (int)m_mapPOFiles.size()-14);

    CPOHandler * pPOHandler = &m_mapPOFiles[itmapPOFiles->first];
    if (g_CharsetUtils.bISPOFile(XMLResdata.strLOCLangFileName) || bTXUpdFile)
      pPOHandler->WritePOFile(strPODir);
//    else if (g_CharsetUtils.bISXMLFile(XMLResdata.strLOCLangFileName))
//      pPOHandler->WriteXMLFile(strPODir);
    else
      CLog::Log(logERROR, "ResHandler::WritePOToFiles: unknown local fileformat: %s", XMLResdata.strLOCLangFileName.c_str());

    // Write individual addon.xml files for language-addons
    if (!strAddonXMLPath.empty())
    {
      strAddonDir = g_CharsetUtils.ReplaceLanginURL(strAddonXMLPath, strLangFormat, itmapPOFiles->first);
      pPOHandler->WriteLangAddonXML(strAddonDir);
    }

    CLog::LogTable(logINFO, "writepo", "\t\t\t%s\t\t%i", itmapPOFiles->first.c_str(),
              pPOHandler->GetClassEntriesCount());
  }
  printf ("%s", RESET);
  if (bTXUpdFile && !m_mapPOFiles.empty())
    printf("\n");

  CLog::LogTable(logADDTABLEHEADER, "writepo", "--------------------------------------------------------------\n");
  CLog::LogTable(logADDTABLEHEADER, "writepo", "WritePOFiles:\tLang\t\tIDEntry\t\tClassEntry\n");
  CLog::LogTable(logADDTABLEHEADER, "writepo", "--------------------------------------------------------------\n");
  CLog::LogTable(logCLOSETABLE, "writepo", "");

  // update local addon.xml file
  if (!XMLResdata.bIsLanguageAddon && strPrefixDir == XMLResdata.strMergedLangfileDir)
  {
    bool bResChangedFromUpstream = !m_lChangedLangsFromUpstream.empty() || !m_lChangedLangsInAddXMLFromUpstream.empty();
    m_AddonXMLHandler.UpdateAddonXMLFile(strProjRootDir + strPrefixDir + DirSepChar + XMLResdata.strLOCAddonPath, bResChangedFromUpstream, XMLResdata);
    if (!XMLResdata.strChangelogFormat.empty())
      m_AddonXMLHandler.UpdateAddonChangelogFile(strProjRootDir + strPrefixDir + DirSepChar + XMLResdata.strLOCChangelogPath, XMLResdata.strChangelogFormat, bResChangedFromUpstream);
  }

  return true;
}

T_itmapPOFiles CResourceHandler::IterateToMapIndex(T_itmapPOFiles it, size_t index)
{
  for (size_t i = 0; i != index; i++) it++;
  return it;
}

std::list<std::string> CResourceHandler::ParseAvailLanguagesTX(std::string strJSON, const std::string &strURL,
                                                           const std::string &strTXLangformat, const CXMLResdata& XMLResData)
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

    LCode = g_LCodeHandler.VerifyLangCode(LCode, strTXLangformat);

    if (LCode == "")
      continue;

    Json::Value valu = *itr;
    std::string strCompletedPerc = valu.get("completed", "unknown").asString();
    std::string strModTime = valu.get("last_update", "unknown").asString();

    // we only add language codes to the list which has a minimum ready percentage defined in the xml file
    // we make an exception with all English derived languages, as they can have only a few srings changed
    if (LCode.find("en_") != std::string::npos || strtol(&strCompletedPerc[0], NULL, 10) > XMLResData.iMinComplPercent-1)
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

std::list<std::string> CResourceHandler::ParseAvailLanguagesGITHUB(std::string strJSON, std::string strURL, std::string strLangformat,
                                                                   std::string strAddonXMLURL, bool bIsLangAddon)
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
      continue;

    lang =JValu.get("name", "unknown").asString();
    if (lang == "unknown")
      CLog::Log(logERROR, "CJSONHandler::ParseAvailLanguagesGITHUB: no valid JSON data downloaded from Github");

    strVersion =JValu.get("sha", "unknown").asString();
    if (strVersion == "unknown")
      CLog::Log(logERROR, "CJSONHandler::ParseAvailLanguagesGITHUB: no valid sha JSON data downloaded from Github");

    std::string strMatchedLangalias = g_CharsetUtils.GetLangnameFromURL(lang, strURL, strLangformat);
    std::string strFoundLangCode = g_LCodeHandler.GetLangCodeFromAlias(strMatchedLangalias, strLangformat);
    if (strFoundLangCode != "")
    {
      listLangs.push_back(strFoundLangCode);
      std::string strURLforFile = strURL;
      g_CharsetUtils.replaceAllStrParts(&strURLforFile, strLangformat, strMatchedLangalias);
      g_Fileversion.SetVersionForURL(strURLforFile, strVersion);
      if (bIsLangAddon)
      {
        std::string strURLforAddonFile = strAddonXMLURL;
        g_CharsetUtils.replaceAllStrParts(&strURLforAddonFile, strLangformat, strMatchedLangalias);
        g_Fileversion.SetVersionForURL(strURLforAddonFile, strVersion);
      }
    }
  };

  return listLangs;
};

void CResourceHandler::ParseAddonXMLVersionGITHUB(const std::string &strJSON, const std::string &strURL, const std::string &strAddXMLFilename, const std::string &strChlogname)
{
  Json::Value root;   // will contains the root value after parsing.
  Json::Reader reader;
  std::string strName, strVersion;

  bool parsingSuccessful = reader.parse(strJSON, root );
  if ( !parsingSuccessful )
    CLog::Log(logERROR, "CJSONHandler::ParseAddonXMLVersionGITHUB: no valid JSON data downloaded from Github");

  const Json::Value JFiles = root;

  for(Json::ValueIterator itr = JFiles.begin() ; itr !=JFiles.end() ; itr++)
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

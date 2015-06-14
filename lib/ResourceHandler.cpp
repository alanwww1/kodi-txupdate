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
{
  m_AddonXMLHandler.SetXMLReasData(XMLResdata);
};

CResourceHandler::~CResourceHandler()
{};

/*
CPOHandler* CResourceHandler::GetPOData(std::string strLang)
{
  if (m_mapPOFiles.empty())
    return NULL;
  if (m_mapPOFiles.find(strLang) != m_mapPOFiles.end())
    return &m_mapPOFiles[strLang];
  return NULL;
}
*/

// Download from Transifex related functions

bool CResourceHandler::FetchPOFilesTXToMem()
{

  std::string strURL = "https://www.transifex.com/api/2/project/" + m_XMLResData.strProjectName + "/resource/" + m_XMLResData.strTXName + "/";
  g_HTTPHandler.Cleanup();
  g_HTTPHandler.ReInit();
  CLog::Log(logINFO, "ResHandler: Starting to load resource from TX URL: %s into memory",strURL.c_str());
  printf(" Langlist");

  std::string strtemp = g_HTTPHandler.GetURLToSTR(strURL + "stats/");
  if (strtemp.empty())
    CLog::Log(logERROR, "ResHandler::FetchPOFilesTXToMem: error getting po file list from transifex.net");

  std::list<std::string> listLCodesTX = ParseAvailLanguagesTX(strtemp, strURL);

  CPOHandler newPOHandler(m_XMLResData);

  for (std::list<std::string>::iterator it = listLCodesTX.begin(); it != listLCodesTX.end(); it++)
  {
    const std::string& sLCode = *it;
    printf (" %s", sLCode.c_str());
    m_mapTRX[sLCode] = newPOHandler;

    CPOHandler& POHandler = m_mapTRX[sLCode];

    std::string sLangNameTX = g_LCodeHandler.GetLangFromLCode(*it, m_XMLResData.strDefTXLFormat);
    POHandler.FetchPOURLToMem(strURL + "translation/" + sLangNameTX + "/?file");
    POHandler.SetIfIsSourceLang(sLCode == m_XMLResData.strSourceLcode);

    CLog::LogTable(logINFO, "txfetch", "\t\t\t%s\t\t%i", sLCode.c_str(), POHandler.GetClassEntriesCount());
  }
  CLog::LogTable(logADDTABLEHEADER, "txfetch", "--------------------------------------------------------------\n");
  CLog::LogTable(logADDTABLEHEADER, "txfetch", "FetchPOFilesTX:\tLang\t\tIDEntry\t\tClassEntry\n");
  CLog::LogTable(logADDTABLEHEADER, "txfetch", "--------------------------------------------------------------\n");
  CLog::LogTable(logCLOSETABLE, "txfetch", "");
  return true;
}

bool CResourceHandler::FetchPOFilesUpstreamToMem()
{
  g_HTTPHandler.Cleanup();
  g_HTTPHandler.ReInit();

  m_AddonXMLHandler.FetchAddonDataFiles();

  if (m_XMLResData.strUPSLangURL.empty())
    return true;

  std::list<std::string> listLangs;
  printf(" Langlist");
  listLangs = GetAvailLangsGITHUB();

  if (!m_XMLResData.strUPSSourceLangURL.empty()) // we have a language-addon with different SRC language upstream URL
    listLangs.push_back(m_XMLResData.strSourceLcode);

  listLangs.sort();

  for (std::list<std::string>::iterator it = listLangs.begin(); it != listLangs.end(); it++)
  {
    CPOHandler POHandler(m_XMLResData);
    const std::string& sLCode = *it;

    bool bIsSourceLang = sLCode == m_XMLResData.strSourceLcode;
    POHandler.SetIfIsSourceLang(bIsSourceLang);
    printf (" %s", sLCode.c_str());

    if (m_XMLResData.bIsLanguageAddon)
    {
      std::string strLangAddonXMLDloadURL;
      if (!bIsSourceLang)
        strLangAddonXMLDloadURL = g_CharsetUtils.ReplaceLanginURL (m_XMLResData.strUPSAddonURL, m_XMLResData.strUPSAddonLangFormat, sLCode);
      else
        strLangAddonXMLDloadURL = g_CharsetUtils.ReplaceLanginURL (m_XMLResData.strUPSSourceLangAddonURL, m_XMLResData.strUPSAddonLangFormat, sLCode);

      POHandler.FetchLangAddonXML(strLangAddonXMLDloadURL);
    }

    std::string strDloadURL;
    if (bIsSourceLang && !m_XMLResData.strUPSSourceLangURL.empty()) // If we have a different URL for source language, use that for download
      strDloadURL = g_CharsetUtils.ReplaceLanginURL(m_XMLResData.strUPSSourceLangURL, m_XMLResData.strUPSLangFormat, sLCode);
    else
      strDloadURL = g_CharsetUtils.ReplaceLanginURL(m_XMLResData.strUPSLangURL, m_XMLResData.strUPSLangFormat, sLCode);

    POHandler.FetchPOURLToMem(strDloadURL);
    if (m_AddonXMLHandler.FindAddonXMLEntry(sLCode))
      POHandler.AddAddonXMLEntries(m_AddonXMLHandler.GetAddonXMLEntry(sLCode), m_AddonXMLHandler.GetAddonXMLEntry(m_XMLResData.strSourceLcode));

    m_mapUPS[sLCode] = POHandler;
  }

  CLog::LogTable(logADDTABLEHEADER, "upstrFetch", "-----------------------------------------------------------------------------\n");
  CLog::LogTable(logADDTABLEHEADER, "upstrFetch", "FetchPOFilesUpstr:\tLang\t\tIDEntry\t\tClassEntry\tCommEntry\n");
  CLog::LogTable(logADDTABLEHEADER, "upstrFetch", "-----------------------------------------------------------------------------\n");
  CLog::LogTable(logCLOSETABLE, "upstrFetch", "");
  return true;
}

void CResourceHandler::MergeResource()
{
  std::list<std::string> listMergedLangs = CreateMergedLangList();
  const CPOHandler& POHandlSRC = m_mapUPS.at(m_XMLResData.strSourceLcode);
  for (std::list<std::string>::iterator itlang = listMergedLangs.begin(); itlang != listMergedLangs.end(); itlang++)
  {
    const std::string& sLCode = *itlang;
    if (sLCode == m_XMLResData.strSourceLcode)
    {
      m_mapMRG[sLCode] = POHandlSRC;
      //TODO check if SRC file differs for TRX and UPS files
    }
  }
}

bool CResourceHandler::WritePOToFiles(std::string strProjRootDir, std::string strPrefixDir, std::string strResname, CXMLResdata XMLResdata, bool bTXUpdFile)
{
  /*
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
*/
  return true;
}

T_itmapPOFiles CResourceHandler::IterateToMapIndex(T_itmapPOFiles it, size_t index)
{
  for (size_t i = 0; i != index; i++) it++;
  return it;
}

std::list<std::string> CResourceHandler::ParseAvailLanguagesTX(std::string strJSON, const std::string &strURL)
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

    LCode = g_LCodeHandler.VerifyLangCode(LCode, m_XMLResData.strDefTXLFormat);

    if (LCode == "")
      continue;

    Json::Value valu = *itr;
    std::string strCompletedPerc = valu.get("completed", "unknown").asString();
    std::string strModTime = valu.get("last_update", "unknown").asString();

    // we only add language codes to the list which has a minimum ready percentage defined in the xml file
    // we make an exception with all English derived languages, as they can have only a few srings changed
    if (LCode.find("en_") != std::string::npos || strtol(&strCompletedPerc[0], NULL, 10) > m_XMLResData.iMinComplPercent-1)
    {
      strLangsToFetch += LCode + ": " + strCompletedPerc + ", ";
      listLangs.push_back(LCode);
      g_Fileversion.SetVersionForURL(strURL + "translation/" + g_LCodeHandler.GetLangFromLCode(LCode, m_XMLResData.strDefTXLFormat) + "/?file", strModTime);
    }
    else
      strLangsToDrop += LCode + ": " + strCompletedPerc + ", ";
  };
  CLog::Log(logINFO, "JSONHandler: ParseAvailLangs: Languages to be Fetcehed: %s", strLangsToFetch.c_str());
  CLog::Log(logINFO, "JSONHandler: ParseAvailLangs: Languages to be Dropped (not enough completion): %s", strLangsToDrop.c_str());
  return listLangs;
};

std::list<std::string> CResourceHandler::GetAvailLangsGITHUB()
{
  std::string sJson, sGitHubURL;
  sGitHubURL = g_HTTPHandler.GetGitHUBAPIURL(m_XMLResData.strUPSLangURLRoot);

  sJson = g_HTTPHandler.GetURLToSTR(sGitHubURL);
  if (sJson.empty())
    CLog::Log(logERROR, "ResHandler::FetchPOFilesUpstreamToMem: error getting po file list from github.com");

  Json::Value root;   // will contains the root value after parsing.
  Json::Reader reader;
  std::string lang, strVersion;
  std::list<std::string> listLangs;

  bool parsingSuccessful = reader.parse(sJson, root );
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

    std::string strMatchedLangalias = g_CharsetUtils.GetLangnameFromURL(lang, m_XMLResData.strUPSLangURL, m_XMLResData.strUPSLangFormat);
    std::string strFoundLangCode = g_LCodeHandler.GetLangCodeFromAlias(strMatchedLangalias, m_XMLResData.strUPSLangFormat);
    if (strFoundLangCode != "")
    {
      listLangs.push_back(strFoundLangCode);
      std::string strURLforFile = m_XMLResData.strUPSLangURL;
      g_CharsetUtils.replaceAllStrParts(&strURLforFile, m_XMLResData.strUPSLangFormat, strMatchedLangalias);
      g_Fileversion.SetVersionForURL(strURLforFile, strVersion);
      if (m_XMLResData.bIsLanguageAddon)
      {
        std::string strURLforAddonFile = m_XMLResData.strUPSAddonURL;
        g_CharsetUtils.replaceAllStrParts(&strURLforAddonFile, m_XMLResData.strUPSLangFormat, strMatchedLangalias);
        g_Fileversion.SetVersionForURL(strURLforAddonFile, strVersion);
      }
    }
  };

  return listLangs;
};

std::list<std::string> CResourceHandler::CreateMergedLangList()
{
  std::list<std::string> listMergedLangs;

  for (T_itmapPOFiles it = m_mapUPS.begin(); it != m_mapUPS.end(); it++)
  {
    const std::string& sLCode = it->first;

    if (std::find(listMergedLangs.begin(), listMergedLangs.end(), sLCode) == listMergedLangs.end())
      listMergedLangs.push_back(sLCode);
  }

  for (T_itmapPOFiles it = m_mapTRX.begin(); it != m_mapTRX.end(); it++)
  {
    const std::string& sLCode = it->first;

    if (std::find(listMergedLangs.begin(), listMergedLangs.end(), sLCode) == listMergedLangs.end())
      listMergedLangs.push_back(sLCode);
  }

  return listMergedLangs;
}



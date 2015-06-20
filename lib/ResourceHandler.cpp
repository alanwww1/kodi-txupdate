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
    POHandler.SetIfIsSourceLang(sLCode == m_XMLResData.strSourceLcode);
    POHandler.SetLCode(sLCode);


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
  bool bHasLanguageFiles = !m_XMLResData.strUPSLangURL.empty();

  m_AddonXMLHandler.FetchAddonDataFiles();

  std::set<std::string> listLangs;
  if (bHasLanguageFiles)
  {
    printf(" Langlist");
    listLangs = GetAvailLangsGITHUB();
  }

  m_AddonXMLHandler.AddAddonXMLLangsToList(listLangs); // Add languages that are only in the addon.xml file

  if (!m_XMLResData.strUPSSourceLangURL.empty()) // we have a language-addon with different SRC language upstream URL
    listLangs.insert(m_XMLResData.strSourceLcode);

  for (std::set<std::string>::iterator it = listLangs.begin(); it != listLangs.end(); it++)
  {
    CPOHandler POHandler(m_XMLResData);
    const std::string& sLCode = *it;

    bool bIsSourceLang = sLCode == m_XMLResData.strSourceLcode;
    POHandler.SetIfIsSourceLang(bIsSourceLang);
    POHandler.SetLCode(sLCode);
    printf (" %s", sLCode.c_str());

    if (bHasLanguageFiles && m_XMLResData.bIsLanguageAddon) // Download individual addon.xml files for language-addons
    {
      std::string strLangAddonXMLDloadURL;
      if (!bIsSourceLang)
        strLangAddonXMLDloadURL = g_CharsetUtils.ReplaceLanginURL (m_XMLResData.strUPSAddonURL, m_XMLResData.strUPSAddonLangFormat, sLCode);
      else
        strLangAddonXMLDloadURL = g_CharsetUtils.ReplaceLanginURL (m_XMLResData.strUPSSourceLangAddonURL, m_XMLResData.strUPSAddonLangFormat, sLCode);

      POHandler.FetchLangAddonXML(strLangAddonXMLDloadURL);
    }

    if (bHasLanguageFiles) // Download language file from upstream for language sLCode
    {
      std::string strDloadURL;
      if (bIsSourceLang && !m_XMLResData.strUPSSourceLangURL.empty()) // If we have a different URL for source language, use that for download
        strDloadURL = g_CharsetUtils.ReplaceLanginURL(m_XMLResData.strUPSSourceLangURL, m_XMLResData.strUPSLangFormat, sLCode);
      else
        strDloadURL = g_CharsetUtils.ReplaceLanginURL(m_XMLResData.strUPSLangURL, m_XMLResData.strUPSLangFormat, sLCode);

      POHandler.FetchPOURLToMem(strDloadURL);
    }

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

bool CResourceHandler::ComparePOFiles(CPOHandler& POHandler1, CPOHandler& POHandler2)
{
  if (POHandler1.GetClassEntriesCount() != POHandler2.GetClassEntriesCount())
    return false;

  T_itPOData itPO1 = POHandler1.GetPOMapBeginIterator();
  T_itPOData itPO2 = POHandler2.GetPOMapBeginIterator();
  T_itPOData itPOEnd1 = POHandler1.GetPOMapEndIterator();

  while (itPO1 != itPOEnd1)
  {
    if (!(itPO1->second == itPO2->second))
      return false;
    itPO2++;
    itPO1++;
  }
  return true;
}

void CResourceHandler::MergeResource()
{
  std::list<std::string> listMergedLangs = CreateMergedLangList();
  CPOHandler& POHandlUPSSRC = m_mapUPS.at(m_XMLResData.strSourceLcode);
  for (std::list<std::string>::iterator itlang = listMergedLangs.begin(); itlang != listMergedLangs.end(); itlang++)
  {
    const std::string& sLCode = *itlang;

    // check if lcode is the source lcode. If so we don't iterate through the entries.
    // We just check if it has changed from the last uploaded one
    bool bWriteUPDFileSRC = false;
    if (sLCode == m_XMLResData.strSourceLcode)
    {
      if (m_mapTRX.find(m_XMLResData.strSourceLcode) != m_mapTRX.end())
      {
        CPOHandler& POHandlTRXSRC = m_mapTRX.at(m_XMLResData.strSourceLcode);
        if (!ComparePOFiles(POHandlTRXSRC, POHandlUPSSRC))       // if the source po file differs from the one at transifex we need to update it
          bWriteUPDFileSRC = true;
      }
      else
        bWriteUPDFileSRC = true;
    }

    T_itPOData itSRCUPSPO = POHandlUPSSRC.GetPOMapBeginIterator();
    T_itPOData itSRCUPSPOEnd = POHandlUPSSRC.GetPOMapEndIterator();

    // Let's iterate by the UPSTREAM source PO file for this resource
    for (; itSRCUPSPO != itSRCUPSPOEnd; itSRCUPSPO++)
    {
      CPOEntry& EntrySRC = itSRCUPSPO->second;

      bool bisInUPS = FindUPSEntry(sLCode, EntrySRC);
      bool bisInTRX = FindTRXEntry(sLCode, EntrySRC);

      if (sLCode == m_XMLResData.strSourceLcode)
      {
        T_itPOData itPOUPS = GetUPSItFoundEntry();
        m_mapMRG[sLCode].AddItEntry(itPOUPS);
        if (bWriteUPDFileSRC)
          m_mapUPD[sLCode].AddItEntry(itPOUPS);
      }
      //We have non-source language. Let's treat it that way
      else if (bisInTRX)
      {
        T_itPOData itPOTRX = GetTRXItFoundEntry();
        m_mapMRG[sLCode].AddItEntry(itPOTRX);
      }
      else if (bisInUPS)
      {
        T_itPOData itPOUPS = GetUPSItFoundEntry();
        m_mapMRG[sLCode].AddItEntry(itPOUPS);
        m_mapUPD[sLCode].AddItEntry(itPOUPS);
      }
    }

    //Pass Resource data to the newly created PO classes for later use (PO file creation)
    CPOHandler& MRGPOHandler = m_mapMRG[sLCode];
    MRGPOHandler.SetXMLReasData(m_XMLResData);
    MRGPOHandler.SetIfIsSourceLang(sLCode == m_XMLResData.strSourceLcode);
    MRGPOHandler.SetPOType(MERGEDPO);
    MRGPOHandler.CreateHeader(m_AddonXMLHandler.GetResHeaderPretext(), sLCode);
    if (m_XMLResData.bIsLanguageAddon)
      MRGPOHandler.SetLangAddonXMLString(m_mapUPS[sLCode].GetLangAddonXMLString());

    if (m_mapUPD.find(sLCode) != m_mapUPD.end())
    {
      CPOHandler& UPDPOHandler = m_mapMRG[sLCode];
      UPDPOHandler.SetXMLReasData(m_XMLResData);
      UPDPOHandler.SetIfIsSourceLang(sLCode == m_XMLResData.strSourceLcode);
      UPDPOHandler.SetPOType(UPDATEPO);
      UPDPOHandler.CreateHeader(m_AddonXMLHandler.GetResHeaderPretext(), sLCode);
    }
  }
  return;
}

// Check if there is a POHandler existing in mapUPS for language code sLCode.
// If so, store it as last found POHandler iterator and store last sLCode.
// If a POHandler exist, try to find a PO entry in it.
bool CResourceHandler::FindUPSEntry(const std::string sLCode, CPOEntry &EntryToFind)
{
  if (m_lastUPSLCode == sLCode)
  {
    if (!m_bLastUPSHandlerFound)
      return false;
    else
      return m_lastUPSIterator->second.FindEntry(EntryToFind);
  }

  m_lastUPSLCode = sLCode;
  m_lastUPSIterator = m_mapUPS.find(sLCode);
  if (m_lastUPSIterator == m_mapUPS.end())
  {
    m_bLastUPSHandlerFound = false;
    return false;
  }
  else
  {
    m_bLastUPSHandlerFound = true;
    return m_lastUPSIterator->second.FindEntry(EntryToFind);
  }
}

// Check if there is a POHandler existing in mapUPS for language code sLCode.
// If so, store it as last found POHandler iterator and store last sLCode.
// If a POHandler exist, try to find a PO entry in it.
bool CResourceHandler::FindTRXEntry(const std::string sLCode, CPOEntry &EntryToFind)
{
  if (m_lastTRXLCode == sLCode)
  {
    if (!m_bLastTRXHandlerFound)
      return false;
    else
      return m_lastTRXIterator->second.FindEntry(EntryToFind);
  }

  m_lastTRXLCode = sLCode;
  m_lastTRXIterator = m_mapTRX.find(sLCode);
  if (m_lastTRXIterator == m_mapTRX.end())
  {
    m_bLastTRXHandlerFound = false;
    return false;
  }
  else
  {
    m_bLastTRXHandlerFound = true;
    return m_lastTRXIterator->second.FindEntry(EntryToFind);
  }
}

// Read iterator to the last found entry stored by function FindUPSEntry.
T_itPOData CResourceHandler::GetUPSItFoundEntry()
{
  return m_lastUPSIterator->second.GetItFoundEntry();
}

// Read iterator to the last found entry stored by function FindTRXEntry.
T_itPOData CResourceHandler::GetTRXItFoundEntry()
{
  return m_lastTRXIterator->second.GetItFoundEntry();
}

void CResourceHandler::WriteMergedPOFiles(const std::string& sAddonXMLPath, const std::string& sLangAddonXMLPath, const std::string& sChangeLogPath, const std::string& sLangPath )
{
  if (!m_XMLResData.bIsLanguageAddon)
  {
    m_AddonXMLHandler.WriteAddonXMLFile(sAddonXMLPath);
    if (!m_XMLResData.strChangelogFormat.empty())
      m_AddonXMLHandler.WriteAddonChangelogFile(sChangeLogPath, m_XMLResData.strChangelogFormat, false);
  }

  if (m_XMLResData.bHasOnlyAddonXML)
    return;


//  if (bMRGOrUPD)
//  {

  //  else
//  {
//    strPath = m_XMLResData.strProjRootdir + m_XMLResData.strTXUpdateLangfilesDir + DirSepChar + m_XMLResData.strResName + DirSepChar + m_XMLResData.strBaseLCode + DirSepChar + "strings.po";
//    strLangFormat = m_XMLResData.strBaseLCode;
  for (T_itmapPOFiles itmapPOFiles = m_mapMRG.begin(); itmapPOFiles != m_mapMRG.end(); itmapPOFiles++)
  {
    const std::string& sLCode = itmapPOFiles->first;
    std::string strPODir, strAddonDir;
    strPODir = g_CharsetUtils.ReplaceLanginURL(sLangPath, m_XMLResData.strLOCLangFormat, sLCode);

//    if (!bMRGOrUPD && counter < 15)
//      printf (" %s", sLCode.c_str());
//    if ((!bMRGOrUPD && counter == 14) && m_mapUPD.size() != 15)
//      printf ("+%i Langs", (int)m_mapUPD.size()-14);

    CPOHandler& POHandler = m_mapMRG.at(sLCode);

    POHandler.WritePOFile(strPODir);

    // Write individual addon.xml files for language-addons
    if (m_XMLResData.bIsLanguageAddon)
    {
      strAddonDir = g_CharsetUtils.ReplaceLanginURL(sLangAddonXMLPath, m_XMLResData.strLOCLangFormat, sLCode);
      POHandler.WriteLangAddonXML(strAddonDir);
    }
  }
 return;
}

void CResourceHandler::WriteUpdatePOFiles(const std::string& strPath)
{
  for (T_itmapPOFiles itmapPOFiles = m_mapUPD.begin(); itmapPOFiles != m_mapUPD.end(); itmapPOFiles++)
  {
    const std::string& sLCode = itmapPOFiles->first;
    std::string strPODir;
    strPODir = g_CharsetUtils.ReplaceLanginURL(strPath, m_XMLResData.strBaseLCode, sLCode);

//    if (!bMRGOrUPD && counter < 15)
//      printf (" %s", sLCode.c_str());
//    if ((!bMRGOrUPD && counter == 14) && m_mapUPD.size() != 15)
//      printf ("+%i Langs", (int)m_mapUPD.size()-14);

    CPOHandler& POHandler = m_mapMRG.at(sLCode);

    POHandler.WritePOFile(strPODir);
  }
  return;
}


void CResourceHandler::GenerateMergedPOFiles()
{

  printf ("%s", RESET);
//  if (!bMRGOrUPD && !m_mapMRG.empty())
//    printf("\n");

   // generate local merged addon.xml file
  if (!m_XMLResData.bIsLanguageAddon)
    m_AddonXMLHandler.GenerateAddonXMLFile(false);

//  if (!bMRGOrUPD && !m_mapUPD.empty())
//    printf("Languages to update from upstream to upload to Transifex:");
  size_t counter = 0;

  printf ("%s", KCYN);

  for (T_itmapPOFiles itmapPOFiles = m_mapMRG.begin(); itmapPOFiles != m_mapMRG.end(); itmapPOFiles++)
  {
    const std::string& sLCode = itmapPOFiles->first;

    CPOHandler& POHandler = m_mapMRG.at(sLCode);

    POHandler.GeneratePOFile();
  }

  return;
}

void CResourceHandler::GenerateUpdatePOFiles()
{

  printf ("%s", RESET);
//  if (!bMRGOrUPD && !m_mapMRG.empty())
//    printf("\n");

  if (!m_mapUPD.empty())
    printf("Languages to update from upstream to upload to Transifex:");
  size_t counter = 0;

  printf ("%s", KCYN);

  for (T_itmapPOFiles itmapPOFiles = m_mapUPD.begin(); itmapPOFiles != m_mapUPD.end(); itmapPOFiles++)
  {
    const std::string& sLCode = itmapPOFiles->first;

    CPOHandler& POHandler = m_mapUPD.at(sLCode);

    POHandler.GeneratePOFile();
  }

  return;
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

std::set<std::string> CResourceHandler::GetAvailLangsGITHUB()
{
  std::string sJson, sGitHubURL;
  sGitHubURL = g_HTTPHandler.GetGitHUBAPIURL(m_XMLResData.strUPSLangURLRoot);

  sJson = g_HTTPHandler.GetURLToSTR(sGitHubURL);
  if (sJson.empty())
    CLog::Log(logERROR, "ResHandler::FetchPOFilesUpstreamToMem: error getting po file list from github.com");

  Json::Value root;   // will contains the root value after parsing.
  Json::Reader reader;
  std::string lang, strVersion;
  std::set<std::string> listLangs;

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
      listLangs.insert(strFoundLangCode);
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



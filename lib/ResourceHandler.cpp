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

// Download from Transifex related functions
bool CResourceHandler::FetchPOFilesTXToMem()
{
  g_HTTPHandler.SetLocation("TRX");
  g_HTTPHandler.SetResName(m_XMLResData.sResName);
  g_HTTPHandler.SetLCode("");
  g_HTTPHandler.SetProjectName(m_XMLResData.TRX.ProjectName);

  std::string strURL = "https://www.transifex.com/api/2/project/" + m_XMLResData.TRX.ProjectName + "/resource/" + m_XMLResData.TRX.ResName + "/";
  g_HTTPHandler.Cleanup();
  g_HTTPHandler.ReInit();
  CLog::Log(logINFO, "ResHandler: Starting to load resource from TX URL: %s into memory",strURL.c_str());
  printf(" Langlist");

  g_HTTPHandler.SetFileName("LanguageList.json");
  g_HTTPHandler.SetDataFile(true);

  std::string strtemp = g_HTTPHandler.GetURLToSTR(strURL + "stats/");
  if (strtemp.empty())
    CLog::Log(logERROR, "ResHandler::FetchPOFilesTXToMem: error getting po file list from transifex.net");

  std::list<std::string> listLCodesTX = ParseAvailLanguagesTX(strtemp, strURL);

  CPOHandler newPOHandler(m_XMLResData);

  g_HTTPHandler.SetFileName("strings.po");
  g_HTTPHandler.SetDataFile(false);

  for (std::list<std::string>::iterator it = listLCodesTX.begin(); it != listLCodesTX.end(); it++)
  {
    const std::string& sLCode = *it;
    printf (" %s", sLCode.c_str());
    m_mapTRX[sLCode] = newPOHandler;

    CPOHandler& POHandler = m_mapTRX[sLCode];
    POHandler.SetIfIsSourceLang(sLCode == m_XMLResData.sSRCLCode);
    POHandler.SetLCode(sLCode);
    g_HTTPHandler.SetLCode(sLCode);

    std::string sLangNameTX = g_LCodeHandler.GetLangFromLCode(*it, m_XMLResData.TRX.LForm);
    POHandler.FetchPOTXPathToMem(strURL + "translation/" + sLangNameTX + "/?file");
    POHandler.SetIfIsSourceLang(sLCode == m_XMLResData.sSRCLCode);

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

  g_HTTPHandler.SetLocation("UPS");
  g_HTTPHandler.SetResName(m_XMLResData.sResName);
  g_HTTPHandler.SetLCode("");
  g_HTTPHandler.SetProjectName("");

  bool bHasLanguageFiles = !m_XMLResData.bHasOnlyAddonXML;

  std::set<std::string> listLangs, listLangsWithStringsPO;
  g_HTTPHandler.SetFileName("LocalFileList.txt");
  g_HTTPHandler.SetDataFile(true);

  printf(" GitDir");
  listLangsWithStringsPO = GetAvailLangsGITHUB();

  if (m_XMLResData.bIsLangAddon)
  {
    g_HTTPHandler.SetFileName("LocalFileList-SRC.txt");
    GetSRCFilesGitData(); //Get version data for the SRC files reside at a different github repo
    listLangsWithStringsPO.insert(m_XMLResData.sSRCLCode);
  }

  m_AddonXMLHandler.FetchAddonDataFiles();

  listLangs = listLangsWithStringsPO;

  m_AddonXMLHandler.AddAddonXMLLangsToList(listLangs); // Add languages that are only in the addon.xml file

  g_HTTPHandler.SetDataFile(false);


  for (std::set<std::string>::iterator it = listLangs.begin(); it != listLangs.end(); it++)
  {
    CPOHandler POHandler(m_XMLResData);
    const std::string& sLCode = *it;

    bool bLangHasStringsPO = listLangsWithStringsPO.find(sLCode) != listLangsWithStringsPO.end();

    bool bIsSourceLang = sLCode == m_XMLResData.sSRCLCode;
    POHandler.SetIfIsSourceLang(bIsSourceLang);
    POHandler.SetLCode(sLCode);
    g_HTTPHandler.SetLCode(sLCode);
    bool bHasPreviousVersion = false;

    if (bLangHasStringsPO && bHasLanguageFiles)
      printf (" %s", sLCode.c_str());

    if (bLangHasStringsPO && bHasLanguageFiles && m_XMLResData.bIsLangAddon) // Download individual addon.xml files for language-addons
    {
      g_HTTPHandler.SetFileName("addon.xml");
      std::string sLangXMLPath;
      CGITData GitData;

      if (!bIsSourceLang)
        GitData = m_XMLResData.UPS;
      else
        GitData = m_XMLResData.UPSSRC;

      sLangXMLPath = g_CharsetUtils.ReplaceLanginURL (GitData.AXMLPath, g_CharsetUtils.GetLFormFromPath(GitData.AXMLPath), sLCode);
      POHandler.FetchLangAddonXML(sLangXMLPath, GitData);
    }

    if (bLangHasStringsPO && bHasLanguageFiles) // Download language file from upstream for language sLCode
    {
      g_HTTPHandler.SetFileName("strings.po");

      std::string sLPath;
      CGITData GitData;

      if (bIsSourceLang && m_XMLResData.bIsLangAddon) // If we have a different URL for source language, use that for download
        GitData = m_XMLResData.UPSSRC;
      else
        GitData = m_XMLResData.UPS;

      sLPath = g_CharsetUtils.ReplaceLanginURL (GitData.LPath, g_CharsetUtils.GetLFormFromPath(GitData.LPath), sLCode);

      POHandler.FetchPOGitPathToMem(sLPath, GitData);
      bHasPreviousVersion = POHandler.GetIfItHasPrevLangVersion();
    }

    if (m_AddonXMLHandler.FindAddonXMLEntry(sLCode))
      POHandler.AddAddonXMLEntries(m_AddonXMLHandler.GetAddonXMLEntry(sLCode), m_AddonXMLHandler.GetAddonXMLEntry(m_XMLResData.sSRCLCode));

    m_mapUPS[sLCode] = POHandler;

    if (bHasPreviousVersion)
    {
      CPOHandler POHandlerPrev(m_XMLResData);
      POHandlerPrev.SetIfIsSourceLang(bIsSourceLang);
      POHandlerPrev.SetLCode(sLCode);
      POHandlerPrev.FetchPrevPOURLToMem();
      m_mapPREV[sLCode] = POHandlerPrev;
    }
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
  CPOHandler& POHandlUPSSRC = m_mapUPS.at(m_XMLResData.sSRCLCode);
  bool bResChangedFromUPS = false;

  for (std::list<std::string>::iterator itlang = listMergedLangs.begin(); itlang != listMergedLangs.end(); itlang++)
  {
    const std::string& sLCode = *itlang;
    bool bPOChangedFromUPS = false;

    // check if lcode is the source lcode. If so we check if it has changed from the last uploaded one
    // if it has has changed set flag that we need to write a complete update PO file for the SRC language
    bool bWriteUPDFileSRC = false;
    if (sLCode == m_XMLResData.sSRCLCode)
    {
      if (m_mapTRX.find(m_XMLResData.sSRCLCode) != m_mapTRX.end())
      {
        CPOHandler& POHandlTRXSRC = m_mapTRX.at(m_XMLResData.sSRCLCode);
        if (!ComparePOFiles(POHandlTRXSRC, POHandlUPSSRC))       // if the source po file differs from the one at transifex we need to update it
          bWriteUPDFileSRC = true;
      }
      else
        bWriteUPDFileSRC = true;
    }

    T_itPOData itSRCUPSPO = POHandlUPSSRC.GetPOMapBeginIterator();
    T_itPOData itSRCUPSPOEnd = POHandlUPSSRC.GetPOMapEndIterator();

    //Let's iterate by the UPSTREAM source PO file for this resource
    for (; itSRCUPSPO != itSRCUPSPOEnd; itSRCUPSPO++)
    {
      CPOEntry& EntrySRC = itSRCUPSPO->second;

      bool bisInUPS = FindUPSEntry(sLCode, EntrySRC);
      bool bisInTRX = FindTRXEntry(sLCode, EntrySRC);

      //Handle the source language
      if (sLCode == m_XMLResData.sSRCLCode)
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

        if (!bisInUPS) // check if this is a new translation at transifex, if so make changed flag true
          bPOChangedFromUPS = true;
        else
        {
          T_itPOData itPOUPS = GetUPSItFoundEntry();
          bPOChangedFromUPS = bPOChangedFromUPS || (itPOTRX->second.msgStr != itPOUPS->second.msgStr) ||
                              (itPOTRX->second.msgStrPlural != itPOUPS->second.msgStrPlural);
        }
      }

      //Entry was not on Transifex, check if it has a new translation at upstream
      else if (bisInUPS)
      {
        bool bIsInPrevUPS = FindPrevUPSEntry(sLCode, EntrySRC);
        if (!bIsInPrevUPS)
        {
          T_itPOData itPOUPS = GetUPSItFoundEntry();
          m_mapMRG[sLCode].AddItEntry(itPOUPS);
          m_mapUPD[sLCode].AddItEntry(itPOUPS);
        }
        else
          m_lLangsWithDeletedEntry.insert(sLCode);
      }
    }

    //Pass Resource data to the newly created MRG PO classes for later use (PO file creation)
    if (m_mapMRG.find(sLCode) != m_mapMRG.end())
    {
      CPOHandler& MRGPOHandler = m_mapMRG.at(sLCode);
      MRGPOHandler.SetXMLReasData(m_XMLResData);
      MRGPOHandler.SetIfIsSourceLang(sLCode == m_XMLResData.sSRCLCode);
      MRGPOHandler.SetPOType(MERGEDPO);
      MRGPOHandler.CreateHeader(m_AddonXMLHandler.GetResHeaderPretext(), sLCode);
      if (m_XMLResData.bIsLangAddon)
        MRGPOHandler.SetLangAddonXMLString(m_mapUPS[sLCode].GetLangAddonXMLString());

      if (bPOChangedFromUPS)
      {
        if (m_XMLResData.bIsLangAddon)
          MRGPOHandler.BumpLangAddonXMLVersion();
        bResChangedFromUPS = true;
        m_lChangedLangsFromUPS.insert(sLCode);
      }
    }

    //Pass Resource data to the newly created UPD PO classes for later use (PO file creation)
    if (m_mapUPD.find(sLCode) != m_mapUPD.end())
    {
      CPOHandler& UPDPOHandler = m_mapUPD.at(sLCode);
      UPDPOHandler.SetXMLReasData(m_XMLResData);
      UPDPOHandler.SetIfIsSourceLang(sLCode == m_XMLResData.sSRCLCode);
      UPDPOHandler.SetPOType(UPDATEPO);
      UPDPOHandler.CreateHeader(m_AddonXMLHandler.GetResHeaderPretext(), sLCode);

      m_lLangsToUPD.insert(sLCode);
    }
  }

  //If resource has been changed in any language, bump the language addon version
  if (bResChangedFromUPS && !m_XMLResData.bIsLangAddon)
    m_AddonXMLHandler.SetBumpAddonVersion();

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

// Check if there is a POHandler existing in mapTRX for language code sLCode.
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

//Check if in previous upstream PO file we can find the particular entry or not
//if not, that means that we have a true new translation coming from UPS not from TRX
bool CResourceHandler::FindPrevUPSEntry(const std::string sLCode, CPOEntry &EntryToFind)
{
  T_itmapPOFiles PrevUPSIterator = m_mapPREV.find(sLCode);
  if (PrevUPSIterator == m_mapPREV.end())
    return false;
  return PrevUPSIterator->second.FindEntry(EntryToFind);
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

void CResourceHandler::WriteMergedPOFiles(const std::string& sAddonXMLPath, const std::string& sLangAddonXMLPath, const std::string& sChangeLogPath, const std::string& sLangPath)
{
  if (!m_XMLResData.bIsLangAddon)
  {
    m_AddonXMLHandler.WriteAddonXMLFile(sAddonXMLPath);
    if (!m_XMLResData.sChgLogFormat.empty())
      m_AddonXMLHandler.WriteAddonChangelogFile(sChangeLogPath);
  }

  if (m_XMLResData.bHasOnlyAddonXML)
    return;


  for (T_itmapPOFiles itmapPOFiles = m_mapMRG.begin(); itmapPOFiles != m_mapMRG.end(); itmapPOFiles++)
  {
    const std::string& sLCode = itmapPOFiles->first;
    std::string strPODir, strAddonDir;
    strPODir = g_CharsetUtils.ReplaceLanginURL(sLangPath, g_CharsetUtils.GetLFormFromPath(m_XMLResData.LOC.LPath), sLCode);

    CPOHandler& POHandler = m_mapMRG.at(sLCode);

    POHandler.WritePOFile(strPODir);

    // Write individual addon.xml files for language-addons
    if (m_XMLResData.bIsLangAddon)
    {
      strAddonDir = g_CharsetUtils.ReplaceLanginURL(sLangAddonXMLPath, g_CharsetUtils.GetLFormFromPath(m_XMLResData.LOC.LPath), sLCode);
      POHandler.WriteLangAddonXML(strAddonDir);
    }
  }
 return;
}

void CResourceHandler::WriteLOCPOFiles()
{
    std::string sLOCGITDir = m_XMLResData.sUPSLocalPath + m_XMLResData.LOC.Owner + "/" + m_XMLResData.LOC.Repo + "/" + m_XMLResData.LOC.Branch + "/";
    std::string sLOCSRCGITDir = m_XMLResData.sUPSLocalPath + m_XMLResData.LOCSRC.Owner + "/" + m_XMLResData.LOCSRC.Repo + "/" + m_XMLResData.LOCSRC.Branch + "/";

    std::string sAddonXMLPath = sLOCGITDir + m_XMLResData.LOC.AXMLPath;
    std::string sChangeLogPath =  sLOCGITDir + m_XMLResData.LOC.ChLogPath;
    std::string sLangPath  = sLOCGITDir + m_XMLResData.LOC.LPath;
//  std::string sLangAddonXMLPath = sLOCGITDir + m_XMLResData.LOC.AXMLPath;

    std::string sLangPathSRC  = sLOCSRCGITDir + m_XMLResData.LOCSRC.LPath;
    std::string sLangAddonXMLPathSRC = sLOCSRCGITDir + m_XMLResData.LOCSRC.AXMLPath;


  if (!m_XMLResData.bIsLangAddon)
  {
    m_AddonXMLHandler.WriteAddonXMLFile(sAddonXMLPath);
    if (!m_XMLResData.sChgLogFormat.empty())
      m_AddonXMLHandler.WriteAddonChangelogFile(sChangeLogPath);
  }

  if (m_XMLResData.bHasOnlyAddonXML)
    return;


  for (T_itmapPOFiles itmapPOFiles = m_mapMRG.begin(); itmapPOFiles != m_mapMRG.end(); itmapPOFiles++)
  {
    const std::string& sLCode = itmapPOFiles->first;
    std::string strPODir, strAddonDir;
    if (sLCode != m_XMLResData.sSRCLCode || !m_XMLResData.bIsLangAddon)
      strPODir = g_CharsetUtils.ReplaceLanginURL(sLangPath, g_CharsetUtils.GetLFormFromPath(m_XMLResData.LOC.LPath), sLCode);
    else
      strPODir = g_CharsetUtils.ReplaceLanginURL(sLangPathSRC, g_CharsetUtils.GetLFormFromPath(m_XMLResData.LOCSRC.LPath), sLCode);

    CPOHandler& POHandler = m_mapMRG.at(sLCode);

    POHandler.WritePOFile(strPODir);

    // Write individual addon.xml files for language-addons
    if (m_XMLResData.bIsLangAddon)
    {
      if (sLCode != m_XMLResData.sSRCLCode)
        strAddonDir = g_CharsetUtils.ReplaceLanginURL(sAddonXMLPath, g_CharsetUtils.GetLFormFromPath(m_XMLResData.LOC.LPath), sLCode);
      else
        strAddonDir = g_CharsetUtils.ReplaceLanginURL(sLangAddonXMLPathSRC, g_CharsetUtils.GetLFormFromPath(m_XMLResData.LOC.LPath), sLCode);

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
    strPODir = g_CharsetUtils.ReplaceLanginURL(strPath, m_XMLResData.sBaseLForm, sLCode);

    CPOHandler& POHandler = m_mapUPD.at(sLCode);

    POHandler.WritePOFile(strPODir);
  }
  return;
}

void CResourceHandler::GenerateMergedPOFiles()
{

  printf ("%s", RESET);

  if (!m_XMLResData.bIsLangAddon)
    m_AddonXMLHandler.ClearAllAddonXMLEntries();


  printf("Generating merged and update PO files: %s%s%s\n", KMAG, m_XMLResData.sResName.c_str(), RESET);
  if (!m_lChangedLangsFromUPS.empty())
  {
    printf("  Changed Langs in strings files from upstream: ");
    PrintChangedLangs(m_lChangedLangsFromUPS);
    printf ("\n");
  }

  for (T_itmapPOFiles itmapPOFiles = m_mapMRG.begin(); itmapPOFiles != m_mapMRG.end(); itmapPOFiles++)
  {
    const std::string& sLCode = itmapPOFiles->first;

    CPOHandler& POHandler = m_mapMRG.at(sLCode);

    POHandler.GeneratePOFile();

    if (!m_XMLResData.bIsLangAddon)
      m_AddonXMLHandler.SetAddonXMLEntry(POHandler.GetAddonXMLEntry(), sLCode);
  }

  if (!m_XMLResData.bIsLangAddon)
  {
    m_AddonXMLHandler.GenerateAddonXMLFile();
    m_AddonXMLHandler.GenerateChangelogFile(m_XMLResData.sChgLogFormat);
  }

  return;
}

void CResourceHandler::GenerateUpdatePOFiles()
{
  if (!m_lLangsToUPD.empty())
  {
    printf("%s  Langs to update%s to Transifex from upstream: ", KRED, RESET);
    PrintChangedLangs(m_lLangsToUPD);
    printf ("\n");
  }

  if (!m_lLangsWithDeletedEntry.empty())
  {
    printf("  Langs which has entry to %sdelete upstream%s, like deleted at Transifex: ", KRED, RESET);
    PrintChangedLangs(m_lLangsWithDeletedEntry);
    printf ("\n");
  }

  for (T_itmapPOFiles itmapPOFiles = m_mapUPD.begin(); itmapPOFiles != m_mapUPD.end(); itmapPOFiles++)
  {
    const std::string& sLCode = itmapPOFiles->first;
    CPOHandler& POHandler = m_mapUPD.at(sLCode);

    POHandler.GeneratePOFile();
  }
  return;
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

    LCode = g_LCodeHandler.VerifyLangCode(LCode, m_XMLResData.TRX.LForm);

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
      g_Fileversion.SetVersionForURL(strURL + "translation/" + g_LCodeHandler.GetLangFromLCode(LCode, m_XMLResData.TRX.LForm) + "/?file", strModTime);
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
  std::string sFileList = g_HTTPHandler.GetGitFileListToSTR(m_XMLResData.sUPSLocalPath, m_XMLResData.UPS, m_XMLResData.bForceGitDloadToCache);

  size_t posLF = 0;
  size_t posNextLF = 0;

  std::string sAXMLLForm = g_CharsetUtils.GetLFormFromPath(m_XMLResData.UPS.AXMLPath);
  std::string sLForm = g_CharsetUtils.GetLFormFromPath(m_XMLResData.UPS.LPath);

  std::string sVersion;
  std::set<std::string> listLangs;

  while ((posNextLF = sFileList.find('\n', posLF)) != std::string::npos)
  {
    std::string sLine = sFileList.substr(posLF +1, posNextLF - posLF-1);
    size_t posWS1, posWS2, posWS3;
    posWS1 = sLine.find(' ');
    posWS2 = sLine.find(' ', posWS1+1);
    posWS3 = sLine.find('\t', posWS2+1);

    if (posWS1 == std::string::npos || posWS2 == std::string::npos || posWS3 == std::string::npos)
      CLog::Log(logINFO, "ResHandler::GetAvailLangsGITHUB: Wrong file list format for local github clone filelist, for resource %s", m_XMLResData.sResName.c_str());

    std::string sSHA = sLine.substr(posWS1 +1, posWS2 - posWS1);
    std::string sReadPath = sLine.substr(posWS3 + 1);

    posLF = posNextLF + 1;

    if (!m_XMLResData.bHasOnlyAddonXML)
    {
      //Get version of strings.po files
      std::string sMatchedLangalias = g_CharsetUtils.GetLangnameFromPath(sReadPath,  m_XMLResData.UPS.LPath, sLForm);
      std::string sFoundLangCode = g_LCodeHandler.GetLangCodeFromAlias(sMatchedLangalias, sLForm);

      if (sFoundLangCode != "")
      {
        listLangs.insert(sFoundLangCode);
        CGITData GitData = m_XMLResData.UPS;
        g_CharsetUtils.replaceAllStrParts(&GitData.LPath, sLForm, sMatchedLangalias);
        g_Fileversion.SetVersionForURL("git://" + GitData.Owner + "/" + GitData.Repo + "/" + GitData.Branch + "/" + GitData.LPath, sSHA);
        continue;
      }
    }
    if (m_XMLResData.bIsLangAddon)
    {
      //If we have a language addon, we also check if there is any language name dependent addon.xml file
      std::string sMatchedLangalias = g_CharsetUtils.GetLangnameFromPath(sReadPath, m_XMLResData.UPS.AXMLPath, sAXMLLForm);
      std::string sFoundLangCode = g_LCodeHandler.GetLangCodeFromAlias(sMatchedLangalias, sAXMLLForm);
      if (sFoundLangCode != "")
      {
        listLangs.insert(sFoundLangCode);
        CGITData GitData = m_XMLResData.UPS;
        g_CharsetUtils.replaceAllStrParts(&GitData.AXMLPath, sAXMLLForm, sMatchedLangalias);
        g_Fileversion.SetVersionForURL("git://" + GitData.Owner + "/" + GitData.Repo + "/" + GitData.Branch + "/" + GitData.AXMLPath, sSHA);
      }
    }
    else if (sReadPath == m_XMLResData.UPS.AXMLPath)
    {
      //Get version for addon.xml file
      CGITData GitData = m_XMLResData.UPS;
      g_Fileversion.SetVersionForURL("git://" + GitData.Owner + "/" + GitData.Repo + "/" + GitData.Branch + "/" + GitData.AXMLPath, sSHA);
    }
    else if (sReadPath == m_XMLResData.UPS.ChLogPath)
    {
      //Get version for changelog.txt file
      CGITData GitData = m_XMLResData.UPS;
      g_Fileversion.SetVersionForURL("git://" + GitData.Owner + "/" + GitData.Repo + "/" + GitData.Branch + "/" + GitData.ChLogPath, sSHA);
    }
  }

  return listLangs;
};

void CResourceHandler::GetSRCFilesGitData()
{
  std::string sFileList = g_HTTPHandler.GetGitFileListToSTR (m_XMLResData.sUPSLocalPath, m_XMLResData.UPSSRC, m_XMLResData.bForceGitDloadToCache);

  size_t posLF = 0;
  size_t posNextLF = 0;

  std::string sVersion;

  std::string sLPathSRC = g_CharsetUtils.ReplaceLanginURL(m_XMLResData.UPSSRC.LPath, g_CharsetUtils.GetLFormFromPath(m_XMLResData.UPSSRC.LPath), m_XMLResData.sSRCLCode);
  std::string sLAXMLPathSRC = g_CharsetUtils.ReplaceLanginURL(m_XMLResData.UPSSRC.AXMLPath, g_CharsetUtils.GetLFormFromPath(m_XMLResData.UPSSRC.AXMLPath), m_XMLResData.sSRCLCode);

  while ((posNextLF = sFileList.find('\n', posLF)) != std::string::npos)
  {
    std::string sLine = sFileList.substr(posLF +1, posNextLF - posLF-1);
    size_t posWS1, posWS2, posWS3;
    posWS1 = sLine.find(' ');
    posWS2 = sLine.find(' ', posWS1+1);
    posWS3 = sLine.find('\t', posWS2+1);

    if (posWS1 == std::string::npos || posWS2 == std::string::npos || posWS3 == std::string::npos)
      CLog::Log(logINFO, "ResHandler::GetAvailLangsGITHUB: Wrong file list format for local github clone filelist, for resource %s", m_XMLResData.sResName.c_str());

    std::string sSHA = sLine.substr(posWS1 +1, posWS2 - posWS1);
    std::string sReadPath = sLine.substr(posWS3 + 1);

    posLF = posNextLF + 1;

    if (sReadPath == sLPathSRC)
    {
      //Set version for SRC strings.po
      CGITData GitData = m_XMLResData.UPSSRC;
      g_Fileversion.SetVersionForURL("git://" + GitData.Owner + "/" + GitData.Repo + "/" + GitData.Branch + "/" + sLPathSRC, sSHA);
    }
    else if (sReadPath == sLAXMLPathSRC)
    {
      //Set version for SRC language addon.xml
      CGITData GitData = m_XMLResData.UPSSRC;
      g_Fileversion.SetVersionForURL("git://" + GitData.Owner + "/" + GitData.Repo + "/" + GitData.Branch + "/" + sLAXMLPathSRC, sSHA);
    }
  }
}

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

//TODO use std::set instead of list everywhere
void CResourceHandler::PrintChangedLangs(const std::set<std::string>& lChangedLangs)
{
  std::set<std::string>::iterator itLangs;
  std::size_t counter = 0;
  printf ("%s", KCYN);
  for (itLangs = lChangedLangs.begin() ; itLangs != lChangedLangs.end(); itLangs++)
  {
    printf ("%s ", itLangs->c_str());
    counter++;
    if (counter > 10)
    {
      printf ("+ %i langs ", (int)lChangedLangs.size() - 10);
      break;
    }
  }
  printf ("%s", RESET);
}

void CResourceHandler::UploadResourceToTransifex(bool bNewResourceOnTRX)
{
  g_HTTPHandler.Cleanup();
  g_HTTPHandler.ReInit();

  printf ("Uploading files for resource: %s%s%s", KMAG, m_XMLResData.sResName.c_str(), RESET);


  if (m_mapUPD.empty()) // No update needed for the specific resource (not even an English one)
  {
    printf (", no upload was necesarry.\n");
    return;
  }

  g_HTTPHandler.SetLocation("UPD");
  g_HTTPHandler.SetProjectName(m_XMLResData.UPD.ProjectName);
  g_HTTPHandler.SetResName(m_XMLResData.sResName);
  g_HTTPHandler.SetFileName("string.po");
  g_HTTPHandler.SetDataFile(false);

  g_HTTPHandler.SetLCode(m_XMLResData.sSRCLCode);

  if (bNewResourceOnTRX)
  {
    // We create the new resource on transifex and also upload the English source file at once

    m_mapUPD.at(m_XMLResData.sSRCLCode).CreateNewResource();

    g_HTTPHandler.Cleanup();
    g_HTTPHandler.ReInit();
  }

  printf ("\n");

  // Upload the source file in case there is one to update
  if (m_mapUPD.find(m_XMLResData.sSRCLCode) != m_mapUPD.end() && !bNewResourceOnTRX)
    m_mapUPD.at(m_XMLResData.sSRCLCode).PutSRCFileToTRX();


  for (T_itmapPOFiles it = m_mapUPD.begin(); it!=m_mapUPD.end(); it++)
  {
    const std::string& sLCode = it->first;
    CPOHandler& POHandler = it->second;

    if (sLCode == m_XMLResData.sSRCLCode) // Let's not upload the Source language file again
      continue;

    g_HTTPHandler.SetLCode(sLCode);

    POHandler.PutTranslFileToTRX();
  }
}

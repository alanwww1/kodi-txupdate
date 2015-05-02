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
#include "JSONHandler.h"
#include "HTTPUtils.h"
#include "Langcodes.h"
#include "Settings.h"

using namespace std;

CResourceHandler::CResourceHandler()
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

  std::list<std::string> listLangsTX = g_Json.ParseAvailLanguagesTX(strtemp, strURL, g_Settings.GetDefaultTXLFormat());

  CPOHandler POHandler;

  for (std::list<std::string>::iterator it = listLangsTX.begin(); it != listLangsTX.end(); it++)
  {
    printf (" %s", it->c_str());
    m_mapPOFiles[*it] = POHandler;
    CPOHandler * pPOHandler = &m_mapPOFiles[*it];
    pPOHandler->FetchPOURLToMem(strURL + "translation/" + g_LCodeHandler.GetLangFromLCode(*it, g_Settings.GetDefaultTXLFormat()) + "/?file", false);
    pPOHandler->SetIfIsSourceLang(*it == g_Settings.GetSourceLcode());
    std::string strLang = *it;
    CLog::LogTable(logINFO, "txfetch", "\t\t\t%s\t\t%i\t\t%i", strLang.c_str(), pPOHandler->GetNumEntriesCount(),
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
    g_Json.ParseAddonXMLVersionGITHUB(strtemp, XMLResdata.strUPSAddonURLRoot, XMLResdata.strUPSAddonXMLFilename, XMLResdata.strUPSChangelogName);

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

  listGithubLangs = g_Json.ParseAvailLanguagesGITHUB(strtemp, XMLResdata.strUPSLangURL, XMLResdata.strUPSLangFormat,
                                                     XMLResdata.strUPSAddonURL, XMLResdata.bIsLanguageAddon);

  listLangs = listGithubLangs;
  listLangs.sort();
  if (!XMLResdata.strUPSSourceLangURL.empty()) // we have a language-addon with different SRC language upstream URL
  {
    printf(" LanglistSRC");
    strtemp.clear();
    strGitHubURL.clear();
//TODO Rather use the language directory itself for language addons to have a chance to get the changes of the individual addon.xml files
//TODO load source repository first to not download the outdated language addons
    strGitHubURL = g_HTTPHandler.GetGitHUBAPIURL(g_CharsetUtils.GetRoot(XMLResdata.strUPSSourceLangURL, XMLResdata.strUPSAddonLangFormat));
    strtemp = g_HTTPHandler.GetURLToSTR(strGitHubURL);
    if (strtemp.empty())
      CLog::Log(logERROR, "ResHandler::FetchPOFilesUpstreamToMem: error getting source language file list from github.com");

    listGithubLangs = g_Json.ParseAvailLanguagesGITHUB(strtemp, XMLResdata.strUPSSourceLangURL, XMLResdata.strUPSLangFormat,
                                                       XMLResdata.strUPSSourceLangAddonURL, XMLResdata.bIsLanguageAddon);
  }

  bool bResult;

  for (std::list<std::string>::iterator it = listLangs.begin(); it != listLangs.end(); it++)
  {
    CPOHandler POHandler;
    bool bIsSourceLang = *it == g_Settings.GetSourceLcode();
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

    if (XMLResdata.strUPSLangFileName == "strings.xml")
      bResult = POHandler.FetchXMLURLToMem(strDloadURL);
    else
      bResult = POHandler.FetchPOURLToMem(strDloadURL,false);
    if (bResult)
    {
      m_mapPOFiles[*it] = POHandler;
      std::string strLang = *it;
      CLog::LogTable(logINFO, "upstrFetch", "\t\t\t%s\t\t%i\t\t%i\t\t%i", strLang.c_str(), POHandler.GetNumEntriesCount(),
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
    strPath = strProjRootDir + strPrefixDir + DirSepChar + XMLResdata.strName + DirSepChar + g_Settings.GetBaseLCode() + DirSepChar + "strings.po";
    strLangFormat = g_Settings.GetBaseLCode();
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
    else if (g_CharsetUtils.bISXMLFile(XMLResdata.strLOCLangFileName))
      pPOHandler->WriteXMLFile(strPODir);
    else
      CLog::Log(logERROR, "ResHandler::WritePOToFiles: unknown local fileformat: %s", XMLResdata.strLOCLangFileName.c_str());

    // Write individual addon.xml files for language-addons
    if (!strAddonXMLPath.empty())
    {
      strAddonDir = g_CharsetUtils.ReplaceLanginURL(strAddonXMLPath, strLangFormat, itmapPOFiles->first);
      pPOHandler->WriteLangAddonXML(strAddonDir);
    }

    CLog::LogTable(logINFO, "writepo", "\t\t\t%s\t\t%i\t\t%i", itmapPOFiles->first.c_str(), pPOHandler->GetNumEntriesCount(),
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
  if (!XMLResdata.bIsLanguageAddon && strPrefixDir == g_Settings.GetMergedLangfilesDir())
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

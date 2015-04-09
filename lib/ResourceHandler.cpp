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
#include "xbmclangcodes.h"
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

bool CResourceHandler::FetchPOFilesTXToMem(const CXMLResdata &XMLResdata, std::string strURL, bool bIsKODICore)
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

  std::list<std::string> listLangsTX = g_Json.ParseAvailLanguagesTX(strtemp, bIsKODICore, strURL);

  CPOHandler POHandler;

  for (std::list<std::string>::iterator it = listLangsTX.begin(); it != listLangsTX.end(); it++)
  {
    printf (" %s", it->c_str());
    m_mapPOFiles[*it] = POHandler;
    CPOHandler * pPOHandler = &m_mapPOFiles[*it];
    pPOHandler->FetchPOURLToMem(strURL + "translation/" + *it + "/?file", false);
    pPOHandler->SetIfIsSourceLang(*it == g_LCodeHandler.GetLangFromLCode(g_Settings.GetSourceLcode(), XMLResdata.strTXLangFormat));
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

bool CResourceHandler::FetchPOFilesUpstreamToMem(const CXMLResdata &XMLResdata, std::list<std::string> listLangsTX)
{
  g_HTTPHandler.Cleanup();
  g_HTTPHandler.ReInit();
  CLog::Log(logINFO, "ResHandler: Starting to load resource from Upsream URL: %s into memory",XMLResdata.strUPSAddonURL.c_str());

  std::string strLangdirPrefix, strGitHubURL, strtemp;

  if (!XMLResdata.strUPSAddonURL.empty() && XMLResdata.strUPSAddonLangFormat.empty()) // kodi core language addon has individual addon.xml files
  {
    // We get the version of the addon.xml and changelog.txt files here
    strGitHubURL = g_HTTPHandler.GetGitHUBAPIURL(XMLResdata.strUPSAddonURLRoot, "");
    printf(" Dir");
    strtemp = g_HTTPHandler.GetURLToSTR(strGitHubURL);
    if (strtemp.empty())
      CLog::Log(logERROR, "ResHandler::FetchPOFilesUpstreamToMem: error getting addon.xml file version from github.com");

//TODO separate addon.xml and changelog.txt version parsing as they can be in a different place
    g_Json.ParseAddonXMLVersionGITHUB(strtemp, XMLResdata.strUPSAddonURLRoot, XMLResdata.strUPSAddonXMLFilename, XMLResdata.strUPSChangelogName);

    printf(" Addxml");
    m_AddonXMLHandler.FetchAddonXMLFileUpstr(XMLResdata.strUPSAddonURL);
    if (!XMLResdata.strUPSChangelogURL.empty())
    {
      printf(" Chlog");
      m_AddonXMLHandler.FetchAddonChangelogFile(XMLResdata.strUPSChangelogURL);
    }

    if (XMLResdata.strUPSLangURL.empty())
      return true;
  }

  std::list<std::string> listLangs, listGithubLangs;

  printf(" Langlist");
  strtemp.clear();
  strGitHubURL.clear();
  strGitHubURL = g_HTTPHandler.GetGitHUBAPIURL(XMLResdata.strUPSLangURLRoot, "");

  strtemp = g_HTTPHandler.GetURLToSTR(strGitHubURL);
  if (strtemp.empty())
    CLog::Log(logERROR, "ResHandler::FetchPOFilesUpstreamToMem: error getting po file list from github.com");

  listGithubLangs = g_Json.ParseAvailLanguagesGITHUB(strtemp, XMLResdata.strUPSLangURL, XMLResdata.strUPSLangFormat);

  listLangs=listGithubLangs;

  bool bResult;

  for (std::list<std::string>::iterator it = listLangs.begin(); it != listLangs.end(); it++)
  {
    CPOHandler POHandler;
    POHandler.SetIfIsSourceLang(*it == g_Settings.GetSourceLcode());
    printf (" %s", it->c_str());

    std::string strDloadURL = g_CharsetUtils.replaceStrParts(XMLResdata.strUPSLangURL, XMLResdata.strUPSAddonLangFormat,
                              g_LCodeHandler.GetLangFromLCode(*it));

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
  std::string strPath, strLangFormat;
  if (!bTXUpdFile)
  {
    strPath = strProjRootDir + strPrefixDir + DirSepChar + XMLResdata.strLOCLangFileName;
    strLangFormat = XMLResdata.strLOCLangFormat;
  }
  else
  {
    strPath = strProjRootDir + strPrefixDir + DirSepChar + XMLResdata.strName + DirSepChar + "$(NEWLCODE)" + DirSepChar + "strings.po";
    strLangFormat = "$(NEWLCODE)";
  }

  if (bTXUpdFile && !m_mapPOFiles.empty())
    printf("Languages to update from upstream to upload to Transifex:");
  size_t counter = 0;

  for (T_itmapPOFiles itmapPOFiles = m_mapPOFiles.begin(); itmapPOFiles != m_mapPOFiles.end(); itmapPOFiles++)
  {
    std::string strPODir = g_CharsetUtils.ReplaceLanginURL(strPath, strLangFormat, itmapPOFiles->first);

    if (bTXUpdFile && counter < 20)
      printf (" %s", itmapPOFiles->first.c_str());
    if ((bTXUpdFile && counter == 19) && m_mapPOFiles.size() != 20)
      printf ("+%i Langs", m_mapPOFiles.size()-19);

    CPOHandler * pPOHandler = &m_mapPOFiles[itmapPOFiles->first];
    if (g_CharsetUtils.bISPOFile(XMLResdata.strLOCLangFileName) || bTXUpdFile)
      pPOHandler->WritePOFile(strPODir);
    else if (g_CharsetUtils.bISPOFile(XMLResdata.strLOCLangFileName))
      pPOHandler->WriteXMLFile(strPODir);
    else
      CLog::Log(logERROR, "ResHandler::WritePOToFiles: unknown local fileformat: %s", XMLResdata.strLOCLangFileName.c_str());

    CLog::LogTable(logINFO, "writepo", "\t\t\t%s\t\t%i\t\t%i", itmapPOFiles->first.c_str(), pPOHandler->GetNumEntriesCount(),
              pPOHandler->GetClassEntriesCount());
  }
  if (bTXUpdFile && !m_mapPOFiles.empty())
    printf("\n");

  CLog::LogTable(logADDTABLEHEADER, "writepo", "--------------------------------------------------------------\n");
  CLog::LogTable(logADDTABLEHEADER, "writepo", "WritePOFiles:\tLang\t\tIDEntry\t\tClassEntry\n");
  CLog::LogTable(logADDTABLEHEADER, "writepo", "--------------------------------------------------------------\n");
  CLog::LogTable(logCLOSETABLE, "writepo", "");

//TODO handle new language addons where each language has a separate addon.xml file
  // update local addon.xml file
  if (strResname != "kodi.core" && strPrefixDir == g_Settings.GetMergedLangfilesDir())
  {
    bool bResChangedFromUpstream = !m_lChangedLangsFromUpstream.empty() || !m_lChangedLangsInAddXMLFromUpstream.empty();
    m_AddonXMLHandler.UpdateAddonXMLFile(XMLResdata.strLOCAddonPath, bResChangedFromUpstream);
    if (!XMLResdata.strChangelogFormat.empty())
      m_AddonXMLHandler.UpdateAddonChangelogFile(XMLResdata.strLOCChangelogURL, XMLResdata.strChangelogFormat, bResChangedFromUpstream);
  }

  return true;
}

T_itmapPOFiles CResourceHandler::IterateToMapIndex(T_itmapPOFiles it, size_t index)
{
  for (size_t i = 0; i != index; i++) it++;
  return it;
}

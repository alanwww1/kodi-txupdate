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

#include "ProjectHandler.h"
#include <list>
#include "HTTPUtils.h"
#include "jsoncpp/json/json.h"
#include <algorithm>
#include "ConfigHandler.h"
#include "Log.h"
#include "CharsetUtils/CharsetUtils.h"
#include "FileUtils/FileUtils.h"
#include "Langcodes.h"

CProjectHandler::CProjectHandler()
{};

CProjectHandler::~CProjectHandler()
{};

void CProjectHandler::LoadConfigToMem(bool bForceUseCache)
{
  CConfigHandler ConfigHandler;
  ConfigHandler.LoadResDataToMem(m_strProjDir, m_mapResData, &m_MapGitRepos, m_mapResOrder);

  size_t iCacheExpire = -1; //in case we are in force cache use, we set cache expiration to the highest possible value
  if (!bForceUseCache)
    iCacheExpire = m_mapResData.begin()->second.iCacheExpire;

  g_HTTPHandler.SetHTTPCacheExpire(iCacheExpire);
  m_BForceUseCache = bForceUseCache;
}

bool CProjectHandler::FetchResourcesFromTransifex()
{

  g_HTTPHandler.Cleanup();
  g_HTTPHandler.ReInit();
  CLog::Log(logPRINT, "TXresourcelist");

  //TODO multiple projects
  const std::string& sProjectName = m_mapResData.begin()->second.TRX.ProjectName;
  g_HTTPHandler.SetLocation("TRX");
  g_HTTPHandler.SetProjectName(sProjectName);
  g_HTTPHandler.SetResName("");
  g_HTTPHandler.SetLCode("");
  g_HTTPHandler.SetFileName("TXResourceList.json");
  g_HTTPHandler.SetDataFile(true);

  std::string strtemp = g_HTTPHandler.GetURLToSTR("https://www.transifex.com/api/2/project/" + sProjectName + "/resources/");
  if (strtemp.empty())
    CLog::Log(logERROR, "ProjectHandler::FetchResourcesFromTransifex: error getting resources from transifex.net");

  CLog::Log(logPRINT, "\n\n");

  std::set<std::string> listResNamesAvailOnTX = ParseResources(strtemp);
//TODO collect out txprojectnames, check all resources in them, if we need to download
  for (T_itResData it = m_mapResData.begin(); it != m_mapResData.end(); it++)
  {
    const std::string& sResName = it->second.sResName;

    CLog::Log(logPRINT, "%s%s%s (", KMAG, sResName.c_str(), RESET);

    if (listResNamesAvailOnTX.find(sResName) == listResNamesAvailOnTX.end())
    {
      CLog::Log(logPRINT, " ) Not yet available at Transifex\n");
      continue;
    }

    const CResData& ResData = m_mapResData[sResName];
    CResourceHandler NewResHandler(ResData);

    if (!m_BForceUseCache)
      g_HTTPHandler.SetHTTPCacheExpire(ResData.iCacheExpire);

    m_mapResources[sResName] = NewResHandler;
    m_mapResources[sResName].FetchPOFilesTXToMem();
    CLog::Log(logPRINT, " )\n");
  }
  return true;
};

void CProjectHandler::GITPushLOCGitRepos()
{
  unsigned int iCounter = 0;
  std::map<unsigned int, CBasicGITData> MapReposToPush;
  for (std::map<std::string, CBasicGITData>::iterator it = m_MapGitRepos.begin(); it != m_MapGitRepos.end(); it++)
  {
    iCounter++;
    CBasicGITData GitData = it->second;
    MapReposToPush[iCounter] = GitData;
  }

  bool bFirstRun = true;
  std::string strInput;
  CLog::Log(logPRINT, "\n");

  do
  {
    if (!bFirstRun)
    {
      //Jump back to the start of list with the cursor
      int iLines = MapReposToPush.size() +2;
      std::string sNumOfLines = g_CharsetUtils.IntToStr(iLines);
      std::string sEscapeCode = "\033[" +sNumOfLines + "A";
      CLog::Log(logPRINT, "%s", sEscapeCode.c_str());
    }

    //Print current state
    for (std::map<unsigned int, CBasicGITData>::iterator it = MapReposToPush.begin(); it != MapReposToPush.end(); it++)
    {
      CBasicGITData GitData = it->second;
      size_t iLastPushAge = g_HTTPHandler.GetLastGitPushAge(GitData.Owner, GitData.Repo, GitData.Branch);
      bool bGitPush = ((GitData.iGitPushInterval * 24 * 60 * 60) < iLastPushAge);
      bGitPush = bGitPush||GitData.bForceGitPush;
      bGitPush = bGitPush&&!GitData.bSkipGitPush;
      float fLastPushAge = (float)iLastPushAge / (float)86400;
      unsigned int iLastPushAgeDays = (unsigned int)fLastPushAge;

      if (iLastPushAgeDays > 100)
        iLastPushAgeDays = 99;

      CLog::Log(logPRINT, "%s%i|%s%s%s|%s%i /%s%i|%s|%s%s%s\n",
                (it->first < 10)?" ":"", it->first,
                bGitPush?KRED:KWHT, bGitPush?"*":" ", RESET,
                (iLastPushAgeDays < 10)?" ":"", iLastPushAgeDays,
                (GitData.iGitPushInterval < 10)?" ":"", GitData.iGitPushInterval,
                (GitData.bForceGitPush||GitData.bSkipGitPush)?(GitData.bForceGitPush?"Force":" Skip"):"     ",
                KMAG, (GitData.Owner + "/" + GitData.Repo + "/" + GitData.Branch).c_str(), RESET);
    }

    CLog::Log(logPRINT, "\n\n\033[2A");
    CLog::Log(logPRINT, "\n%sChoose option:%s %s0%s:Continue with pushing to Github %s1%s:Option 1 %s2%s:Option 2 %ss%s:Skip. Your Choice:                           \b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b", KRED, RESET, KMAG, RESET, KMAG, RESET, KMAG, RESET, KRED, RESET);
    cin >> strInput;

    if (strInput.find("fp") == 0 || strInput.find("sp") == 0)
    {
      // We have a forcepush or skippush request
      bool bForceGitPush = strInput.find("fp") ==0;
      bool bSkipGitPush = !bForceGitPush;

      std::set<int> ListRepos;

      if (!ParseRepoList(strInput, ListRepos))
        CLog::Log(logERROR, "Unable to parse command, with wron syntax: %s", strInput.c_str());

      for (std::set<int>::iterator it = ListRepos.begin(); it!= ListRepos.end(); it++)
      {
        if (MapReposToPush.find(*it) == MapReposToPush.end())
          CLog::Log(logERROR, "Wrong projectnumber given in command %s, wrong number: %i", strInput.c_str(), *it);

        MapReposToPush[*it].bForceGitPush = bForceGitPush;
        MapReposToPush[*it].bSkipGitPush = bSkipGitPush;
      }
    }

/*    if (charInput == '1')
    {
    }
    else if (charInput == '2')
    {
    } */
    if (strInput == "s")
      return;

    bFirstRun = false;
  }
  while (strInput != "0" && strInput != "dr");


  //Make the actual push
  for (std::map<unsigned int, CBasicGITData>::iterator it = MapReposToPush.begin(); it != MapReposToPush.end(); it++)
  {
    CBasicGITData GitData = it->second;
    size_t iLastPushAge = g_HTTPHandler.GetLastGitPushAge(GitData.Owner, GitData.Repo, GitData.Branch);
    bool bGitPush = ((GitData.iGitPushInterval * 24 * 60 * 60) < iLastPushAge);
    bGitPush = bGitPush||GitData.bForceGitPush;
    bGitPush = bGitPush&&!GitData.bSkipGitPush;

    if (bGitPush)
    {
      if (strInput == "0") // if it was "dr" we are in "dry run" mode
        g_HTTPHandler.SetGitPushTime(GitData.Owner, GitData.Repo, GitData.Branch);

      CLog::Log(logPRINT, "\nPushing: %s%s%s\n", KMAG, (GitData.Owner + "/" + GitData.Repo + "/" + GitData.Branch).c_str(), RESET);

      std::string sGitHubRootNoBranch = GitData.sUPSLocalPath + GitData.Owner + "/" + GitData.Repo;
      std::string sGitHubRoot = sGitHubRootNoBranch + "/" + GitData.Branch;
      std::string sCommand;

      if (!g_File.FileExist(sGitHubRoot +  "/.git/config"))
        CLog::Log(logERROR, "Error while pushing. Directory is not a  GIT repo for Owner: %s, Repo: %s, Branch: %s\n", GitData.Owner.c_str(), GitData.Repo.c_str(), GitData.Branch.c_str());

      sCommand = "cd " + sGitHubRoot + ";";
      sCommand += "git push origin " + GitData.Branch;
      CLog::Log(logPRINT, "%s%s%s\n", KYEL, sCommand.c_str(), RESET);

      if (strInput == "0") // if it was "dr" we are in "dry run" mode
        g_File.SytemCommand(sCommand);
    }
  }
}

bool CProjectHandler::ParseRepoList(const std::string& sStringToParse, std::set<int>& ListRepos)
{
  if (sStringToParse.size() == 2)
    return false;

  size_t pos = 2;
  size_t nextPos;
  do
  {
    nextPos = sStringToParse.find_first_of(",-", pos);
    if (nextPos == std::string::npos || sStringToParse.at(nextPos) == ',')
    {
      std::string sNumber = sStringToParse.substr(pos, nextPos-pos);
      if (!isdigit(sNumber.at(0))) // verify if the first char is digit
        return false;

      int iNum  = strtol(sNumber.c_str(), NULL, 10);
      ListRepos.insert(iNum);
    }
    else
    {
      std::string sNumberFrom = sStringToParse.substr(pos, nextPos-pos);
      if (!isdigit(sNumberFrom.at(0))) // verify if the first char is digit
        return false;

      int iNumFrom  = strtol(sNumberFrom.c_str(), NULL, 10);

      pos =nextPos +1;
      nextPos = sStringToParse.find_first_of(",", pos);
      std::string sNumberTo = sStringToParse.substr(pos, (nextPos == std::string::npos)?std::string::npos:nextPos-pos);
      if (!isdigit(sNumberTo.at(0))) // verify if the first char is digit
        return false;

      int iNumTo  = strtol(sNumberTo.c_str(), NULL, 10);
      for (int i=iNumFrom; i<=iNumTo; i++)
        ListRepos.insert(i);
    }
    pos = nextPos +1;
  }
  while (nextPos != std::string::npos);

  return true;
}


bool CProjectHandler::FetchResourcesFromUpstream()
{
  g_HTTPHandler.GITPullUPSRepos(m_MapGitRepos, m_mapResData.begin()->second.bSkipGitReset);

  for (std::map<std::string, CResData>::iterator it = m_mapResData.begin(); it != m_mapResData.end(); it++)
  {
    const CResData& ResData = it->second;
    const std::string& sResName = it->first;

    CLog::Log(logPRINT, "%s%s%s (", KMAG, sResName.c_str(), RESET);

    if (m_mapResources.find(sResName) == m_mapResources.end()) // if it was not created in the map (by TX pull), make a new entry
    {
      CResourceHandler NewResHandler(ResData);
      m_mapResources[sResName] = NewResHandler;
    }

    if (!m_BForceUseCache)
      g_HTTPHandler.SetHTTPCacheExpire(ResData.iCacheExpire);

    m_mapResources[sResName].FetchPOFilesUpstreamToMem();
    CLog::Log(logPRINT, " )\n");
  }
  return true;
};


bool CProjectHandler::WriteResourcesToFile(std::string strProjRootDir)
{
//TODO
  g_File.DeleteDirectory(strProjRootDir + m_mapResData.begin()->second.sMRGLFilesDir);
  g_File.DeleteDirectory(strProjRootDir + m_mapResData.begin()->second.sUPDLFilesDir);

  for (T_itmapRes itmapResources = m_mapResources.begin(); itmapResources != m_mapResources.end(); itmapResources++)
  {
    const std::string& sResName = itmapResources->first;
    CResourceHandler& ResHandler = itmapResources->second;
    const CResData& ResData = m_mapResData[sResName];

    std::string sMergedLangDir = ResData.sProjRootDir + DirSepChar + ResData.sMRGLFilesDir + DirSepChar;
    std::string sAddonXMLPath = sMergedLangDir + ResData.MRG.AXMLPath;
    std::string sChangeLogPath =  sMergedLangDir + ResData.MRG.ChLogPath;
    std::string sLangPath  = sMergedLangDir + ResData.MRG.LPath;
    std::string sLangAddonXMLPath = sMergedLangDir + ResData.MRG.AXMLPath;
    ResHandler.GenerateMergedPOFiles ();
    ResHandler.WriteMergedPOFiles (sAddonXMLPath, sLangAddonXMLPath, sChangeLogPath, sLangPath);

    std::string sPathUpdate = ResData.sProjRootDir + ResData.sUPDLFilesDir + DirSepChar + ResData.sResName + DirSepChar + ResData.sBaseLForm + DirSepChar + "strings.po";
    ResHandler.GenerateUpdatePOFiles ();

    if (!m_BForceUseCache)
      g_HTTPHandler.SetHTTPCacheExpire(ResData.iCacheExpire);

    ResHandler.WriteUpdatePOFiles (sPathUpdate);
  }

  CLog::Log(logPRINT, "\n\n");
  return true;
};

bool CProjectHandler::WriteResourcesToLOCGitRepos(std::string strProjRootDir)
{
//TODO
  for (std::map<int, std::string>::iterator itResOrder = m_mapResOrder.begin(); itResOrder != m_mapResOrder.end(); itResOrder++)
  {
    const std::string& sResName = itResOrder->second;
    CResourceHandler& ResHandler = m_mapResources[sResName];

    ResHandler.WriteLOCPOFiles();
  }
  CLog::Log(logPRINT, "\n\n");
  return true;
};


bool CProjectHandler::CreateMergedResources()
{
  CLog::Log(LogHEADLINE, "MERGING RESOURCES\n");

  for (T_itmapRes it = m_mapResources.begin(); it != m_mapResources.end(); it++)
  {
    CResourceHandler& ResHandler = it->second;
    ResHandler.MergeResource();
  }

  return true;
}

void CProjectHandler::UploadTXUpdateFiles(std::string strProjRootDir)
{
  char charInput;
  CLog::Log(logPRINT, "\n");
  do
  {
    CLog::Log(logPRINT, "%sChoose option:%s %s0%s:Continue with update Transifex %s1%s:VIM UPD files %s2%s:VIM MRG files %ss%s:Skip. Your Choice:   \b\b", KRED, RESET, KMAG, RESET, KMAG, RESET, KMAG, RESET, KRED, RESET);
    cin >> charInput;

    if (charInput == '1')
    {
      std::string sCommand = "vim " + strProjRootDir + m_mapResData.begin()->second.sUPDLFilesDir;
      g_File.SytemCommand(sCommand);
    }
    else if (charInput == '2')
    {
      std::string sCommand = "vim " + strProjRootDir + m_mapResData.begin()->second.sMRGLFilesDir;
      g_File.SytemCommand(sCommand);
    }
    else if (charInput == 's')
      return;

    CLog::Log(logPRINT, "\e[A");
  }
  while (charInput != '0');

  CLog::Log(logPRINT, "                                                                                                                                        \n\e[A");

  g_HTTPHandler.Cleanup();
  g_HTTPHandler.ReInit();
  CLog::Log(logPRINT, "TXresourcelist");

  //TODO
  const std::string& strTargetProjectName = m_mapResData.begin()->second.UPD.ProjectName;

  //TODO ditry fix for always getting a fresh txlist here
  g_HTTPHandler.SetSkipCache(true);

  std::string strtemp = g_HTTPHandler.GetURLToSTR("https://www.transifex.com/api/2/project/" + strTargetProjectName + "/resources/");
  if (strtemp.empty())
    CLog::Log(logERROR, "ProjectHandler::FetchResourcesFromTransifex: error getting resources from transifex.net");

  g_HTTPHandler.SetSkipCache(false);
  CLog::Log(logPRINT, "\n\n");

  std::set<std::string> lResourcesAtTX = ParseResources(strtemp);

  g_HTTPHandler.Cleanup();
  g_HTTPHandler.ReInit();

  for (T_itmapRes it = m_mapResources.begin(); it != m_mapResources.end(); it++)
  {
    CResourceHandler& ResHandler = it->second;
    const std::string& sResName = it->first;
    const CResData& ResData = m_mapResData[sResName];

    if (!m_BForceUseCache)
      g_HTTPHandler.SetHTTPCacheExpire(ResData.iCacheExpire);

    ResHandler.UploadResourceToTransifex(lResourcesAtTX.find(sResName) == lResourcesAtTX.end());
  }

  return;
}

void CProjectHandler::MigrateTranslators()
{
  //TODO
  CResData Resdata = m_mapResData.begin()->second;
  const std::string& strProjectName = Resdata.TRX.ProjectName;
  const std::string& strTargetProjectName = Resdata.UPD.ProjectName;
  const std::string& strTargetTXLangFormat = Resdata.UPD.LForm;

  if (strProjectName.empty() || strTargetProjectName.empty() || strProjectName == strTargetProjectName)
    CLog::Log(logERROR, "Cannot tranfer translators database. Wrong projectname and/or target projectname,");

  std::map<std::string, std::string> mapCoordinators, mapReviewers, mapTranslators;

  CLog::Log(logPRINT, "\n%sCoordinators:%s\n", KGRN, RESET);
  mapCoordinators = g_LCodeHandler.GetTranslatorsDatabase("coordinators", strProjectName, Resdata);

  CLog::Log(logPRINT, "\n%sReviewers:%s\n", KGRN, RESET);
  mapReviewers = g_LCodeHandler.GetTranslatorsDatabase("reviewers", strProjectName, Resdata);

  CLog::Log(logPRINT, "\n%sTranslators:%s\n", KGRN, RESET);
  mapTranslators = g_LCodeHandler.GetTranslatorsDatabase("translators", strProjectName, Resdata);

  CLog::Log(LogHEADLINE, "PUSH TRANSLATION GROUPS TO TX\n");

  g_LCodeHandler.UploadTranslatorsDatabase(mapCoordinators, mapReviewers, mapTranslators, strTargetProjectName, strTargetTXLangFormat);
};

void CProjectHandler::InitLCodeHandler()
{
//TODO
  CResData Resdata = m_mapResData.begin()->second;
  g_LCodeHandler.Init(Resdata.sLDatabaseURL, Resdata);
}

std::set<std::string> CProjectHandler::ParseResources(std::string strJSON)
{
  Json::Value root;   // will contains the root value after parsing.
  Json::Reader reader;
  std::string sTXResName, sResName;
  std::set<std::string> listResources;

  bool parsingSuccessful = reader.parse(strJSON, root );
  if ( !parsingSuccessful )
  {
    CLog::Log(logERROR, "JSONHandler: Parse resource: no valid JSON data");
    return listResources;
  }

  for(Json::ValueIterator itr = root.begin() ; itr != root.end() ; itr++)
  {
    Json::Value valu = *itr;
    sTXResName = valu.get("slug", "unknown").asString();

    if (sTXResName.size() == 0 || sTXResName == "unknown")
      CLog::Log(logERROR, "JSONHandler: Parse resource: no valid JSON data while iterating");

    //TODO check if a resource only exists at transifex and warn user
    if ((sResName = GetResNameFromTXResName(sTXResName)) != "")
      listResources.insert(sResName);
  };
  return listResources;
};

std::string CProjectHandler::GetResNameFromTXResName(const std::string& strTXResName)
{
  for (T_itResData itResdata = m_mapResData.begin(); itResdata != m_mapResData.end(); itResdata++)
  {
    if (itResdata->second.TRX.ResName == strTXResName)
      return itResdata->first;
  }
  return "";
}

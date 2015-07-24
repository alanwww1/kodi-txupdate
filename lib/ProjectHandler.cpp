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
#include "UpdateXMLHandler.h"
#include "Log.h"
#include "CharsetUtils/CharsetUtils.h"
#include "FileUtils/FileUtils.h"
#include "Langcodes.h"

CProjectHandler::CProjectHandler()
{};

CProjectHandler::~CProjectHandler()
{};

void CProjectHandler::LoadUpdXMLToMem()
{
  CUpdateXMLHandler UpdateXMLHandler;
//  UpdateXMLHandler.LoadUpdXMLToMem (m_strProjDir, m_mapResData);
  UpdateXMLHandler.LoadResDataToMem(m_strProjDir, m_mapResData);
  g_HTTPHandler.SetHTTPCacheExpire(m_mapResData.begin()->second.iCacheExpire);
}

bool CProjectHandler::FetchResourcesFromTransifex()
{

  g_HTTPHandler.Cleanup();
  g_HTTPHandler.ReInit();
  printf ("TXresourcelist");

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

  printf ("\n\n");

  std::set<std::string> listResNamesAvailOnTX = ParseResources(strtemp);
//TODO collect out txprojectnames, check all resources in them, if we need to download
  for (T_itResData it = m_mapResData.begin(); it != m_mapResData.end(); it++)
  {
    const std::string& sResName = it->second.sResName;

    printf("%s%s%s (", KMAG, sResName.c_str(), RESET);
    CLog::Log(logLINEFEED, "");
    CLog::Log(logINFO, "ProjHandler: ****** FETCH Resource from TRANSIFEX: %s", sResName.c_str());
    CLog::IncIdent(4);

    if (listResNamesAvailOnTX.find(sResName) == listResNamesAvailOnTX.end())
    {
      printf(" ) Not yet available at Transifex\n");
      continue;
    }

    const CXMLResdata& XMLResData = m_mapResData[sResName];
    CResourceHandler NewResHandler(XMLResData);

    g_HTTPHandler.SetHTTPCacheExpire(XMLResData.iCacheExpire);

    m_mapResources[sResName] = NewResHandler;
    m_mapResources[sResName].FetchPOFilesTXToMem();
    CLog::DecIdent(4);
    printf(" )\n");
  }
  return true;
};

bool CProjectHandler::FetchResourcesFromUpstream()
{
  for (std::map<std::string, CXMLResdata>::iterator it = m_mapResData.begin(); it != m_mapResData.end(); it++)
  {
    const CXMLResdata& XMLResData = it->second;
    const std::string& sResName = it->first;

    printf("%s%s%s (", KMAG, sResName.c_str(), RESET);
    CLog::Log(logLINEFEED, "");
    CLog::Log(logINFO, "ProjHandler: ****** FETCH Resource from UPSTREAM: %s ******", sResName.c_str());

    CLog::IncIdent(4);

    if (m_mapResources.find(sResName) == m_mapResources.end()) // if it was not created in the map (by TX pull), make a new entry
    {
      CResourceHandler NewResHandler(XMLResData);
      m_mapResources[sResName] = NewResHandler;
    }

    g_HTTPHandler.SetHTTPCacheExpire(XMLResData.iCacheExpire);

    m_mapResources[sResName].FetchPOFilesUpstreamToMem();
    CLog::DecIdent(4);
    printf(" )\n");
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
    const CXMLResdata& XMLResData = m_mapResData[sResName];

    CLog::Log(logLINEFEED, "");
    CLog::Log(logINFO, "ProjHandler: *** Write Merged Resource: %s ***", itmapResources->first.c_str());
    CLog::IncIdent(4);

    std::string sMergedLangDir = XMLResData.sProjRootDir + DirSepChar + XMLResData.sMRGLFilesDir + DirSepChar;
    std::string sAddonXMLPath = sMergedLangDir + XMLResData.LOC.AXMLURL;
    std::string sChangeLogPath =  sMergedLangDir + XMLResData.LOC.ChLogURL;
    std::string sLangPath  = sMergedLangDir + XMLResData.LOC.LURL;
    std::string sLangAddonXMLPath = sMergedLangDir + XMLResData.LOC.AXMLURL;
    ResHandler.GenerateMergedPOFiles ();
    ResHandler.WriteMergedPOFiles (sAddonXMLPath, sLangAddonXMLPath, sChangeLogPath, sLangPath);

    std::string sPathUpdate = XMLResData.sProjRootDir + XMLResData.sUPDLFilesDir + DirSepChar + XMLResData.sResName + DirSepChar + XMLResData.sBaseLForm + DirSepChar + "strings.po";
    ResHandler.GenerateUpdatePOFiles ();

   g_HTTPHandler.SetHTTPCacheExpire(XMLResData.iCacheExpire);

    ResHandler.WriteUpdatePOFiles (sPathUpdate);


    CLog::DecIdent(4);
  }
  printf ("\n\n");
  return true;
};

bool CProjectHandler::CreateMergedResources()
{

  CLog::Log(logINFO, "CreateMergedResources started");

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
  printf ("\n");
  do
  {
    printf ("%sChoose option:%s %s0%s:Continue with update Transifex %s1%s:VIM UPD files %s2%s:VIM MRG files %sx%s:Exit. Your Choice:   \b\b", KRED, RESET, KMAG, RESET, KMAG, RESET, KMAG, RESET, KRED, RESET);
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
    else if (charInput == 'x')
      CLog::Log(logERROR, "Updating Transifex with new files aborted by user.");

    printf ("\e[A");
  }
  while (charInput != '0');

  printf ("                                                                                                                                        \n\e[A");

  g_HTTPHandler.Cleanup();
  g_HTTPHandler.ReInit();
  printf ("TXresourcelist");

  //TODO
  const std::string& strTargetProjectName = m_mapResData.begin()->second.UPD.ProjectName;

  //TODO ditry fix for always getting a fresh txlist here
  g_HTTPHandler.SetSkipCache(true);

  std::string strtemp = g_HTTPHandler.GetURLToSTR("https://www.transifex.com/api/2/project/" + strTargetProjectName + "/resources/");
  if (strtemp.empty())
    CLog::Log(logERROR, "ProjectHandler::FetchResourcesFromTransifex: error getting resources from transifex.net");

  g_HTTPHandler.SetSkipCache(false);
  printf ("\n\n");

  std::set<std::string> lResourcesAtTX = ParseResources(strtemp);

  g_HTTPHandler.Cleanup();
  g_HTTPHandler.ReInit();

  for (T_itmapRes it = m_mapResources.begin(); it != m_mapResources.end(); it++)
  {
    CResourceHandler& ResHandler = it->second;
    const std::string& sResName = it->first;
    const CXMLResdata& XMLResData = m_mapResData[sResName];

    g_HTTPHandler.SetHTTPCacheExpire(XMLResData.iCacheExpire);

    ResHandler.UploadResourceToTransifex(lResourcesAtTX.find(sResName) == lResourcesAtTX.end());
  }

  return;
}

void CProjectHandler::MigrateTranslators()
{
  //TODO
  CXMLResdata XMLResdata = m_mapResData.begin()->second;
  const std::string& strProjectName = XMLResdata.TRX.ProjectName;
  const std::string& strTargetProjectName = XMLResdata.UPD.ProjectName;
  const std::string& strTargetTXLangFormat = XMLResdata.UPD.LForm;

  if (strProjectName.empty() || strTargetProjectName.empty() || strProjectName == strTargetProjectName)
    CLog::Log(logERROR, "Cannot tranfer translators database. Wrong projectname and/or target projectname,");

  std::map<std::string, std::string> mapCoordinators, mapReviewers, mapTranslators;

  printf("\n%sCoordinators:%s\n", KGRN, RESET);
  mapCoordinators = g_LCodeHandler.GetTranslatorsDatabase("coordinators", strProjectName, XMLResdata);

  printf("\n%sReviewers:%s\n", KGRN, RESET);
  mapReviewers = g_LCodeHandler.GetTranslatorsDatabase("reviewers", strProjectName, XMLResdata);

  printf("\n%sTranslators:%s\n", KGRN, RESET);
  mapTranslators = g_LCodeHandler.GetTranslatorsDatabase("translators", strProjectName, XMLResdata);

  printf("\n%s", KGRN);
  printf("-----------------------------\n");
  printf("PUSH TRANSLATION GROUPS TO TX\n");
  printf("-----------------------------%s\n", RESET);

  g_LCodeHandler.UploadTranslatorsDatabase(mapCoordinators, mapReviewers, mapTranslators, strTargetProjectName, strTargetTXLangFormat);
};

void CProjectHandler::InitLCodeHandler()
{
//TODO
  CXMLResdata XMLResdata = m_mapResData.begin()->second;
  g_LCodeHandler.Init(XMLResdata.sLDatabaseURL, XMLResdata);
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
  for (T_itResData itXMLResdata = m_mapResData.begin(); itXMLResdata != m_mapResData.end(); itXMLResdata++)
  {
    if (itXMLResdata->second.TRX.ResName == strTXResName)
      return itXMLResdata->first;
  }
  return "";
}

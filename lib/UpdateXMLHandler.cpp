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

#include "UpdateXMLHandler.h"
#include "Log.h"
#include <stdlib.h>
#include <sstream>
#include "HTTPUtils.h"
#include "CharsetUtils/CharsetUtils.h"
#include "FileUtils/FileUtils.h"


using namespace std;

CXMLResdata::CXMLResdata()
{
 iMinComplPercent = 40;
 iCacheExpire = 60;
 bForceComm = false;
 bRebrand = false;
 bForceTXUpd = false;
 bHasOnlyAddonXML = false;
 bIsLangAddon = false;
}

CXMLResdata::~CXMLResdata()
{}

CUpdateXMLHandler::CUpdateXMLHandler()
{};

CUpdateXMLHandler::~CUpdateXMLHandler()
{};

void CUpdateXMLHandler::SubstituteExternalVariables(std::string& sVal)
{
  size_t iCurrPos = 0;
  size_t iNextPos = 0;
  while ((iNextPos = sVal.find('$', iCurrPos)) != std::string::npos)
  {
    // If the char is at the end of string, or if it is an internal var like $(LCODE), skip
    if (iNextPos + 1 == sVal.size() || sVal.at(iNextPos +1) == '(')
    {
      iCurrPos = iNextPos +1;
      continue;
    }
    size_t iVarLength = 1;
    while (iVarLength + iNextPos < sVal.size())
    {
      size_t iMatchedEntries = FindVariable(sVal.substr(iNextPos+1, iVarLength));
      if (iMatchedEntries == std::string::npos)
        CLog::Log(logERROR, "Undefined variable in value: %s", sVal.c_str());
      if (iMatchedEntries == 0)
        break;
      iVarLength++;
    }

    if ((iVarLength + iNextPos) == sVal.size())
      CLog::Log(logERROR, "Undefined variable in value: %s", sVal.c_str());

    std::string sVarToReplace = sVal.substr(iNextPos+1, iVarLength);
    std::string sReplaceString = m_MapOfVariables.at(sVarToReplace);
    sVal.replace(iNextPos,iVarLength +1, sReplaceString);

    iCurrPos = iNextPos + sReplaceString.size();
  }
}

size_t CUpdateXMLHandler::FindVariable(const std::string& sVar)
{
  size_t iMatchedEntries = 0;
  bool bExactMatchFound = false;

  for (std::map<std::string, std::string>::iterator it = m_MapOfVariables.begin(); it != m_MapOfVariables.end(); it++)
  {
    if (it->first.find(sVar) != std::string::npos)
    {
      iMatchedEntries++;
      if (sVar.size() == it->first.size())
        bExactMatchFound = true;
    }
  }
  if (iMatchedEntries == 0)
    return std::string::npos;
  if (iMatchedEntries == 1 && bExactMatchFound)
    return 0;
  return iMatchedEntries;
}

void CUpdateXMLHandler::SetExternalVariables(const std::string& sLine)
{
  size_t iPosVar1 = 0;
  size_t iPosVar2 = sLine.find(" = ", iPosVar1);

  if (iPosVar2 == std::string::npos)
    CLog::Log(logERROR, "ConfHandler: Wrong line in conf file. variable = value format is wrong for line:\n%s", sLine.c_str());

  std::string sVar = sLine.substr(iPosVar1, iPosVar2-iPosVar1);

  std::string sVal = g_CharsetUtils.UnescapeCPPString(sLine.substr(iPosVar2 + 3));

  SubstituteExternalVariables(sVal);

  m_MapOfVariables[sVar] = sVal;
}

void CUpdateXMLHandler::SetInternalVariables(const std::string& sLine, CXMLResdata& ResData)
{
  size_t iPosVar1 = 4;
  size_t iPosVar2 = sLine.find(" = ", iPosVar1);

  if (iPosVar2 == std::string::npos)
    CLog::Log(logERROR, "ConfHandler: Wrong line in conf file. set variable = value format is wrong for line:\n%s", sLine.c_str());

  std::string sVar = sLine.substr(iPosVar1, iPosVar2-iPosVar1);

  std::string sVal = g_CharsetUtils.UnescapeCPPString(sLine.substr(iPosVar2 + 3));

  SubstituteExternalVariables(sVal);

  return SetInternalVariable(sVar, sVal, ResData);
}

void CUpdateXMLHandler::ClearVariables(const std::string& sLine, CXMLResdata& ResData)
{
  std::string sVar = sLine.substr(6);

  if (sVar.empty())
    CLog::Log(logERROR, "ConfHandler: Wrong line in conf file. Clear variable name is empty.");

  if (sVar.find('*') != sVar.size()-1)
    SetInternalVariable(sVar, "", ResData);

  //We clear variables that has a match at the begining with our string
  sVar = sVar.substr(0,sVar.size()-1);

  for (std::map<std::string, std::string>::iterator it = m_MapOfVariables.begin(); it != m_MapOfVariables.end(); it++)
  {
    if (it->first.find(sVar) == 0)
      SetInternalVariable(it->first, "", ResData);
  }
}

void CUpdateXMLHandler::SetInternalVariable(const std::string& sVar, const std::string sVal, CXMLResdata& ResData)
{
  if (sVar == "UPSOwner")                   ResData.UPS.Owner = sVal;
  else if (sVar == "UPSRepo")               ResData.UPS.Repo = sVal;
  else if (sVar == "UPSBranch")             ResData.UPS.Branch = sVal;

  else if (sVar == "UPSLpath")              ResData.UPS.LPath = sVal;
  else if (sVar == "UPSLForm")              ResData.UPS.LForm = sVal;
  else if (sVar == "UPSAXMLPath")           ResData.UPS.AXMLPath = sVal;
  else if (sVar == "UPSLFormInAXML")        ResData.UPS.LFormInAXML = sVal;
  else if (sVar == "UPSLAXMLPath")          ResData.UPS.LAXMLPath = sVal;
  else if (sVar == "UPSLAXMLFormat")        ResData.UPS.LAXMLForm = sVal;
  else if (sVar == "UPSChLogPath")          ResData.UPS.ChLogPath = sVal;

  else if (sVar == "LOCOwner")              ResData.LOC.Owner = sVal;
  else if (sVar == "LOCRepo")               ResData.LOC.Repo = sVal;
  else if (sVar == "LOCBranch")             ResData.LOC.Branch = sVal;

  else if (sVar == "LOCLpath")              ResData.LOC.LPath = sVal;
  else if (sVar == "LOCLForm")              ResData.LOC.LForm = sVal;
  else if (sVar == "LOCAXMLPath")           ResData.LOC.AXMLPath = sVal;
  else if (sVar == "LOCLFormInAXML")        ResData.LOC.LFormInAXML = sVal;
  else if (sVar == "LOCLAXMLPath")          ResData.LOC.LAXMLPath = sVal;
  else if (sVar == "LOCLAXMLFormat")        ResData.LOC.LAXMLForm = sVal;
  else if (sVar == "LOCChLogPath")          ResData.LOC.ChLogPath = sVal;

  else if (sVar == "UPSSRCOwner")           ResData.UPSSRC.Owner = sVal;
  else if (sVar == "UPSSRCRepo")            ResData.UPSSRC.Repo = sVal;
  else if (sVar == "UPSSRCBranch")          ResData.UPSSRC.Branch = sVal;

  else if (sVar == "UPSSRCLpath")           ResData.UPSSRC.LPath = sVal;
  else if (sVar == "UPSSRCLForm")           ResData.UPSSRC.LForm = sVal;
  else if (sVar == "UPSSRCAXMLPath")        ResData.UPSSRC.AXMLPath = sVal;
  else if (sVar == "UPSSRCLFormInAXML")     ResData.UPSSRC.LFormInAXML = sVal;
  else if (sVar == "UPSSRCLAXMLPath")       ResData.UPSSRC.LAXMLPath = sVal;
  else if (sVar == "UPSSRCLAXMLFormat")     ResData.UPSSRC.LAXMLForm = sVal;
  else if (sVar == "UPSSRCChLogPath")       ResData.UPSSRC.ChLogPath = sVal;

  else if (sVar == "LOCSRCOwner")           ResData.LOCSRC.Owner = sVal;
  else if (sVar == "LOCSRCRepo")            ResData.LOCSRC.Repo = sVal;
  else if (sVar == "LOCSRCBranch")          ResData.LOCSRC.Branch = sVal;

  else if (sVar == "LOCSRCLpath")           ResData.LOCSRC.LPath = sVal;
  else if (sVar == "LOCSRCLForm")           ResData.LOCSRC.LForm = sVal;
  else if (sVar == "LOCSRCAXMLPath")        ResData.LOCSRC.AXMLPath = sVal;
  else if (sVar == "LOCSRCLFormInAXML")     ResData.LOCSRC.LFormInAXML = sVal;
  else if (sVar == "LOCSRCLAXMLPath")       ResData.LOCSRC.LAXMLPath = sVal;
  else if (sVar == "LOCSRCLAXMLFormat")     ResData.LOCSRC.LAXMLForm = sVal;
  else if (sVar == "LOCSRCChLogPath")       ResData.LOCSRC.ChLogPath = sVal;

//To be deleted
  else if (sVar == "UPSLURL")               ResData.UPS.LURL = sVal;
  else if (sVar == "UPSLURLRoot")           ResData.UPS.LURLRoot = sVal;
  else if (sVar == "UPSAXMLURL")            ResData.UPS.AXMLURL = sVal;
  else if (sVar == "UPSAXMLURLRoot")        ResData.UPS.AXMLURLRoot = sVal;
  else if (sVar == "UPSALForm")             ResData.UPS.ALForm = sVal;
  else if (sVar == "UPSAXMLFileName")       ResData.UPS.AXMLFileName = sVal;
  else if (sVar == "UPSChLogURL")           ResData.UPS.ChLogURL = sVal;
  else if (sVar == "UPSChLogURLRoot")       ResData.UPS.ChLogURLRoot = sVal;
  else if (sVar == "UPSChLogName")          ResData.UPS.ChLogName = sVal;

  else if (sVar == "LOCLURL")               ResData.LOC.LURL = sVal;
  else if (sVar == "LOCLURLRoot")           ResData.LOC.LURLRoot = sVal;
  else if (sVar == "LOCAXMLURL")            ResData.LOC.AXMLURL = sVal;
  else if (sVar == "LOCAXMLURLRoot")        ResData.LOC.AXMLURLRoot = sVal;
  else if (sVar == "LOCALForm")             ResData.LOC.ALForm = sVal;
  else if (sVar == "LOCAXMLFileName")       ResData.LOC.AXMLFileName = sVal;
  else if (sVar == "LOCChLogURL")           ResData.LOC.ChLogURL = sVal;
  else if (sVar == "LOCChLogURLRoot")       ResData.LOC.ChLogURLRoot = sVal;
  else if (sVar == "LOCChLogName")          ResData.LOC.ChLogName = sVal;

  else if (sVar == "UPSSRCLURL")            ResData.UPSSRC.LURL = sVal;
  else if (sVar == "UPSSRCLURLRoot")        ResData.UPSSRC.LURLRoot = sVal;
  else if (sVar == "UPSSRCAXMLURL")         ResData.UPSSRC.AXMLURL = sVal;
  else if (sVar == "UPSSRCAXMLURLRoot")     ResData.UPSSRC.AXMLURLRoot = sVal;
  else if (sVar == "UPSSRCALForm")          ResData.UPSSRC.ALForm = sVal;
  else if (sVar == "UPSSRCAXMLFileName")    ResData.UPSSRC.AXMLFileName = sVal;
  else if (sVar == "UPSSRCChLogURL")        ResData.UPSSRC.ChLogURL = sVal;
  else if (sVar == "UPSSRCChLogURLRoot")    ResData.UPSSRC.ChLogURLRoot = sVal;
  else if (sVar == "UPSSRCChLogName")       ResData.UPSSRC.ChLogName = sVal;

  else if (sVar == "LOCSRCLURL")            ResData.LOCSRC.LURL = sVal;
  else if (sVar == "LOCSRCLURLRoot")        ResData.LOCSRC.LURLRoot = sVal;
  else if (sVar == "LOCSRCAXMLURL")         ResData.LOCSRC.AXMLURL = sVal;
  else if (sVar == "LOCSRCAXMLURLRoot")     ResData.LOCSRC.AXMLURLRoot = sVal;
  else if (sVar == "LOCSRCALForm")          ResData.LOCSRC.ALForm = sVal;
  else if (sVar == "LOCSRCAXMLFileName")    ResData.LOCSRC.AXMLFileName = sVal;
  else if (sVar == "LOCSRCChLogURL")        ResData.LOCSRC.ChLogURL = sVal;
  else if (sVar == "LOCSRCChLogURLRoot")    ResData.LOCSRC.ChLogURLRoot = sVal;
  else if (sVar == "LOCSRCChLogName")       ResData.LOCSRC.ChLogName = sVal;

//

  else if (sVar == "TRXProjectName")        ResData.TRX.ProjectName = sVal;
  else if (sVar == "TRXLongProjectName")    ResData.TRX.LongProjectName = sVal;
  else if (sVar == "TRXResName")            ResData.TRX.ResName = sVal;
  else if (sVar == "TRXLForm")              ResData.TRX.LForm = sVal;

  else if (sVar == "UPDProjectName")        ResData.UPD.ProjectName = sVal;
  else if (sVar == "UPDLongProjectName")    ResData.UPD.LongProjectName = sVal;
  else if (sVar == "UPDResName")            ResData.UPD.ResName = sVal;
  else if (sVar == "UPDLForm")              ResData.UPD.LForm = sVal;


  else if (sVar == "ResName")               ResData.sResName = sVal;
  else if (sVar == "ChgLogFormat")          ResData.sChgLogFormat = sVal;
//else if (sVar == "ProjRootDir")           ResData.sProjRootDir = sVal;
  else if (sVar == "MRGLFilesDir")          ResData.sMRGLFilesDir = sVal;
  else if (sVar == "UPDLFilesDir")          ResData.sUPDLFilesDir = sVal;
  else if (sVar == "SupportEmailAddr")      ResData.sSupportEmailAddr = sVal;
  else if (sVar == "SRCLCode")              ResData.sSRCLCode = sVal;
  else if (sVar == "BaseLForm")             ResData.sBaseLForm = sVal;
  else if (sVar == "LTeamLFormat")          ResData.sLTeamLFormat = sVal;
  else if (sVar == "LDatabaseURL")          ResData.sLDatabaseURL = sVal;
  else if (sVar == "MinComplPercent")       ResData.iMinComplPercent = strtol(&sVal[0], NULL, 10);
  else if (sVar == "CacheExpire")           ResData.iCacheExpire = strtol(&sVal[0], NULL, 10);
  else if (sVar == "ForceComm")             ResData.bForceComm = (sVal == "true");
  else if (sVar == "Rebrand")               ResData.bRebrand = (sVal == "true");
  else if (sVar == "ForceTXUpd")            ResData.bForceTXUpd = (sVal == "true");
  else if (sVar == "IsLangAddon")           ResData.bIsLangAddon = (sVal == "true");
  else if (sVar == "HasOnlyAddonXML")       ResData.bHasOnlyAddonXML = (sVal == "true");

  else
    CLog::Log(logERROR, "ConfHandler: Unreconised internal variable name: \"%s\"", sVar.c_str());

  m_MapOfVariables[sVar] = sVal;
}

void CUpdateXMLHandler::LoadResDataToMem (std::string rootDir, std::map<std::string, CXMLResdata> & mapResData, std::map<std::string, CGITData> * pMapGitRepos)
{
  std::string sConfFile = g_File.ReadFileToStr(rootDir + "kodi-txupdate.conf");
  if (sConfFile == "")
    CLog::Log(logERROR, "Confhandler: erroe: missing conf file.");

  size_t iPos1 = 0;
  size_t iPos2 = 0;

  CXMLResdata ResData;

  ResData.sProjRootDir = rootDir;
  ResData.m_pMapGitRepos = pMapGitRepos;


  while ((iPos2 = sConfFile.find('\n', iPos1)) != std::string::npos)
  {
    std::string sLine = sConfFile.substr(iPos1, iPos2-iPos1);
    iPos1 = iPos2 +1;

    if (sLine.empty() || sLine.find('#') == 0) // If line is empty or a comment line, ignore
      continue;

    if (sLine.find("set ") == 0)
      SetInternalVariables(sLine, ResData);
    else if (sLine.find("clear ") == 0)
      ClearVariables(sLine, ResData);
    else if (sLine.find("create resource ") == 0)
      CreateResource(ResData, sLine, mapResData);
    else
      SetExternalVariables(sLine);
  }
}

std::string CUpdateXMLHandler::ReplaceResName(std::string sVal, const CXMLResdata& ResData)
{
  g_CharsetUtils.replaceAllStrParts (&sVal, "$(RESNAME)", ResData.sResName);
  g_CharsetUtils.replaceAllStrParts (&sVal, "$(TRXRESNAME)", ResData.TRX.ResName);
  return sVal;
}

void CUpdateXMLHandler::CreateResource(CXMLResdata& ResData, const std::string& sLine, std::map<std::string, CXMLResdata> & mapResData)
{
  CXMLResdata ResDataToStore;

  //Parse the resource names
  size_t posResName, posTRXResName;
  if ((posResName = sLine.find("ResName = ")) == std::string::npos)
    CLog::Log(logERROR, "Confhandler: Cannot create resource, missing ResName");
  posResName = posResName +10;

  ResDataToStore.sResName = sLine.substr(posResName, sLine.find(' ', posResName)-posResName);

  if ((posTRXResName = sLine.find("TRXResName = ")) == std::string::npos)
    CLog::Log(logERROR, "Confhandler: Cannot create resource, missing TRXResName");
  posTRXResName = posTRXResName +13;

  ResDataToStore.TRX.ResName = sLine.substr(posTRXResName, sLine.find('\n', posTRXResName)-posTRXResName);


  ResDataToStore.UPS.Owner            = ReplaceResName(ResData.UPS.Owner, ResDataToStore);
  ResDataToStore.UPS.Repo             = ReplaceResName(ResData.UPS.Repo, ResDataToStore);
  ResDataToStore.UPS.Branch           = ReplaceResName(ResData.UPS.Branch, ResDataToStore);

  ResDataToStore.UPS.LPath            = ReplaceResName(ResData.UPS.LPath, ResDataToStore);
  ResDataToStore.UPS.LForm            = ReplaceResName(ResData.UPS.LForm, ResDataToStore);
  ResDataToStore.UPS.AXMLPath         = ReplaceResName(ResData.UPS.AXMLPath, ResDataToStore);
  ResDataToStore.UPS.LFormInAXML      = ReplaceResName(ResData.UPS.LFormInAXML, ResDataToStore);
  ResDataToStore.UPS.LAXMLPath        = ReplaceResName(ResData.UPS.LAXMLPath, ResDataToStore);
  ResDataToStore.UPS.LAXMLForm        = ReplaceResName(ResData.UPS.LAXMLForm, ResDataToStore);
  ResDataToStore.UPS.ChLogPath        = ReplaceResName(ResData.UPS.ChLogPath, ResDataToStore);

  ResDataToStore.LOC.Owner            = ReplaceResName(ResData.LOC.Owner, ResDataToStore);
  ResDataToStore.LOC.Repo             = ReplaceResName(ResData.LOC.Repo, ResDataToStore);
  ResDataToStore.LOC.Branch           = ReplaceResName(ResData.LOC.Branch, ResDataToStore);

  ResDataToStore.LOC.LPath            = ReplaceResName(ResData.LOC.LPath, ResDataToStore);
  ResDataToStore.LOC.LForm            = ReplaceResName(ResData.LOC.LForm, ResDataToStore);
  ResDataToStore.LOC.AXMLPath         = ReplaceResName(ResData.LOC.AXMLPath, ResDataToStore);
  ResDataToStore.LOC.LFormInAXML      = ReplaceResName(ResData.LOC.LFormInAXML, ResDataToStore);
  ResDataToStore.LOC.LAXMLPath        = ReplaceResName(ResData.LOC.LAXMLPath, ResDataToStore);
  ResDataToStore.LOC.LAXMLForm        = ReplaceResName(ResData.LOC.LAXMLForm, ResDataToStore);
  ResDataToStore.LOC.ChLogPath        = ReplaceResName(ResData.LOC.ChLogPath, ResDataToStore);

  ResDataToStore.UPSSRC.Owner         = ReplaceResName(ResData.UPSSRC.Owner, ResDataToStore);
  ResDataToStore.UPSSRC.Repo          = ReplaceResName(ResData.UPSSRC.Repo, ResDataToStore);
  ResDataToStore.UPSSRC.Branch        = ReplaceResName(ResData.UPSSRC.Branch, ResDataToStore);

  ResDataToStore.UPSSRC.LPath         = ReplaceResName(ResData.UPSSRC.LPath, ResDataToStore);
  ResDataToStore.UPSSRC.LForm         = ReplaceResName(ResData.UPSSRC.LForm, ResDataToStore);
  ResDataToStore.UPSSRC.AXMLPath      = ReplaceResName(ResData.UPSSRC.AXMLPath, ResDataToStore);
  ResDataToStore.UPSSRC.LFormInAXML   = ReplaceResName(ResData.UPSSRC.LFormInAXML, ResDataToStore);
  ResDataToStore.UPSSRC.LAXMLPath     = ReplaceResName(ResData.UPSSRC.LAXMLPath, ResDataToStore);
  ResDataToStore.UPSSRC.LAXMLForm     = ReplaceResName(ResData.UPSSRC.LAXMLForm, ResDataToStore);
  ResDataToStore.UPSSRC.ChLogPath     = ReplaceResName(ResData.UPSSRC.ChLogPath, ResDataToStore);

  ResDataToStore.LOCSRC.Owner         = ReplaceResName(ResData.LOCSRC.Owner, ResDataToStore);
  ResDataToStore.LOCSRC.Repo          = ReplaceResName(ResData.LOCSRC.Repo, ResDataToStore);
  ResDataToStore.LOCSRC.Branch        = ReplaceResName(ResData.LOCSRC.Branch, ResDataToStore);

  ResDataToStore.LOCSRC.LPath         = ReplaceResName(ResData.LOCSRC.LPath, ResDataToStore);
  ResDataToStore.LOCSRC.LForm         = ReplaceResName(ResData.LOCSRC.LForm, ResDataToStore);
  ResDataToStore.LOCSRC.AXMLPath      = ReplaceResName(ResData.LOCSRC.AXMLPath, ResDataToStore);
  ResDataToStore.LOCSRC.LFormInAXML   = ReplaceResName(ResData.LOCSRC.LFormInAXML, ResDataToStore);
  ResDataToStore.LOCSRC.LAXMLPath     = ReplaceResName(ResData.LOCSRC.LAXMLPath, ResDataToStore);
  ResDataToStore.LOCSRC.LAXMLForm     = ReplaceResName(ResData.LOCSRC.LAXMLForm, ResDataToStore);
  ResDataToStore.LOCSRC.ChLogPath     = ReplaceResName(ResData.LOCSRC.ChLogPath, ResDataToStore);

//To be deleted
  ResDataToStore.UPS.LURL             = ReplaceResName(ResData.UPS.LURL, ResDataToStore);
  ResDataToStore.UPS.LURLRoot         = ReplaceResName(ResData.UPS.LURLRoot, ResDataToStore);
  ResDataToStore.UPS.AXMLURL          = ReplaceResName(ResData.UPS.AXMLURL, ResDataToStore);
  ResDataToStore.UPS.AXMLURLRoot      = ReplaceResName(ResData.UPS.AXMLURLRoot, ResDataToStore);
  ResDataToStore.UPS.ALForm           = ReplaceResName(ResData.UPS.ALForm, ResDataToStore);
  ResDataToStore.UPS.AXMLFileName     = ReplaceResName(ResData.UPS.AXMLFileName, ResDataToStore);
  ResDataToStore.UPS.ChLogURL         = ReplaceResName(ResData.UPS.ChLogURL, ResDataToStore);
  ResDataToStore.UPS.ChLogURLRoot     = ReplaceResName(ResData.UPS.ChLogURLRoot, ResDataToStore);
  ResDataToStore.UPS.ChLogName        = ReplaceResName(ResData.UPS.ChLogName, ResDataToStore);

  ResDataToStore.LOC.LURL             = ReplaceResName(ResData.LOC.LURL, ResDataToStore);
  ResDataToStore.LOC.LURLRoot         = ReplaceResName(ResData.LOC.LURLRoot, ResDataToStore);
  ResDataToStore.LOC.AXMLURL          = ReplaceResName(ResData.LOC.AXMLURL, ResDataToStore);
  ResDataToStore.LOC.AXMLURLRoot      = ReplaceResName(ResData.LOC.AXMLURLRoot, ResDataToStore);
  ResDataToStore.LOC.ALForm           = ReplaceResName(ResData.LOC.ALForm, ResDataToStore);
  ResDataToStore.LOC.AXMLFileName     = ReplaceResName(ResData.LOC.AXMLFileName, ResDataToStore);
  ResDataToStore.LOC.ChLogURL         = ReplaceResName(ResData.LOC.ChLogURL, ResDataToStore);
  ResDataToStore.LOC.ChLogURLRoot     = ReplaceResName(ResData.LOC.ChLogURLRoot, ResDataToStore);
  ResDataToStore.LOC.ChLogName        = ReplaceResName(ResData.LOC.ChLogName, ResDataToStore);

  ResDataToStore.UPSSRC.LURL          = ReplaceResName(ResData.UPSSRC.LURL, ResDataToStore);
  ResDataToStore.UPSSRC.LURLRoot      = ReplaceResName(ResData.UPSSRC.LURLRoot, ResDataToStore);
  ResDataToStore.UPSSRC.AXMLURL       = ReplaceResName(ResData.UPSSRC.AXMLURL, ResDataToStore);
  ResDataToStore.UPSSRC.AXMLURLRoot   = ReplaceResName(ResData.UPSSRC.AXMLURLRoot, ResDataToStore);
  ResDataToStore.UPSSRC.ALForm        = ReplaceResName(ResData.UPSSRC.ALForm, ResDataToStore);
  ResDataToStore.UPSSRC.AXMLFileName  = ReplaceResName(ResData.UPSSRC.AXMLFileName, ResDataToStore);
  ResDataToStore.UPSSRC.ChLogURL      = ReplaceResName(ResData.UPSSRC.ChLogURL, ResDataToStore);
  ResDataToStore.UPSSRC.ChLogURLRoot  = ReplaceResName(ResData.UPSSRC.ChLogURLRoot, ResDataToStore);
  ResDataToStore.UPSSRC.ChLogName     = ReplaceResName(ResData.UPSSRC.ChLogName, ResDataToStore);

  ResDataToStore.LOCSRC.LURL          = ReplaceResName(ResData.LOCSRC.LURL, ResDataToStore);
  ResDataToStore.LOCSRC.LURLRoot      = ReplaceResName(ResData.LOCSRC.LURLRoot, ResDataToStore);
  ResDataToStore.LOCSRC.AXMLURL       = ReplaceResName(ResData.LOCSRC.AXMLURL, ResDataToStore);
  ResDataToStore.LOCSRC.AXMLURLRoot   = ReplaceResName(ResData.LOCSRC.AXMLURLRoot, ResDataToStore);
  ResDataToStore.LOCSRC.ALForm        = ReplaceResName(ResData.LOCSRC.ALForm, ResDataToStore);
  ResDataToStore.LOCSRC.AXMLFileName  = ReplaceResName(ResData.LOCSRC.AXMLFileName, ResDataToStore);
  ResDataToStore.LOCSRC.ChLogURL      = ReplaceResName(ResData.LOCSRC.ChLogURL, ResDataToStore);
  ResDataToStore.LOCSRC.ChLogURLRoot  = ReplaceResName(ResData.LOCSRC.ChLogURLRoot, ResDataToStore);
  ResDataToStore.LOCSRC.ChLogName     = ReplaceResName(ResData.LOCSRC.ChLogName, ResDataToStore);

//

  ResDataToStore.TRX.ProjectName      = ReplaceResName(ResData.TRX.ProjectName, ResDataToStore);
  ResDataToStore.TRX.LongProjectName  = ReplaceResName(ResData.TRX.LongProjectName, ResDataToStore);
//ResDataToStore.TRX.ResName          = ReplaceResName(ResData.TRX.ResName, ResDataToStore);
  ResDataToStore.TRX.LForm            = ReplaceResName(ResData.TRX.LForm, ResDataToStore);

  ResDataToStore.UPD.ProjectName      = ReplaceResName(ResData.UPD.ProjectName, ResDataToStore);
  ResDataToStore.UPD.LongProjectName  = ReplaceResName(ResData.UPD.LongProjectName, ResDataToStore);
  ResDataToStore.UPD.ResName          = ReplaceResName(ResData.UPD.ResName, ResDataToStore);
  ResDataToStore.UPD.LForm            = ReplaceResName(ResData.UPD.LForm, ResDataToStore);


//ResDataToStore.sResName             = ReplaceResName(ResData.sResName, ResDataToStore);
  ResDataToStore.sChgLogFormat        = ReplaceResName(ResData.sChgLogFormat, ResDataToStore);
  ResDataToStore.sProjRootDir         = ReplaceResName(ResData.sProjRootDir, ResDataToStore);
  ResDataToStore.sMRGLFilesDir        = ReplaceResName(ResData.sMRGLFilesDir, ResDataToStore);
  ResDataToStore.sUPDLFilesDir        = ReplaceResName(ResData.sUPDLFilesDir, ResDataToStore);
  ResDataToStore.sSupportEmailAddr    = ReplaceResName(ResData.sSupportEmailAddr, ResDataToStore);
  ResDataToStore.sSRCLCode            = ReplaceResName(ResData.sSRCLCode, ResDataToStore);
  ResDataToStore.sBaseLForm           = ReplaceResName(ResData.sBaseLForm, ResDataToStore);
  ResDataToStore.sLTeamLFormat        = ReplaceResName(ResData.sLTeamLFormat, ResDataToStore);
  ResDataToStore.sLDatabaseURL        = ReplaceResName(ResData.sLDatabaseURL, ResDataToStore);
  ResDataToStore.iMinComplPercent     = ResData.iMinComplPercent;
  ResDataToStore.iCacheExpire         = ResData.iCacheExpire;
  ResDataToStore.bForceComm           = ResData.bForceComm;
  ResDataToStore.bRebrand             = ResData.bRebrand;
  ResDataToStore.bForceTXUpd          = ResData.bForceTXUpd;
  ResDataToStore.bIsLangAddon         = ResData.bIsLangAddon;
  ResDataToStore.bHasOnlyAddonXML     = ResData.bHasOnlyAddonXML;
  ResDataToStore.m_pMapGitRepos       = ResData.m_pMapGitRepos;

  //If we don't have a different target trx resource name, use the source trx resource name
  if (ResDataToStore.UPD.ResName.empty())
    ResDataToStore.UPD.ResName = ResDataToStore.TRX.ResName;


  mapResData[ResDataToStore.sResName] = ResDataToStore;

  ResData.UPD.ResName.clear();
  ResDataToStore.m_pMapGitRepos->operator[](ResDataToStore.UPS.Owner + "/" + ResDataToStore.UPS.Repo + "/" + ResDataToStore.UPS.Branch) = ResDataToStore.UPS;
}

bool CUpdateXMLHandler::GetParamsFromURLorPath (string const &strURL, string &strLangFormat, string &strFileName,
                                                 string &strURLRoot, const char strSeparator)
{
  if (strURL.empty())
    return false;

  size_t pos0, posStart, posEnd;

  pos0 = strURL.find_last_of("$");
  if (((posStart = strURL.find("$("), pos0) != std::string::npos) && ((posEnd = strURL.find(")",posStart)) != std::string::npos))
    strLangFormat = strURL.substr(posStart, posEnd - posStart +1);

  return GetParamsFromURLorPath (strURL, strFileName, strURLRoot, strSeparator);
}

bool CUpdateXMLHandler::GetParamsFromURLorPath (string const &strURL, string &strFileName,
                                                 string &strURLRoot, const char strSeparator)
{
  if (strURL.empty())
    return false;

  if (strURL.find(strSeparator) == std::string::npos)
    return false;

  strFileName = strURL.substr(strURL.find_last_of(strSeparator)+1);
  strURLRoot = g_CharsetUtils.GetRoot(strURL, strFileName);
  return true;
}
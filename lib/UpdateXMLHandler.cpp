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
 bForceGitDloadToCache = false;
 bSkipGitReset = false;
 bSkipVersionBump = false;
 bHasOnlyAddonXML = false;
 bIsLangAddon = false;
}

CXMLResdata::~CXMLResdata()
{}

CUpdateXMLHandler::CUpdateXMLHandler()
{};

CUpdateXMLHandler::~CUpdateXMLHandler()
{};

void CUpdateXMLHandler::SubstituteExternalVariables(std::string& sVal, bool bIgnoreMissing)
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
      {
        if (!bIgnoreMissing)
          CLog::Log(logERROR, "Undefined variable in value: %s", sVal.c_str());
        sVal = "";
        return;
      }
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

  SubstituteExternalVariables(sVal, false);

  m_MapOfVariables[sVar] = sVal;
}

void CUpdateXMLHandler::SetInternalVariables(const std::string& sLine, CXMLResdata& ResData)
{
  size_t iPosVar1 = sLine.find(" ",0) +1;
  size_t iPosVar2 = sLine.find(" = ", iPosVar1);

  if (iPosVar2 == std::string::npos)
    CLog::Log(logERROR, "ConfHandler: Wrong line in conf file. set variable = value format is wrong for line:\n%s", sLine.c_str());

  std::string sVar = sLine.substr(iPosVar1, iPosVar2-iPosVar1);

  std::string sVal = g_CharsetUtils.UnescapeCPPString(sLine.substr(iPosVar2 + 3));

  std::string sVarDerived, sValDerived;
  if (sVar == "MRG" || sVar == "LOC" || sVar == "UPS" || sVar == "UPSSRC" || sVar == "LOCSRC")
  {
    //Examine if we have a simplified assign of variables, by only referring to the as groups like MRG, LOC, UPS etc.
    sValDerived = sVal + "Owner"; sVarDerived = sVar + "Owner";
    SubstituteExternalVariables(sValDerived, true); SetInternalVariable(sVarDerived, sValDerived, ResData, true);
    sValDerived = sVal + "Repo"; sVarDerived = sVar + "Repo";
    SubstituteExternalVariables(sValDerived, true); SetInternalVariable(sVarDerived, sValDerived, ResData, true);
    sValDerived = sVal + "Branch"; sVarDerived = sVar + "Branch";
    SubstituteExternalVariables(sValDerived, true); SetInternalVariable(sVarDerived, sValDerived, ResData, true);
    sValDerived = sVal + "LPath"; sVarDerived = sVar + "LPath";
    SubstituteExternalVariables(sValDerived, true); SetInternalVariable(sVarDerived, sValDerived, ResData, true);
    sValDerived = sVal + "AXMLPath"; sVarDerived = sVar + "AXMLPath";
    SubstituteExternalVariables(sValDerived, true); SetInternalVariable(sVarDerived, sValDerived, ResData, true);
    sValDerived = sVal + "LFormInAXML"; sVarDerived = sVar + "LFormInAXML";
    SubstituteExternalVariables(sValDerived, true); SetInternalVariable(sVarDerived, sValDerived, ResData, true);
    sValDerived = sVal + "ChLogPath"; sVarDerived = sVar + "ChLogPath";
    SubstituteExternalVariables(sValDerived, true); SetInternalVariable(sVarDerived, sValDerived, ResData, true);

    return;
  }

  if (sVar == "UPD" || sVar == "TRX")
  {
    //Examine if we have a simplified assing of variables, by only referring to the as groups like TRX, UPD etc.
    sValDerived = sVal + "ProjectName"; sVarDerived = sVar + "ProjectName";
    SubstituteExternalVariables(sValDerived, true); SetInternalVariable(sVarDerived, sValDerived, ResData, true);
    sValDerived = sVal + "LongProjectName"; sVarDerived = sVar + "LongProjectName";
    SubstituteExternalVariables(sValDerived, true); SetInternalVariable(sVarDerived, sValDerived, ResData, true);
    sValDerived = sVal + "LForm"; sVarDerived = sVar + "LForm";
    SubstituteExternalVariables(sValDerived, true); SetInternalVariable(sVarDerived, sValDerived, ResData, true);
    return;
  }

  SubstituteExternalVariables(sVal, false);

  return SetInternalVariable(sVar, sVal, ResData, false);
}

void CUpdateXMLHandler::ClearVariables(const std::string& sLine, CXMLResdata& ResData)
{
  std::string sVar = sLine.substr(6);

  if (sVar.empty())
    CLog::Log(logERROR, "ConfHandler: Wrong line in conf file. Clear variable name is empty.");

  if (sVar.find('*') != sVar.size()-1)
    SetInternalVariable(sVar, "", ResData, false);

  //We clear variables that has a match at the begining with our string
  sVar = sVar.substr(0,sVar.size()-1);

  for (std::map<std::string, std::string>::iterator it = m_MapOfVariables.begin(); it != m_MapOfVariables.end(); it++)
  {
    if (it->first.find(sVar) == 0)
      SetInternalVariable(it->first, "", ResData, false);
  }
}

void CUpdateXMLHandler::SetInternalVariable(const std::string& sVar, const std::string sVal, CXMLResdata& ResData, bool bIgnoreMissing)
{
  if (sVar == "UPSOwner")                   ResData.UPS.Owner = sVal;
  else if (sVar == "UPSRepo")               ResData.UPS.Repo = sVal;
  else if (sVar == "UPSBranch")             ResData.UPS.Branch = sVal;

  else if (sVar == "UPSLPath")              ResData.UPS.LPath = sVal;
//else if (sVar == "UPSLForm")              ResData.UPS.LForm = sVal;
  else if (sVar == "UPSAXMLPath")           ResData.UPS.AXMLPath = sVal;
  else if (sVar == "UPSLFormInAXML")        ResData.UPS.LFormInAXML = sVal;
//else if (sVar == "UPSLAXMLPath")          ResData.UPS.LAXMLPath = sVal;
//else if (sVar == "UPSLAXMLFormat")        ResData.UPS.LAXMLForm = sVal;
  else if (sVar == "UPSChLogPath")          ResData.UPS.ChLogPath = sVal;

  else if (sVar == "LOCOwner")              ResData.LOC.Owner = sVal;
  else if (sVar == "LOCRepo")               ResData.LOC.Repo = sVal;
  else if (sVar == "LOCBranch")             ResData.LOC.Branch = sVal;

  else if (sVar == "LOCLPath")              ResData.LOC.LPath = sVal;
//else if (sVar == "LOCLForm")              ResData.LOC.LForm = sVal;
  else if (sVar == "LOCAXMLPath")           ResData.LOC.AXMLPath = sVal;
  else if (sVar == "LOCLFormInAXML")        ResData.LOC.LFormInAXML = sVal;
//else if (sVar == "LOCLAXMLPath")          ResData.LOC.LAXMLPath = sVal;
//else if (sVar == "LOCLAXMLFormat")        ResData.LOC.LAXMLForm = sVal;
  else if (sVar == "LOCChLogPath")          ResData.LOC.ChLogPath = sVal;

  else if (sVar == "MRGLPath")              ResData.MRG.LPath = sVal;
//else if (sVar == "MRGLForm")              ResData.MRG.LForm = sVal;
  else if (sVar == "MRGAXMLPath")           ResData.MRG.AXMLPath = sVal;
//else if (sVar == "MRGLFormInAXML")        ResData.MRG.LFormInAXML = sVal;
//else if (sVar == "MRGLAXMLPath")          ResData.MRG.LAXMLPath = sVal;
//else if (sVar == "MRGLAXMLFormat")        ResData.MRG.LAXMLForm = sVal;
  else if (sVar == "MRGChLogPath")          ResData.MRG.ChLogPath = sVal;

  else if (sVar == "UPSSRCOwner")           ResData.UPSSRC.Owner = sVal;
  else if (sVar == "UPSSRCRepo")            ResData.UPSSRC.Repo = sVal;
  else if (sVar == "UPSSRCBranch")          ResData.UPSSRC.Branch = sVal;

  else if (sVar == "UPSSRCLPath")           ResData.UPSSRC.LPath = sVal;
//else if (sVar == "UPSSRCLForm")           ResData.UPSSRC.LForm = sVal;
  else if (sVar == "UPSSRCAXMLPath")        ResData.UPSSRC.AXMLPath = sVal;
//else if (sVar == "UPSSRCLFormInAXML")     ResData.UPSSRC.LFormInAXML = sVal;
//else if (sVar == "UPSSRCLAXMLPath")       ResData.UPSSRC.LAXMLPath = sVal;
//else if (sVar == "UPSSRCLAXMLFormat")     ResData.UPSSRC.LAXMLForm = sVal;
//else if (sVar == "UPSSRCChLogPath")       ResData.UPSSRC.ChLogPath = sVal;

  else if (sVar == "LOCSRCOwner")           ResData.LOCSRC.Owner = sVal;
  else if (sVar == "LOCSRCRepo")            ResData.LOCSRC.Repo = sVal;
  else if (sVar == "LOCSRCBranch")          ResData.LOCSRC.Branch = sVal;

  else if (sVar == "LOCSRCLPath")           ResData.LOCSRC.LPath = sVal;
//else if (sVar == "LOCSRCLForm")           ResData.LOCSRC.LForm = sVal;
  else if (sVar == "LOCSRCAXMLPath")        ResData.LOCSRC.AXMLPath = sVal;
  else if (sVar == "LOCSRCLFormInAXML")     ResData.LOCSRC.LFormInAXML = sVal;
//else if (sVar == "LOCSRCLAXMLPath")       ResData.LOCSRC.LAXMLPath = sVal;
//else if (sVar == "LOCSRCLAXMLFormat")     ResData.LOCSRC.LAXMLForm = sVal;
  else if (sVar == "LOCSRCChLogPath")       ResData.LOCSRC.ChLogPath = sVal;

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
  else if (sVar == "GitCommitText")         ResData.sGitCommitText = sVal;
  else if (sVar == "GitCommitTextSRC")      ResData.sGitCommitTextSRC = sVal;
//else if (sVar == "ProjRootDir")           ResData.sProjRootDir = sVal;
  else if (sVar == "MRGLFilesDir")          ResData.sMRGLFilesDir = sVal;
  else if (sVar == "UPSLocalPath")          ResData.sUPSLocalPath = sVal;
  else if (sVar == "UPDLFilesDir")          ResData.sUPDLFilesDir = sVal;
  else if (sVar == "SupportEmailAddr")      ResData.sSupportEmailAddr = sVal;
  else if (sVar == "SRCLCode")              ResData.sSRCLCode = sVal;
  else if (sVar == "BaseLForm")             ResData.sBaseLForm = sVal;
  else if (sVar == "LTeamLFormat")          ResData.sLTeamLFormat = sVal;
  else if (sVar == "LDatabaseURL")          ResData.sLDatabaseURL = sVal;
  else if (sVar == "MinComplPercent")       ResData.iMinComplPercent = strtol(&sVal[0], NULL, 10);
  else if (sVar == "CacheExpire")           ResData.iCacheExpire = strtol(&sVal[0], NULL, 10);
  else if (sVar == "ForceComm")             ResData.bForceComm = (sVal == "true");
  else if (sVar == "ForceGitDloadToCache")  ResData.bForceGitDloadToCache = (sVal == "true");
  else if (sVar == "SkipGitReset")          ResData.bSkipGitReset = (sVal == "true");
  else if (sVar == "Rebrand")               ResData.bRebrand = (sVal == "true");
  else if (sVar == "ForceTXUpd")            ResData.bForceTXUpd = (sVal == "true");
  else if (sVar == "IsLangAddon")           ResData.bIsLangAddon = (sVal == "true");
  else if (sVar == "HasOnlyAddonXML")       ResData.bHasOnlyAddonXML = (sVal == "true");
  else if (bIgnoreMissing)
    return;

  else
    CLog::Log(logERROR, "ConfHandler: Unreconised internal variable name: \"%s\"", sVar.c_str());

  m_MapOfVariables[sVar] = sVal;
}

void CUpdateXMLHandler::LoadResDataToMem (std::string rootDir, std::map<std::string, CXMLResdata> & mapResData, std::map<std::string, CBasicGITData> * pMapGitRepos,
                                          std::map<int, std::string>& mapResOrder)
{
  iResCounter = 0;
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
    else if (sLine.find("pset ") == 0)
      m_vecPermVariables.push_back(sLine);
    else if (sLine.find("clear ") == 0)
      ClearVariables(sLine, ResData);
    else if (sLine.find("create resource ") == 0)
      CreateResource(ResData, sLine, mapResData, mapResOrder);
    else
      SetExternalVariables(sLine);
  }
}

void CUpdateXMLHandler::HandlePermanentVariables(CXMLResdata& ResData)
{
  for (std::vector<std::string>::iterator itvec = m_vecPermVariables.begin(); itvec != m_vecPermVariables.end(); itvec++)
  {
    SetInternalVariables(*itvec, ResData);
  }
}

std::string CUpdateXMLHandler::ReplaceResName(std::string sVal, const CXMLResdata& ResData)
{
  g_CharsetUtils.replaceAllStrParts (&sVal, "$(RESNAME)", ResData.sResName);
  g_CharsetUtils.replaceAllStrParts (&sVal, "$(TRXRESNAME)", ResData.TRX.ResName);
  return sVal;
}

void CUpdateXMLHandler::CreateResource(CXMLResdata& ResData, const std::string& sLine, std::map<std::string, CXMLResdata> & mapResData, std::map<int, std::string>& mapResOrder)
{
  HandlePermanentVariables(ResData); //Handle Permanent variable assignements, which get new values after each create resource

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
  ResDataToStore.TRX.ResName = sLine.substr(posTRXResName, sLine.find_first_of(" ,\n", posTRXResName)-posTRXResName);


  if (sLine.find("GITCommit") != std::string::npos)
  {
    ResDataToStore.sGitCommitText = ReplaceResName(ResData.sGitCommitText, ResDataToStore);
    ResDataToStore.sGitCommitTextSRC = ReplaceResName(ResData.sGitCommitTextSRC, ResDataToStore);
  }

  if (sLine.find("SkipVersionBump") != std::string::npos)
    ResDataToStore.bSkipVersionBump = true;

  ResDataToStore.UPS.Owner            = ReplaceResName(ResData.UPS.Owner, ResDataToStore);
  ResDataToStore.UPS.Repo             = ReplaceResName(ResData.UPS.Repo, ResDataToStore);
  ResDataToStore.UPS.Branch           = ReplaceResName(ResData.UPS.Branch, ResDataToStore);

  ResDataToStore.UPS.LPath            = ReplaceResName(ResData.UPS.LPath, ResDataToStore);
//ResDataToStore.UPS.LForm            = ReplaceResName(ResData.UPS.LForm, ResDataToStore);
  ResDataToStore.UPS.AXMLPath         = ReplaceResName(ResData.UPS.AXMLPath, ResDataToStore);
  ResDataToStore.UPS.LFormInAXML      = ReplaceResName(ResData.UPS.LFormInAXML, ResDataToStore);
//ResDataToStore.UPS.LAXMLPath        = ReplaceResName(ResData.UPS.LAXMLPath, ResDataToStore);
//ResDataToStore.UPS.LAXMLForm        = ReplaceResName(ResData.UPS.LAXMLForm, ResDataToStore);
  ResDataToStore.UPS.ChLogPath        = ReplaceResName(ResData.UPS.ChLogPath, ResDataToStore);

  ResDataToStore.LOC.Owner            = ReplaceResName(ResData.LOC.Owner, ResDataToStore);
  ResDataToStore.LOC.Repo             = ReplaceResName(ResData.LOC.Repo, ResDataToStore);
  ResDataToStore.LOC.Branch           = ReplaceResName(ResData.LOC.Branch, ResDataToStore);

  ResDataToStore.LOC.LPath            = ReplaceResName(ResData.LOC.LPath, ResDataToStore);
//ResDataToStore.LOC.LForm            = ReplaceResName(ResData.LOC.LForm, ResDataToStore);
  ResDataToStore.LOC.AXMLPath         = ReplaceResName(ResData.LOC.AXMLPath, ResDataToStore);
  ResDataToStore.LOC.LFormInAXML      = ReplaceResName(ResData.LOC.LFormInAXML, ResDataToStore);
//ResDataToStore.LOC.LAXMLPath        = ReplaceResName(ResData.LOC.LAXMLPath, ResDataToStore);
//ResDataToStore.LOC.LAXMLForm        = ReplaceResName(ResData.LOC.LAXMLForm, ResDataToStore);
  ResDataToStore.LOC.ChLogPath        = ReplaceResName(ResData.LOC.ChLogPath, ResDataToStore);

  ResDataToStore.MRG.LPath            = ReplaceResName(ResData.MRG.LPath, ResDataToStore);
//ResDataToStore.MRG.LForm            = ReplaceResName(ResData.MRG.LForm, ResDataToStore);
  ResDataToStore.MRG.AXMLPath         = ReplaceResName(ResData.MRG.AXMLPath, ResDataToStore);
//ResDataToStore.MRG.LFormInAXML      = ReplaceResName(ResData.MRG.LFormInAXML, ResDataToStore);
//ResDataToStore.MRG.LAXMLPath        = ReplaceResName(ResData.MRG.LAXMLPath, ResDataToStore);
//ResDataToStore.MRG.LAXMLForm        = ReplaceResName(ResData.MRG.LAXMLForm, ResDataToStore);
  ResDataToStore.MRG.ChLogPath        = ReplaceResName(ResData.MRG.ChLogPath, ResDataToStore);

  ResDataToStore.UPSSRC.Owner         = ReplaceResName(ResData.UPSSRC.Owner, ResDataToStore);
  ResDataToStore.UPSSRC.Repo          = ReplaceResName(ResData.UPSSRC.Repo, ResDataToStore);
  ResDataToStore.UPSSRC.Branch        = ReplaceResName(ResData.UPSSRC.Branch, ResDataToStore);

  ResDataToStore.UPSSRC.LPath         = ReplaceResName(ResData.UPSSRC.LPath, ResDataToStore);
//ResDataToStore.UPSSRC.LForm         = ReplaceResName(ResData.UPSSRC.LForm, ResDataToStore);
  ResDataToStore.UPSSRC.AXMLPath      = ReplaceResName(ResData.UPSSRC.AXMLPath, ResDataToStore);
//ResDataToStore.UPSSRC.LFormInAXML   = ReplaceResName(ResData.UPSSRC.LFormInAXML, ResDataToStore);
//ResDataToStore.UPSSRC.LAXMLPath     = ReplaceResName(ResData.UPSSRC.LAXMLPath, ResDataToStore);
//ResDataToStore.UPSSRC.LAXMLForm     = ReplaceResName(ResData.UPSSRC.LAXMLForm, ResDataToStore);
//ResDataToStore.UPSSRC.ChLogPath     = ReplaceResName(ResData.UPSSRC.ChLogPath, ResDataToStore);

  ResDataToStore.LOCSRC.Owner         = ReplaceResName(ResData.LOCSRC.Owner, ResDataToStore);
  ResDataToStore.LOCSRC.Repo          = ReplaceResName(ResData.LOCSRC.Repo, ResDataToStore);
  ResDataToStore.LOCSRC.Branch        = ReplaceResName(ResData.LOCSRC.Branch, ResDataToStore);

  ResDataToStore.LOCSRC.LPath         = ReplaceResName(ResData.LOCSRC.LPath, ResDataToStore);
//ResDataToStore.LOCSRC.LForm         = ReplaceResName(ResData.LOCSRC.LForm, ResDataToStore);
  ResDataToStore.LOCSRC.AXMLPath      = ReplaceResName(ResData.LOCSRC.AXMLPath, ResDataToStore);
  ResDataToStore.LOCSRC.LFormInAXML   = ReplaceResName(ResData.LOCSRC.LFormInAXML, ResDataToStore);
//ResDataToStore.LOCSRC.LAXMLPath     = ReplaceResName(ResData.LOCSRC.LAXMLPath, ResDataToStore);
//ResDataToStore.LOCSRC.LAXMLForm     = ReplaceResName(ResData.LOCSRC.LAXMLForm, ResDataToStore);
  ResDataToStore.LOCSRC.ChLogPath     = ReplaceResName(ResData.LOCSRC.ChLogPath, ResDataToStore);

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
  ResDataToStore.sUPSLocalPath        = ReplaceResName(ResData.sUPSLocalPath, ResDataToStore);
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
  ResDataToStore.bForceGitDloadToCache= ResData.bForceGitDloadToCache;
  ResDataToStore.bSkipGitReset        = ResData.bSkipGitReset;
  ResDataToStore.bIsLangAddon         = ResData.bIsLangAddon;
  ResDataToStore.bHasOnlyAddonXML     = ResData.bHasOnlyAddonXML;
  ResDataToStore.m_pMapGitRepos       = ResData.m_pMapGitRepos;

  //If we don't have a different target trx resource name, use the source trx resource name
  if (ResDataToStore.UPD.ResName.empty())
    ResDataToStore.UPD.ResName = ResDataToStore.TRX.ResName;


  mapResData[ResDataToStore.sResName] = ResDataToStore;
  iResCounter++;
  mapResOrder[iResCounter] = ResDataToStore.sResName;

  ResData.UPD.ResName.clear();

  //Store git data for git clone the repositories needed for upstream push and pull handling
  //TODO include LOC gitrepositories as well
  if (ResDataToStore.UPS.Repo.empty() || ResDataToStore.UPS.Branch.empty() || ResDataToStore.UPS.Owner.empty())
    CLog::Log(logERROR, "Confhandler: Insufficient UPS git data. Missing Owner or Repo or Branch data.");
  if (ResDataToStore.sUPSLocalPath.empty())
    CLog::Log(logERROR, "Confhandler: missing folder path for local upstream git clone data.");

  CBasicGITData BasicGitData;
  BasicGitData.Owner = ResDataToStore.UPS.Owner; BasicGitData.Repo = ResDataToStore.UPS.Repo; BasicGitData.Branch = ResDataToStore.UPS.Branch;
  BasicGitData.sUPSLocalPath = ResDataToStore.sUPSLocalPath;
  ResDataToStore.m_pMapGitRepos->operator[](ResDataToStore.UPS.Owner + "/" + ResDataToStore.UPS.Repo + "/" + ResDataToStore.UPS.Branch) = BasicGitData;
  if (ResDataToStore.bIsLangAddon)
  {
    if (ResDataToStore.UPSSRC.Repo.empty() || ResDataToStore.UPSSRC.Branch.empty() || ResDataToStore.UPSSRC.Owner.empty())
      CLog::Log(logERROR, "Confhandler: Insufficient UPSSRC git data. Missing owner or Repo or Branch data.");

    BasicGitData.Owner = ResDataToStore.UPSSRC.Owner; BasicGitData.Repo = ResDataToStore.UPSSRC.Repo; BasicGitData.Branch = ResDataToStore.UPSSRC.Branch;
    BasicGitData.sUPSLocalPath = ResDataToStore.sUPSLocalPath;
    ResDataToStore.m_pMapGitRepos->operator[](ResDataToStore.UPSSRC.Owner + "/" + ResDataToStore.UPSSRC.Repo + "/" + ResDataToStore.UPSSRC.Branch) = BasicGitData;
  }
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
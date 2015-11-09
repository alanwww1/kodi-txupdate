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
#ifndef UPDXMLHANDLER_H
#define UPDXMLHANDLER_H

#pragma once

#include "TinyXML/tinyxml.h"
#include <string>
#include <map>
#include <vector>

struct CBasicGITData
{
  std::string Owner, Repo, Branch;
  std::string sUPSLocalPath;
};

struct CGITData
{
  // L=Language, A = Addon
  std::string Owner, Repo, Branch;
  std::string LPath;
//std::string LForm;
  std::string AXMLPath, LFormInAXML;
//std::string LAXMLPath, LAXMLForm;
  std::string ChLogPath;
};

struct CTRXData
{
  std::string ProjectName;
  std::string LongProjectName;
  std::string ResName;
  std::string LForm;
};

class CXMLResdata
{
public:
  CXMLResdata();
  ~CXMLResdata();
  std::string sResName;

  //NEW
  CGITData UPS, UPSSRC;
  CGITData LOC, LOCSRC;
  CGITData MRG;
  CTRXData TRX, UPD;

  std::string sChgLogFormat;
  std::string sGitCommitText, sGitCommitTextSRC;
  bool bIsLangAddon;
  bool bHasOnlyAddonXML;

  std::string sProjRootDir;
  std::string sMRGLFilesDir;
  std::string sUPSLocalPath;
  std::string sUPDLFilesDir;
  std::string sSupportEmailAddr;
  std::string sSRCLCode;
  std::string sBaseLForm;
  std::string sLTeamLFormat;
  std::string sLDatabaseURL;
  int iMinComplPercent;
  int iCacheExpire;
  bool bForceComm;
  bool bRebrand;
  bool bForceTXUpd;
  bool bForceGitDloadToCache;
  bool bSkipGitReset;
  bool bSkipVersionBump;
  std::map<std::string, CBasicGITData> * m_pMapGitRepos;
};

class CUpdateXMLHandler
{
public:
  CUpdateXMLHandler();
  ~CUpdateXMLHandler();
  void LoadResDataToMem (std::string rootDir, std::map<std::string, CXMLResdata> & mapResData, std::map<std::string, CBasicGITData> * pMapGitRepos,
                         std::map<int, std::string>& mapResOrder);

private:
  bool GetParamsFromURLorPath (std::string const &strURL, std::string &strLangFormat, std::string &strFileName,
                               std::string &strURLRoot, const char strSeparator);
  bool GetParamsFromURLorPath (std::string const &strURL, std::string &strFileName,
                               std::string &strURLRoot, const char strSeparator);

  std::map<std::string, std::string> m_MapOfVariables;
  std::vector<std::string> m_vecPermVariables;
  size_t FindVariable(const std::string& sVar);
  void SetInternalVariables(const std::string& sLine, CXMLResdata& ResData);
  void SetExternalVariables(const std::string& sLine);
  void SubstituteExternalVariables(std::string& sVar, bool bIgnoreMissing);
protected:
  void CreateResource(CXMLResdata& ResData, const std::string& sLine, std::map<std::string, CXMLResdata> & mapResData, std::map<int, std::string>& mapResOrder);
  void HandlePermanentVariables(CXMLResdata& ResData);
  std::string ReplaceResName(std::string sVal, const CXMLResdata& ResData);
  void ClearVariables(const std::string& sLine, CXMLResdata& ResData);
  void SetInternalVariable(const std::string& sVar, const std::string sVal, CXMLResdata& ResData, bool bIgnoreMissing);
  int iResCounter;
};

#endif
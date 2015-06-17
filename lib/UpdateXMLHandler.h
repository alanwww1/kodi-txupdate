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

const size_t DEFAULTCACHEEXPIRE = 360; // 6 hours
const int DEFAULTMINCOMPLETION = 10; // %
const std::string DEFAULTMERGEDLANGDIR = "merged-langfiles";
const std::string DEFAULTTXUPDLANGDIR = "tempfiles_txupdate";
const std::string DEFAULTSUPPORTMAIL = "txtranslation@kodi.tv";
const std::string DEFAULTSOURCELCODE = "en_GB";
const std::string DEFAULTBASELCODE = "$(LCODE)";
const std::string DEFAULTTXLFORMAT = "$(LCODE)";
const std::string DEFAULTLANGTEAMLFORMAT = "$(GUILNAME)";
const std::string DEFAULTLANGDATABASELINK = "https://raw.github.com/xbmc/translations/master/tool/lang-database/kodi-languages.json";
const std::string DEFAULTLANGFORMATINADDONXML = "$(OLDLCODE)";

class CXMLResdata
{
public:
  CXMLResdata();
  ~CXMLResdata();
  std::string strResName;

  std::string strTXName, strTargetTXName;

  std::string strUPSLangURL, strUPSLangURLRoot, strUPSLangFormat, strUPSLangFileName;
  std::string strUPSSourceLangURL;
  std::string strUPSAddonURL, strUPSAddonURLRoot, strUPSAddonLangFormat, strUPSAddonLangFormatinXML, strUPSAddonXMLFilename;
  std::string strUPSSourceLangAddonURL;
  std::string strUPSChangelogURL, strUPSChangelogURLRoot, strUPSChangelogName;

  std::string strLOCLangPath, strLOCLangPathRoot, strLOCLangFormat, strLOCLangFileName;
  std::string strLOCAddonPath, strLOCAddonPathRoot, strLOCAddonLangFormat, strLOCAddonLangFormatinXML, strLOCAddonXMLFilename;
  std::string strLOCChangelogPath, strLOCChangelogPathRoot, strLOCChangelogName;

  std::string strChangelogFormat;
  bool bIsLanguageAddon;
  bool bHasOnlyAddonXML;
  bool bDloadFromLocalGitRepo;
  bool bCopyToLocalGitRepo;

  std::string strProjectName;
  std::string strProjRootdir;
  std::string strTargetProjectName;
  std::string strTargetProjectNameLong;
  std::string strMergedLangfileDir;
  int iMinComplPercent;
  std::string strTXUpdateLangfilesDir;
  std::string strSupportEmailAdd;
  std::string strSourceLcode;
  std::string strBaseLCode;
  std::string strDefTXLFormat;
  std::string strTargTXLFormat;
  std::string strLangteamLFormat;
  std::string LangDatabaseURL;
  bool bForceComm;
  bool bRebrand;
  bool bForceTXUpd;
};

class CUpdateXMLHandler
{
public:
  CUpdateXMLHandler();
  ~CUpdateXMLHandler();
  void LoadUpdXMLToMem(std::string rootDir, std::map<std::string, CXMLResdata> & mapResData);

private:
  bool GetParamsFromURLorPath (std::string const &strURL, std::string &strLangFormat, std::string &strFileName,
                               std::string &strURLRoot, const char strSeparator);
  bool GetParamsFromURLorPath (std::string const &strURL, std::string &strFileName,
                               std::string &strURLRoot, const char strSeparator);
};

#endif
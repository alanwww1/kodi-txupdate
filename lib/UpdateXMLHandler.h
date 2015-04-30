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
#include "POUtils/POUtils.h"
#include <string>

class CXMLResdata
{
public:
  CXMLResdata();
  ~CXMLResdata();
  std::string strName;

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
};

class CUpdateXMLHandler
{
public:
  CUpdateXMLHandler();
  ~CUpdateXMLHandler();
  bool LoadXMLToMem(std::string rootDir);
  CXMLResdata GetResData(std::string strResName);
  const std::map<std::string, CXMLResdata> &GetResMap() const {return m_mapXMLResdata;}
  std::string GetResNameFromTXResName(std::string const &strTXResName);
private:
  int GetResType(std::string const &ResRootDir) const {return m_resType;}
  std::string IntToStr(int number);
  bool GetParamsFromURLorPath (std::string const &strURL, std::string &strLangFormat, std::string &strFileName,
                               std::string &strURLRoot, const char strSeparator);
  bool GetParamsFromURLorPath (std::string const &strURL, std::string &strFileName,
                               std::string &strURLRoot, const char strSeparator);
  int m_resType; 
  std::map<std::string, CXMLResdata> m_mapXMLResdata;
  std::map<std::string, CXMLResdata>::iterator itXMLResdata;
};
extern CUpdateXMLHandler g_UpdateXMLHandler;
#endif
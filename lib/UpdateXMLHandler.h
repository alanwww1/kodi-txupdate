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

  std::string strTXName;
  std::string strTXSourcelang;

  std::string strUPSLangURL, strUPSLangURLRoot, strUPSLangURLPost, strUPSLangFormat, strUPSLangFileType;
  std::string strUPSLangEnURL;
  std::string strUPSAddonURL, strUPSAddonURLRoot;
  std::string strUPSSourcelang;
  std::string strUPSChangelogURL;

  std::string strLocalLangPath;
  std::string strLocalAddonPath;


//  std::string strLangsFromUpstream;
//  int Restype;
//  std::string strResDirectory;
//  std::string strURLSuffix;
//  std::string strDIRprefix;
//  std::string strAddonXMLSuffix;
  std::string strChangelogFormat;
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
  void GetParametersFromURL(string const &strURL, string &strPre, string &strPost,
                            string &strLangFormat, string &strLangFileType, string &strSourcelang);
  int m_resType; 
  std::map<std::string, CXMLResdata> m_mapXMLResdata;
  std::map<std::string, CXMLResdata>::iterator itXMLResdata;
};
extern CUpdateXMLHandler g_UpdateXMLHandler;
#endif
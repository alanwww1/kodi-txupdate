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
#include "Settings.h"
#include <stdlib.h>
#include <sstream>


using namespace std;

CXMLResdata::CXMLResdata()
{
  strTXLangFormat = "$(NEWLCODE)";
}

CXMLResdata::~CXMLResdata()
{}

CUpdateXMLHandler::CUpdateXMLHandler()
{};

CUpdateXMLHandler::~CUpdateXMLHandler()
{};

bool CUpdateXMLHandler::LoadXMLToMem (std::string rootDir)
{
  std::string UpdateXMLFilename = rootDir  + DirSepChar + "xbmc-txupdate.xml";
  TiXmlDocument xmlUpdateXML;

  if (!xmlUpdateXML.LoadFile(UpdateXMLFilename.c_str()))
  {
    CLog::Log(logERROR, "UpdXMLHandler: No 'xbmc-txupdate.xml' file exists in the specified project dir. Cannot continue. "
                        "Please create one!");
    return false;
  }

  CLog::Log(logINFO, "UpdXMLHandler: Succesfuly found the update.xml file in the specified project directory");

  TiXmlElement* pRootElement = xmlUpdateXML.RootElement();
  if (!pRootElement || pRootElement->NoChildren() || pRootElement->ValueTStr()!="resources")
  {
    CLog::Log(logERROR, "UpdXMLHandler: No root element called \"resources\" in xml file. Cannot continue. Please create it");
    return false;
  }

  std::string strHTTPCacheExp;
  if (pRootElement->Attribute("http_cache_expire") && (strHTTPCacheExp = pRootElement->Attribute("http_cache_expire")) != "")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Found http cache expire time in xbmc-txupdate.xml file: %s", strHTTPCacheExp.c_str());
    g_Settings.SetHTTPCacheExpire(strtol(&strHTTPCacheExp[0], NULL, 10));
  }
  else
    CLog::Log(logINFO, "UpdXMLHandler: No http cache expire time specified in xbmc-txupdate.xml file. Using default value: %iminutes",
              DEFAULTCACHEEXPIRE);

  std::string strProjName ;
  if (pRootElement->Attribute("projectname") && (strProjName = pRootElement->Attribute("projectname")) != "")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Found projectname in xbmc-txupdate.xml file: %s",strProjName.c_str());
    g_Settings.SetProjectname(strProjName);
  }
  else
    CLog::Log(logERROR, "UpdXMLHandler: No projectname specified in xbmc-txupdate.xml file. Cannot continue. "
                        "Please specify the Transifex projectname in the xml file");

  std::string strMinCompletion;
  if (pRootElement->Attribute("min_completion") && (strMinCompletion = pRootElement->Attribute("min_completion")) != "")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Found min completion percentage in xbmc-txupdate.xml file: %s", strMinCompletion.c_str());
    g_Settings.SetMinCompletion(strtol(&strMinCompletion[0], NULL, 10));
  }
  else
    CLog::Log(logINFO, "UpdXMLHandler: No min completion percentage specified in xbmc-txupdate.xml file. Using default value: %i%",
              DEFAULTMINCOMPLETION);

  std::string strMergedLangfileDir;
  if (pRootElement->Attribute("merged_langfiledir") && (strMergedLangfileDir = pRootElement->Attribute("merged_langfiledir")) != "")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Found merged language file directory in xbmc-txupdate.xml file: %s", strMergedLangfileDir.c_str());
    g_Settings.SetMergedLangfilesDir(strMergedLangfileDir);
  }
  else
    CLog::Log(logINFO, "UpdXMLHandler: No merged language file directory specified in xbmc-txupdate.xml file. Using default value: %s",
              g_Settings.GetMergedLangfilesDir().c_str());

  std::string strTXUpdatelangfileDir;
  if (pRootElement->Attribute("temptxupdate_langfiledir") && (strTXUpdatelangfileDir = pRootElement->Attribute("temptxupdate_langfiledir")) != "")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Found temp tx update language file directory in xbmc-txupdate.xml file: %s", strTXUpdatelangfileDir.c_str());
    g_Settings.SetTXUpdateLangfilesDir(strTXUpdatelangfileDir);
  }
  else
    CLog::Log(logINFO, "UpdXMLHandler: No temp tx update language file directory specified in xbmc-txupdate.xml file. Using default value: %s",
              g_Settings.GetTXUpdateLangfilesDir().c_str());

  std::string strSupportEmailAdd;
  if (pRootElement->Attribute("support_email") && (strSupportEmailAdd = pRootElement->Attribute("support_email")) != "")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Found support email address in xbmc-txupdate.xml file: %s", strSupportEmailAdd.c_str());
    g_Settings.SetSupportEmailAdd(strSupportEmailAdd);
  }
  else
    CLog::Log(logINFO, "UpdXMLHandler: No support email address specified in xbmc-txupdate.xml file. Using default value: %s",
              g_Settings.GetSupportEmailAdd().c_str());

  std::string strAttr;
  if (pRootElement->Attribute("forcePOComm") && (strAttr = pRootElement->Attribute("forcePOComm")) == "true")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Forced PO file comments for non English languages.", strMergedLangfileDir.c_str());
    g_Settings.SetForcePOComments(true);
  }

  if (pRootElement->Attribute("Rebrand") && (strAttr = pRootElement->Attribute("Rebrand")) == "true")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Rebrand of XBMC strings to Kodi strings turned on.");
    g_Settings.SetRebrand(true);
  }

  if (pRootElement->Attribute("ForceTXUpd") && (strAttr = pRootElement->Attribute("ForceTXUpd")) == "true")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Create of TX update files is forced.");
    g_Settings.SetForceTXUpdate(true);
  }

  const TiXmlElement *pChildResElement = pRootElement->FirstChildElement("resource");
  if (!pChildResElement || pChildResElement->NoChildren())
  {
    CLog::Log(logERROR, "UpdXMLHandler: No xml element called \"resource\" exists in the xml file. Cannot continue. Please create at least one");
    return false;
  }

  std::string strType;
  while (pChildResElement && pChildResElement->FirstChild())
  {
    CXMLResdata currResData;
    std::string strResName;
    if (!pChildResElement->Attribute("name") || (strResName = pChildResElement->Attribute("name")) == "")
    {
      CLog::Log(logERROR, "UpdXMLHandler: No name specified for resource. Cannot continue. Please specify it.");
      return false;
    }
    currResData.strName =strResName;

    if (pChildResElement->FirstChild())
    {
      const TiXmlElement *pChildTXNameElement = pChildResElement->FirstChildElement("TXname");
      if (pChildTXNameElement && pChildTXNameElement->FirstChild())
        currResData.strTXName = pChildTXNameElement->FirstChild()->Value();
      if (currResData.strTXName.empty())
        CLog::Log(logERROR, "UpdXMLHandler: Transifex resource name is empty or missing for resource %s", strResName.c_str());
      std::string strTXLcodeFormat;
      if (pRootElement->Attribute("lcode") && (strTXLcodeFormat = pRootElement->Attribute("lcode")) != "")
        currResData.strTXLangFormat = strTXLcodeFormat;
      currResData.strTXSourceLang = g_LCodeHandler.GetLangFromLCode(g_Settings.GetSourceLcode(), currResData.strTXLangFormat);

      const TiXmlElement *pChildURLElement = pChildResElement->FirstChildElement("upstreamLangURL");
      if (pChildURLElement && pChildURLElement->FirstChild())
        currResData.strUPSLangURL = pChildURLElement->FirstChild()->Value();
      if (currResData.strUPSLangURL.empty())
        CLog::Log(logINFO, "UpdXMLHandler: UpstreamURL entry is empty for resource %s, which means we have no language files for this addon", strResName.c_str());
      GetParametersFromLangURL(currResData.strUPSLangURL, currResData.strUPSLangURLRoot, currResData.strUPSLangURLPost, currResData.strUPSLangFormat,
                           currResData.strUPSLangFileType, currResData.strUPSSourcelang);

      const TiXmlElement *pChildURLENElement = pChildResElement->FirstChildElement("upstreamLangEnURL");
      if (pChildURLENElement && pChildURLENElement->FirstChild())
        currResData.strUPSLangEnURL = pChildURLENElement->FirstChild()->Value();

      const TiXmlElement *pChildAddonURLElement = pChildResElement->FirstChildElement("upstreamAddonURL");
      if (pChildAddonURLElement && pChildAddonURLElement->FirstChild())
        currResData.strUPSAddonURL = pChildAddonURLElement->FirstChild()->Value();
      currResData.strUPSAddonURLRoot = currResData.strUPSAddonURL;
      currResData.strUPSAddonURLRoot.resize(currResData.strUPSAddonURLRoot.find_last_of("/")+1);

      const TiXmlElement *pChildChglogElement = pChildResElement->FirstChildElement("changelogFormat");
      if (pChildChglogElement && pChildChglogElement->FirstChild())
        currResData.strChangelogFormat = pChildChglogElement->FirstChild()->Value();

      const TiXmlElement *pChildChglogUElement = pChildResElement->FirstChildElement("upstreamChangelogURL");
      if (pChildChglogUElement && pChildChglogUElement->FirstChild())
        currResData.strUPSChangelogURL = pChildChglogUElement->FirstChild()->Value();

      const TiXmlElement *pChildLocLangElement = pChildResElement->FirstChildElement("localLangPath");
      if (pChildLocLangElement && pChildLocLangElement->FirstChild())
        currResData.strLocalLangPath = pChildLocLangElement->FirstChild()->Value();

      const TiXmlElement *pChildLocAddonElement = pChildResElement->FirstChildElement("localAddonPath");
      if (pChildLocAddonElement && pChildLocAddonElement->FirstChild())
        currResData.strLocalAddonPath = pChildLocAddonElement->FirstChild()->Value();

      m_mapXMLResdata[strResName] = currResData;
      CLog::Log(logINFO, "UpdXMLHandler: found resource in update.xml file: %s, Type: %s, SubDir: %s",
                strResName.c_str(), strType.c_str(), currResData.strResDirectory.c_str());
    }
    pChildResElement = pChildResElement->NextSiblingElement("resource");
  }

  return true;
};

std::string CUpdateXMLHandler::GetResNameFromTXResName(std::string const &strTXResName)
{
  for (itXMLResdata = m_mapXMLResdata.begin(); itXMLResdata != m_mapXMLResdata.end(); itXMLResdata++)
  {
    if (itXMLResdata->second.strTXResName == strTXResName)
      return itXMLResdata->first;
  }
  return "";
}

std::string CUpdateXMLHandler::IntToStr(int number)
{
  std::stringstream ss;//create a stringstream
  ss << number;//add number to the stream
  return ss.str();//return a string with the contents of the stream
};

CXMLResdata CUpdateXMLHandler::GetResData(string strResName)
{
  CXMLResdata EmptyXMLResdata;
  if (m_mapXMLResdata.find(strResName) != m_mapXMLResdata.end())
    return m_mapXMLResdata[strResName];
  CLog::Log(logINFO, "UpdXMLHandler::GetResData: unknown resource to find: %s", strResName.c_str());
  return EmptyXMLResdata;
}

void CUpdateXMLHandler::GetParametersFromLangURL(string const &strURL, string &strPre, string &strPost, string &strLangFormat,
                                                 string &strLangFileName, string &strSourcelang, string strSeparator = "/")
{
  if (strURL.empty())
    return;

  if (strURL.find(strSeparator) == std::string::npos)
    CLog::Log(logERROR, "UpdXMLHandler: Wrong language file URL format: %s", strURL.c_str());

  strLangFileName = strURL.substr(strURL.find_last_of(strSeparator));

  size_t posStart, posEnd;

  if (posStart = strURL.find("$(") == std::string::npos || posEnd = strURL.find("(",posStart) == std::string::npos)
    CLog::Log(logERROR, "UpdXMLHandler: Unknown upstream language file format found for URL %s", strURL.c_str());

  strPre = strURL.substr(posStart);
  strPost = strURL.substr(posEnd);
  strLangFormat = strURL.substr(posStart, posStart-posEnd+1);
  strSourcelang = g_LCodeHandler.GetLangFromLCode(g_Settings.GetSourceLcode(),strLangFormat);
}
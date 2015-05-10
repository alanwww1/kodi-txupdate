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

CUpdateXMLHandler g_UpdateXMLHandler;

CXMLResdata::CXMLResdata()
{}

CXMLResdata::~CXMLResdata()
{}

CUpdateXMLHandler::CUpdateXMLHandler()
{};

CUpdateXMLHandler::~CUpdateXMLHandler()
{};

bool CUpdateXMLHandler::LoadXMLToMem (std::string rootDir)
{
  std::string UpdateXMLFilename = rootDir  + DirSepChar + "kodi-txupdate.xml";
  TiXmlDocument xmlUpdateXML;

  if (!xmlUpdateXML.LoadFile(UpdateXMLFilename.c_str()))
  {
    CLog::Log(logERROR, "UpdXMLHandler: No 'kodi-txupdate.xml' file exists in the specified project dir. Cannot continue. "
                        "Please create one!");
    return false;
  }

  CLog::Log(logINFO, "UpdXMLHandler: Succesfuly found the update.xml file in the specified project directory");

  TiXmlElement* pProjectRootElement = xmlUpdateXML.RootElement();
  if (!pProjectRootElement || pProjectRootElement->NoChildren() || pProjectRootElement->ValueTStr()!="project")
  {
    CLog::Log(logERROR, "UpdXMLHandler: No root element called \"project\" in xml file. Cannot continue. Please create it");
    return false;
  }

  TiXmlElement* pDataRootElement = pProjectRootElement->FirstChildElement("projectdata");
  if (!pDataRootElement || pDataRootElement->NoChildren())
  {
    CLog::Log(logERROR, "UpdXMLHandler: No element called \"projectdata\" in xml file. Cannot continue. Please create it");
    return false;
  }

  TiXmlElement * pData;
  std::string strHTTPCacheExp;
  if ((pData = pDataRootElement->FirstChildElement("http_cache_expire")) && (strHTTPCacheExp = pData->FirstChild()->Value()) != "")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Found http cache expire time in kodi-txupdate.xml file: %s", strHTTPCacheExp.c_str());
    g_Settings.SetHTTPCacheExpire(strtol(&strHTTPCacheExp[0], NULL, 10));
  }
  else
    CLog::Log(logINFO, "UpdXMLHandler: No http cache expire time specified in kodi-txupdate.xml file. Using default value: %iminutes",
              DEFAULTCACHEEXPIRE);

//TODO separate download TX projectname from upload TX projectname to handle project name changes
  std::string strProjName;
  if ((pData = pDataRootElement->FirstChildElement("projectname")) && (strProjName = pData->FirstChild()->Value()) != "")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Found projectname in kodi-txupdate.xml file: %s",strProjName.c_str());
    g_Settings.SetProjectname(strProjName);
    g_Settings.SetTargetProjectname(strProjName);
  }
  else
    CLog::Log(logERROR, "UpdXMLHandler: No projectname specified in kodi-txupdate.xml file. Cannot continue. "
    "Please specify the Transifex projectname in the xml file");

  std::string strTargetProjName;
  if ((pData = pDataRootElement->FirstChildElement("targetprojectname")) && (strTargetProjName = pData->FirstChild()->Value()) != "")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Found target projectname in kodi-txupdate.xml file: %s",strTargetProjName.c_str());
    g_Settings.SetTargetProjectname(strTargetProjName);
  }


  std::string strLongProjName;
  if ((pData = pDataRootElement->FirstChildElement("longprojectname")) && (strLongProjName = pData->FirstChild()->Value()) != "")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Found long projectname in kodi-txupdate.xml file: %s",strLongProjName.c_str());
    g_Settings.SetTargetProjectnameLong(strLongProjName);
  }
  else
    CLog::Log(logERROR, "UpdXMLHandler: No long projectname specified in kodi-txupdate.xml file. Cannot continue. "
    "Please specify the Transifex long projectname in the xml file");

  std::string strTXLangFormat;
  if ((pData = pDataRootElement->FirstChildElement("txlcode")) && (strTXLangFormat = pData->FirstChild()->Value()) != "")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Found tx langformat in kodi-txupdate.xml file: %s",strTXLangFormat.c_str());
    g_Settings.SetDefaultTXLFormat(strTXLangFormat);
  }
  g_Settings.SetTargetTXLFormat(g_Settings.GetDefaultTXLFormat());
  if ((pData = pDataRootElement->FirstChildElement("targettxlcode")) && (strTXLangFormat = pData->FirstChild()->Value()) != "")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Found target tx langformat in kodi-txupdate.xml file: %s",strTXLangFormat.c_str());
    g_Settings.SetTargetTXLFormat(strTXLangFormat);
  }

  std::string strDefAddonLangFormatinXML;
  if ((pData = pDataRootElement->FirstChildElement("addonxmllangformat")) && (strDefAddonLangFormatinXML = pData->FirstChild()->Value()) != "")
    CLog::Log(logINFO, "UpdXMLHandler: Found addon.xml langformat in kodi-txupdate.xml file: %s",strDefAddonLangFormatinXML.c_str());
  else
    strDefAddonLangFormatinXML = g_Settings.GetDefaultAddonLFormatinXML();

  std::string strDefLangdatabaseURL;
  if ((pData = pDataRootElement->FirstChildElement("langdatabaseurl")) && (strDefLangdatabaseURL = pData->FirstChild()->Value()) != "")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Found language database URL in kodi-txupdate.xml file: %s",strDefLangdatabaseURL.c_str());
    g_Settings.SetLangDatabaseURL(strDefLangdatabaseURL);
  }

  std::string strBaseLcode;
  if ((pData = pDataRootElement->FirstChildElement("baselcode")) && (strBaseLcode = pData->FirstChild()->Value()) != "")
  {
    CLog::Log(logINFO, "UpdXMLHandler: found base language code format in kodi-txupdate.xml file: %s",strBaseLcode.c_str());
  }

  std::string strMinCompletion;
  if ((pData = pDataRootElement->FirstChildElement("min_completion")) && (strMinCompletion = pData->FirstChild()->Value()) != "")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Found min completion percentage in kodi-txupdate.xml file: %s", strMinCompletion.c_str());
    g_Settings.SetMinCompletion(strtol(&strMinCompletion[0], NULL, 10));
  }
  else
    CLog::Log(logINFO, "UpdXMLHandler: No min completion percentage specified in kodi-txupdate.xml file. Using default value: %i%",
              DEFAULTMINCOMPLETION);

  std::string strMergedLangfileDir;
  if ((pData = pDataRootElement->FirstChildElement("merged_langfiledir")) && (strMergedLangfileDir = pData->FirstChild()->Value()) != "")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Found merged language file directory in kodi-txupdate.xml file: %s", strMergedLangfileDir.c_str());
    g_Settings.SetMergedLangfilesDir(strMergedLangfileDir);
  }
  else
    CLog::Log(logINFO, "UpdXMLHandler: No merged language file directory specified in kodi-txupdate.xml file. Using default value: %s",
              g_Settings.GetMergedLangfilesDir().c_str());

  std::string strSourcelcode;
  if ((pData = pDataRootElement->FirstChildElement("sourcelcode")) && (strSourcelcode = pData->FirstChild()->Value()) != "")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Found sourcelcode in kodi-txupdate.xml file: %s", strSourcelcode.c_str());
    g_Settings.SetSourceLcode(strSourcelcode);
  }
  else
    CLog::Log(logINFO, "UpdXMLHandler: No source language code specified in kodi-txupdate.xml file. Using default value: %s",
              g_Settings.GetSourceLcode().c_str());

  std::string strTXUpdatelangfileDir;
  if ((pData = pDataRootElement->FirstChildElement("temptxupdate_langfiledir")) && (strTXUpdatelangfileDir = pData->FirstChild()->Value()) != "")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Found temp tx update language file directory in kodi-txupdate.xml file: %s", strTXUpdatelangfileDir.c_str());
    g_Settings.SetTXUpdateLangfilesDir(strTXUpdatelangfileDir);
  }
  else
    CLog::Log(logINFO, "UpdXMLHandler: No temp tx update language file directory specified in kodi-txupdate.xml file. Using default value: %s",
              g_Settings.GetTXUpdateLangfilesDir().c_str());

  std::string strSupportEmailAdd;
  if ((pData = pDataRootElement->FirstChildElement("support_email")) && (strSupportEmailAdd = pData->FirstChild()->Value()) != "")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Found support email address in kodi-txupdate.xml file: %s", strSupportEmailAdd.c_str());
    g_Settings.SetSupportEmailAdd(strSupportEmailAdd);
  }
  else
    CLog::Log(logINFO, "UpdXMLHandler: No support email address specified in kodi-txupdate.xml file. Using default value: %s",
              g_Settings.GetSupportEmailAdd().c_str());

  std::string strAttr;
  if ((pData = pDataRootElement->FirstChildElement("forcePOComm")) && (strAttr = pData->FirstChild()->Value()) == "true")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Forced PO file comments for non English languages.", strMergedLangfileDir.c_str());
    g_Settings.SetForcePOComments(true);
  }

  if ((pData = pDataRootElement->FirstChildElement("rebrand")) && (strAttr = pData->FirstChild()->Value()) == "true")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Rebrand of XBMC strings to Kodi strings turned on.");
    g_Settings.SetRebrand(true);
  }

  std::string strLangteamLFormat;
  if ((pData = pDataRootElement->FirstChildElement("langteamformat")) && (strLangteamLFormat = pData->FirstChild()->Value()) != "")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Found language team format to put into PO files in kodi-txupdate.xml file: %s", strLangteamLFormat.c_str());
    g_Settings.SetLangteamLFormat(strLangteamLFormat);
  }
  else
    CLog::Log(logINFO, "UpdXMLHandler: No language team format specified in kodi-txupdate.xml file. Using default value: %s",
              g_Settings.GetLangteamLFormat().c_str());

  if ((pData = pDataRootElement->FirstChildElement("ForceTXUpd")) && (strAttr = pData->FirstChild()->Value()) == "true")
  {
    CLog::Log(logINFO, "UpdXMLHandler: Create of TX update files is forced.");
    g_Settings.SetForceTXUpdate(true);
  }

  TiXmlElement* pRootElement = pProjectRootElement->FirstChildElement("resources");
  if (!pRootElement || pRootElement->NoChildren())
  {
    CLog::Log(logERROR, "UpdXMLHandler: No element called \"resources\" in xml file. Cannot continue. Please create it");
    return false;
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
      {
        currResData.strTXName = pChildTXNameElement->FirstChild()->Value();
        currResData.strTargetTXName =currResData.strTXName;
      }
      if (currResData.strTXName.empty())
        CLog::Log(logERROR, "UpdXMLHandler: Transifex resource name is empty or missing for resource %s", strResName.c_str());

      const TiXmlElement *pChildTTXNameElement = pChildResElement->FirstChildElement("TargetTXname");
      if (pChildTTXNameElement && pChildTTXNameElement->FirstChild())
        currResData.strTargetTXName = pChildTTXNameElement->FirstChild()->Value();

      const TiXmlElement *pChildURLElement = pChildResElement->FirstChildElement("upstreamLangURL");
      if (pChildURLElement && pChildURLElement->FirstChild())
        currResData.strUPSLangURL = pChildURLElement->FirstChild()->Value();
      if ((currResData.bHasOnlyAddonXML = currResData.strUPSLangURL.empty()))
        CLog::Log(logINFO, "UpdXMLHandler: UpstreamURL entry is empty for resource %s, which means we have no language files for this addon", strResName.c_str());
      else if (!GetParamsFromURLorPath (currResData.strUPSLangURL, currResData.strUPSLangFormat, currResData.strUPSLangFileName,
                                        currResData.strUPSLangURLRoot, '/'))
        CLog::Log(logERROR, "UpdXMLHandler: UpstreamURL format is wrong for resource %s", strResName.c_str());
      if (!currResData.strUPSLangURLRoot.empty() && currResData.strUPSLangURLRoot.find (".github") == std::string::npos)
        CLog::Log(logERROR, "UpdXMLHandler: Only github is supported as upstream repository for resource %s", strResName.c_str());

      const TiXmlElement *pChildURLSRCElement = pChildResElement->FirstChildElement("upstreamLangSRCURL");
      if (pChildURLSRCElement && pChildURLSRCElement->FirstChild())
        currResData.strUPSSourceLangURL = pChildURLSRCElement->FirstChild()->Value();

      const TiXmlElement *pChildURLSRCAddonElement = pChildResElement->FirstChildElement("upstreamAddonSRCURL");
      if (pChildURLSRCAddonElement && pChildURLSRCAddonElement->FirstChild())
        currResData.strUPSSourceLangAddonURL = pChildURLSRCAddonElement->FirstChild()->Value();

      const TiXmlElement *pChildAddonURLElement = pChildResElement->FirstChildElement("upstreamAddonURL");
      if (pChildAddonURLElement && pChildAddonURLElement->FirstChild())
        currResData.strUPSAddonURL = pChildAddonURLElement->FirstChild()->Value();
      if (currResData.strUPSAddonURL.empty())
        currResData.strUPSAddonURL = currResData.strUPSLangURL.substr(0,currResData.strUPSLangURL.find(currResData.strName)
                                                                      + currResData.strName.size()) + "/addon.xml";
      if (currResData.strUPSAddonURL.empty())
        CLog::Log(logERROR, "UpdXMLHandler: Unable to determine the URL for the addon.xml file for resource %s", strResName.c_str());
      GetParamsFromURLorPath (currResData.strUPSAddonURL, currResData.strUPSAddonLangFormat, currResData.strUPSAddonXMLFilename,
                                currResData.strUPSAddonURLRoot, '/');
      if (!currResData.strUPSAddonURL.empty() && currResData.strUPSAddonURL.find (".github") == std::string::npos)
          CLog::Log(logERROR, "UpdXMLHandler: Only github is supported as upstream repository for resource %s", strResName.c_str());
      std::string strUPSAddonLangFormatinXML;
      if (pChildAddonURLElement &&  pChildAddonURLElement->Attribute("addonxmllangformat"))
        strUPSAddonLangFormatinXML = pChildAddonURLElement->Attribute("addonxmllangformat");
      if (strUPSAddonLangFormatinXML != "")
        currResData.strUPSAddonLangFormatinXML = strUPSAddonLangFormatinXML;
      else
        currResData.strUPSAddonLangFormatinXML = strDefAddonLangFormatinXML;
      currResData.bIsLanguageAddon = !currResData.strUPSAddonLangFormat.empty();


      const TiXmlElement *pChildChglogElement = pChildResElement->FirstChildElement("changelogFormat");
      if (pChildChglogElement && pChildChglogElement->FirstChild())
        currResData.strChangelogFormat = pChildChglogElement->FirstChild()->Value();

      const TiXmlElement *pChildChglogUElement = pChildResElement->FirstChildElement("upstreamChangelogURL");
      if (pChildChglogUElement && pChildChglogUElement->FirstChild())
        currResData.strUPSChangelogURL = pChildChglogUElement->FirstChild()->Value();
      else if (!currResData.strChangelogFormat.empty())
        currResData.strUPSChangelogURL = currResData.strUPSAddonURLRoot + "changelog.txt";
      if (!currResData.strChangelogFormat.empty())
      GetParamsFromURLorPath (currResData.strUPSChangelogURL, currResData.strUPSChangelogName,
                                currResData.strUPSChangelogURLRoot, '/');

//TODO if not defined automatic creation fails for language-addons
      const TiXmlElement *pChildLocLangElement = pChildResElement->FirstChildElement("localLangPath");
      if (pChildLocLangElement && pChildLocLangElement->FirstChild())
        currResData.strLOCLangPath = pChildLocLangElement->FirstChild()->Value();
      if (currResData.strLOCLangPath.empty() && !currResData.bHasOnlyAddonXML)
        currResData.strLOCLangPath = currResData.strUPSLangURL.substr(currResData.strUPSAddonURLRoot.size()-1);
      if (!currResData.bHasOnlyAddonXML)
        currResData.strLOCLangPath = currResData.strName + DirSepChar + currResData.strLOCLangPath;
      if (!currResData.bHasOnlyAddonXML && !GetParamsFromURLorPath (currResData.strLOCLangPath, currResData.strLOCLangFormat,
          currResData.strLOCLangFileName, currResData.strLOCLangPathRoot, DirSepChar))
        CLog::Log(logERROR, "UpdXMLHandler: Local langpath format is wrong for resource %s", strResName.c_str());

      const TiXmlElement *pChildLocAddonElement = pChildResElement->FirstChildElement("localAddonPath");
      if (pChildLocAddonElement && pChildLocAddonElement->FirstChild())
        currResData.strLOCAddonPath = pChildLocAddonElement->FirstChild()->Value();
      if (currResData.strLOCAddonPath.empty())
        currResData.strLOCAddonPath = currResData.strUPSAddonXMLFilename;
      currResData.strLOCAddonPath = currResData.strName + DirSepChar + currResData.strLOCAddonPath;
      GetParamsFromURLorPath (currResData.strLOCAddonPath, currResData.strLOCAddonLangFormat, currResData.strLOCAddonXMLFilename,
                              currResData.strLOCAddonPathRoot, DirSepChar);
      std::string strLOCAddonLangFormatinXML;
      if (pChildLocAddonElement && pChildLocAddonElement->Attribute("addonxmllangformat"))
        strLOCAddonLangFormatinXML = pChildLocAddonElement->Attribute("addonxmllangformat");
      if (strLOCAddonLangFormatinXML != "")
        currResData.strLOCAddonLangFormatinXML = strLOCAddonLangFormatinXML;
      else
        currResData.strLOCAddonLangFormatinXML = strDefAddonLangFormatinXML;

      const TiXmlElement *pChildChglogLElement = pChildResElement->FirstChildElement("localChangelogPath");
      if (pChildChglogLElement && pChildChglogLElement->FirstChild())
        currResData.strLOCChangelogPath = pChildChglogLElement->FirstChild()->Value();
      else
	currResData.strLOCChangelogPath = currResData.strLOCAddonPathRoot + "changelog.txt";
      GetParamsFromURLorPath (currResData.strLOCChangelogPath, currResData.strLOCChangelogName,
                                currResData.strLOCChangelogPathRoot, '/');

      m_mapXMLResdata[strResName] = currResData;
      CLog::Log(logINFO, "UpdXMLHandler: found resource in update.xml file: %s, Type: %s, SubDir: %s",
                strResName.c_str(), strType.c_str(), currResData.strLOCAddonPath.c_str());
    }
    pChildResElement = pChildResElement->NextSiblingElement("resource");
  }

  return true;
};

std::string CUpdateXMLHandler::GetResNameFromTXResName(std::string const &strTXResName)
{
  for (itXMLResdata = m_mapXMLResdata.begin(); itXMLResdata != m_mapXMLResdata.end(); itXMLResdata++)
  {
    if (itXMLResdata->second.strTXName == strTXResName)
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
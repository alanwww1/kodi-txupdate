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

#ifndef HTTPUTILS_H
#define HTTPUTILS_H

#pragma once

#include <string>
#include <stdio.h>
#include <curl/curl.h>
#include "TinyXML/tinyxml.h"
#include "POHandler.h"

struct CLoginData
{
  std::string strLogin;
  std::string strPassword;
};

struct CGithubURLData
{
  std::string strOwner;
  std::string strRepo;
  std::string strPath;
  std::string strGitBranch;
};

const std::string strUserAgent = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/35.0.1870.2 Safari/537.36";

class CHTTPHandler
{
public:
  CHTTPHandler();
  ~CHTTPHandler();
  void ReInit();
  std::string GetHTTPErrorFromCode(int http_code);
  void HTTPRetry(int nretry);
  std::string GetURLToSTR(std::string strURL);
  std::string GetURLToSTRNew(std::string strURL);

  void Cleanup();
  void SetCacheDir(std::string strCacheDir);
  std::string GetCacheDir() {return m_strCacheDir;}
  void SetHTTPCacheExpire(size_t iCacheTimeInMins) {m_iHTTPCacheExp = iCacheTimeInMins;}
  size_t GetHTTPCacheExpire() {return m_iHTTPCacheExp;}

  bool LoadCredentials (std::string CredentialsFilename);
  bool PutFileToURL(std::string const &strFilePath, std::string const &strURL, bool &buploaded,
                    size_t &stradded, size_t &strupd);
  bool CreateNewResource(const std::string& sPOFile, const CXMLResdata& XMLResData, size_t &iAddedNew);
  void DeleteCachedFile(std::string const &strURL, std::string strPrefix);
  void GetGithubData (const std::string &strURL, CGithubURLData &GithubURLData);
  std::string GetGitHUBAPIURL(std::string const & strURL);
  void GetGitCloneURL(std::string const & strURL, std::string &strGitHubURL, CGithubURLData &GithubURLData);
  bool UploadTranslatorsDatabase(std::string strJson, std::string strURL);

  void SetResName (const std::string& sResName) {m_sResName = sResName;}
  void SetLCode (const std::string& sLCode) {m_sLCode = sLCode;}
  void SetLocation (const std::string& sLocation) {m_sFileLocation = sLocation;}
  void SetProjectName (const std::string& sProjName) {m_sProjectName = sProjName;}
  void SetFileName (const std::string& sFileName) {m_sFileName = sFileName;}


private:
  std::string CacheFileNameFromURL(std::string strURL);
  long curlPUTPOStrToURL(std::string const &strFilePath, std::string const &strURL, size_t &stradded, size_t &strupd);

  CURL *m_curlHandle;
  std::string m_strCacheDir;
  long curlURLToCache(std::string strCacheFile, std::string strURL, std::string &strBuffer);

  CLoginData GetCredentials (std::string strURL);
  std::string URLEncode (std::string strURL);
  std::string CreateNewresJSONStrFromPOStr(std::string strTXResname, std::string const &strPO);
  void ParseUploadedStringsData(std::string const &strJSON, size_t &stradded, size_t &strupd);
  void ParseUploadedStrForNewRes(std::string const &strJSON, size_t &stradded);
  std::string GetURLToSTRCache(std::string strURL, std::string& strCacheFile);

  std::map<std::string, CLoginData> m_mapLoginData;
  std::map<std::string, CLoginData>::iterator itMapLoginData;
  size_t m_iHTTPCacheExp;

  std::string m_sResName, m_sFileLocation, m_sLCode, m_sProjectName, m_sFileName;
};

size_t Write_CurlData_File(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t Write_CurlData_String(char *data, size_t size, size_t nmemb, std::string *buffer);

extern CHTTPHandler g_HTTPHandler;
#endif
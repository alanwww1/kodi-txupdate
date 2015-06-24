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

#include "Log.h"
#include "HTTPUtils.h"
#include <curl/easy.h>
#include "FileUtils/FileUtils.h"
#include <cctype>
#include "Fileversioning.h"
#include "jsoncpp/json/json.h"
#include "Langcodes.h"
#include "CharsetUtils/CharsetUtils.h"


CHTTPHandler g_HTTPHandler;

using namespace std;

CHTTPHandler::CHTTPHandler()
{
  m_curlHandle = curl_easy_init();
};

CHTTPHandler::~CHTTPHandler()
{
  Cleanup();
};

std::string CHTTPHandler::GetHTTPErrorFromCode(int http_code)
{
  if (http_code == 503) return ": Service Unavailable (probably TX server maintenance) please try later.";
  else if (http_code == 400) return ": Bad request (probably an error in the utility or the API changed. Please contact the Developer.";
  else if (http_code == 401) return ": Unauthorized. Please create a .passwords file in the project root dir with credentials.";
  else if (http_code == 403) return ": Forbidden. Service is currently forbidden.";
  else if (http_code == 404) return ": File not found on the URL.";
  else if (http_code == 500) return ": Internal server error. Try again later, or contact the Utility Developer.";
  return "";
}

void CHTTPHandler::HTTPRetry(int nretry)
{
  for (int i = 0; i < nretry*6; i++)
  {
    printf (" Retry %i: %i  \b\b\b\b\b\b\b\b\b\b\b\b\b", nretry, nretry*6-i);
    if (nretry*6-i > 9)
      printf("\b");
    usleep(300000);
  }
  printf ("             \b\b\b\b\b\b\b\b\b\b\b\b\b");
  g_HTTPHandler.Cleanup();
  g_HTTPHandler.ReInit();
}

std::string CHTTPHandler::GetURLToSTRNew(std::string strURL)
{
  std::string sCacheFileName = m_strCacheDir;
  if (!m_sFileLocation.empty())
    sCacheFileName += m_sFileLocation + DirSepChar;
  if (!m_sResName.empty())
    sCacheFileName += m_sResName + DirSepChar;

  if (strURL.find("github.com") != std::string::npos)
  {
    CGithubURLData GitData;
    GetGithubData(strURL,GitData);
    sCacheFileName += GitData.strGitBranch + "/";
  }

    sCacheFileName += g_CharsetUtils.GetFilenameFromURL(strURL);

    return GetURLToSTRCache(strURL, sCacheFileName);
}

std::string CHTTPHandler::GetURLToSTR(std::string strURL)
{
  std::string strCacheFile = CacheFileNameFromURL(strURL);
  strCacheFile = m_strCacheDir + "GET/" + strCacheFile;
  return GetURLToSTRCache(strURL, strCacheFile);
}

std::string CHTTPHandler::GetURLToSTRCache(std::string strURL, std::string& strCacheFile)
{
  std::string strBuffer;
  bool bCacheFileExists = g_File.FileExist(strCacheFile);

  size_t CacheFileAge = bCacheFileExists ? g_File.GetFileAge(strCacheFile): -1; //in seconds
  size_t MaxCacheFileAge = m_iHTTPCacheExp * 60; // in seconds

  bool bCacheFileExpired = CacheFileAge > MaxCacheFileAge;

  std::string strCachedFileVersion, strWebFileVersion;
  strWebFileVersion = g_Fileversion.GetVersionForURL(strURL);


  if (strWebFileVersion != "" && g_File.FileExist(strCacheFile + ".version"))
    strCachedFileVersion = g_File.ReadFileToStr(strCacheFile + ".version");

  bool bFileChangedOnWeb = strCachedFileVersion != strWebFileVersion;

  if (!bCacheFileExists || (bCacheFileExpired && (strWebFileVersion == "" || bFileChangedOnWeb)))
  {
    printf("%s*%s", KGRN, RESET);
    g_File.DeleteFile(strCacheFile + ".version");
    g_File.DeleteFile(strCacheFile + ".time");

    long result = curlURLToCache(strCacheFile, strURL, strBuffer);
    if (result < 200 || result >= 400)
      return "";

    if (strWebFileVersion != "")
      g_File.WriteFileFromStr(strCacheFile + ".version", strWebFileVersion);

    g_File.WriteNowToFileAgeFile(strCacheFile);
  }
  else
  {
    strBuffer = g_File.ReadFileToStr(strCacheFile);
    if (bCacheFileExpired)
      printf ("%s-%s", KCYN, RESET);
    else
      printf ("%s.%s", KYEL, RESET);
  }

  return strBuffer;
};

long CHTTPHandler::curlURLToCache(std::string strCacheFile, std::string strURL, std::string &strBuffer)
{
  CURLcode curlResult;
  strURL = URLEncode(strURL);

  CLoginData LoginData = GetCredentials(strURL);

    if(m_curlHandle) 
    {
      int nretry = 0;
      bool bSuccess;
      long http_code = 0;
      do
      {
        strBuffer.clear();
        if (nretry > 0)
          HTTPRetry(nretry);

        curl_easy_setopt(m_curlHandle, CURLOPT_URL, strURL.c_str());
        curl_easy_setopt(m_curlHandle, CURLOPT_WRITEFUNCTION, Write_CurlData_String);
        if (!LoginData.strLogin.empty())
        {
          curl_easy_setopt(m_curlHandle, CURLOPT_USERNAME, LoginData.strLogin.c_str());
          curl_easy_setopt(m_curlHandle, CURLOPT_PASSWORD, LoginData.strPassword.c_str());
        }
        curl_easy_setopt(m_curlHandle, CURLOPT_FAILONERROR, true);
        curl_easy_setopt(m_curlHandle, CURLOPT_WRITEDATA, &strBuffer);
        curl_easy_setopt(m_curlHandle, CURLOPT_USERAGENT, strUserAgent.c_str());
        curl_easy_setopt(m_curlHandle, CURLOPT_SSL_VERIFYPEER, 0);
//        curl_easy_setopt(m_curlHandle, CURLOPT_SSL_VERIFYHOST, 0);
        curl_easy_setopt(m_curlHandle, CURLOPT_VERBOSE, 0);
//        curl_easy_setopt(m_curlHandle, CURLOPT_SSLVERSION, 3);
        curl_easy_setopt(m_curlHandle, CURLOPT_FOLLOWLOCATION, true);

        curlResult = curl_easy_perform(m_curlHandle);
        curl_easy_getinfo (m_curlHandle, CURLINFO_RESPONSE_CODE, &http_code);
        nretry++;
        bSuccess = (curlResult == 0 && http_code >= 200 && http_code < 400);
      }
      while (nretry < 5 && !bSuccess);

      if (bSuccess)
        CLog::Log(logINFO, "HTTPHandler: curlURLToCache finished with success from URL %s to cachefile %s, read filesize: %ibytes",
                  strURL.c_str(), strCacheFile.c_str(), strBuffer.size());
      else
        CLog::Log(logERROR, "HTTPHandler: curlURLToCache finished with error: \ncurl error: %i, %s\nhttp error: %i%s\nURL: %s\nlocaldir: %s",
                  curlResult, curl_easy_strerror(curlResult), http_code, GetHTTPErrorFromCode(http_code).c_str(),  strURL.c_str(), strCacheFile.c_str());

      g_File.WriteFileFromStr(strCacheFile, strBuffer);
      return http_code;
    }
    else
      CLog::Log(logERROR, "HTTPHandler: curlURLToCache failed because Curl was not initalized");
    return 0;
};

void CHTTPHandler::ReInit()
{
  if (!m_curlHandle)
    m_curlHandle = curl_easy_init();
  else
    CLog::Log(logWARNING, "HTTPHandler: Trying to reinitalize an already existing Curl handle");
};

void CHTTPHandler::Cleanup()
{
  if (m_curlHandle)
  {
    curl_easy_cleanup(m_curlHandle);
    m_curlHandle = NULL;
  }
};

size_t Write_CurlData_File(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  size_t written = fwrite(ptr, size, nmemb, stream);
  return written;
};

size_t Read_CurlData_File(char *bufptr, size_t size, size_t nitems, FILE *stream) 
{
  size_t read;
  read = fread(bufptr, size, nitems, stream);
  return read;
}


size_t Write_CurlData_String(char *data, size_t size, size_t nmemb, string *buffer)
{
  size_t written = 0;
  if(buffer != NULL)
  {
    buffer -> append(data, size * nmemb);
    written = size * nmemb;
  }
  return written;
}

typedef struct
{ 
  std::string * pPOString; 
  size_t pos; 
} Tputstrdata; 


size_t Read_CurlData_String(void * ptr, size_t size, size_t nmemb, void * stream)
{
  if (stream)
  {
    Tputstrdata * pPutStrData = (Tputstrdata*) stream; 

    size_t available = (pPutStrData->pPOString->size() - pPutStrData->pos);

    if (available > 0)
    {
      size_t written = std::min(size * nmemb, available);
      memcpy(ptr, &pPutStrData->pPOString->at(pPutStrData->pos), written); 
      pPutStrData->pos += written;
      return written; 
    }
  }

  return 0; 
}


void CHTTPHandler::SetCacheDir(std::string strCacheDir)
{
  if (!g_File.DirExists(strCacheDir))
    g_File.MakeDir(strCacheDir);
  m_strCacheDir = strCacheDir + DirSepChar;
  CLog::Log(logINFO, "HTTPHandler: Cache directory set to: %s", strCacheDir.c_str());
};

std::string CHTTPHandler::CacheFileNameFromURL(std::string strURL)
{
  std::string strResult;
  std::string strCharsToKeep  = "/.-=_() ";
  std::string strReplaceChars = "/.-=_() ";

  std::string hexChars = "01234567890abcdef"; 

  for (std::string::iterator it = strURL.begin(); it != strURL.end(); it++)
  {
    if (isalnum(*it))
      strResult += *it;
    else
    {
      size_t pos = strCharsToKeep.find(*it);
      if (pos != std::string::npos)
        strResult += strReplaceChars[pos];
      else
      {
        strResult += "%";
        strResult += hexChars[(*it >> 4) & 0x0F];
        strResult += hexChars[*it & 0x0F];
      }
    }
  }

  if (strResult.at(strResult.size()-1) == '/')
    strResult[strResult.size()-1] = '-';

  return strResult;
};

bool CHTTPHandler::LoadCredentials (std::string CredentialsFilename)
{
  TiXmlDocument xmlPasswords;

  if (!xmlPasswords.LoadFile(CredentialsFilename.c_str()))
  {
    CLog::Log(logINFO, "HTTPHandler: No \".passwords.xml\" file exists in project dir. No password protected web download will be available.");
    return false;
  }

  CLog::Log(logINFO, "HTTPHandler: Succesfuly found the .passwsords.xml file: %s", CredentialsFilename.c_str());

  TiXmlElement* pRootElement = xmlPasswords.RootElement();

  if (!pRootElement || pRootElement->NoChildren() || pRootElement->ValueTStr()!="websites")
  {
    CLog::Log(logWARNING, "HTTPHandler: No root element called \"websites\" in xml file.");
    return false;
  }

  CLoginData LoginData;

  const TiXmlElement *pChildElement = pRootElement->FirstChildElement("website");
  while (pChildElement && pChildElement->FirstChild())
  {
    std::string strWebSitePrefix = pChildElement->Attribute("prefix");
    if (pChildElement->FirstChild())
    {
      const TiXmlElement *pChildLOGINElement = pChildElement->FirstChildElement("login");
      if (pChildLOGINElement && pChildLOGINElement->FirstChild())
        LoginData.strLogin = pChildLOGINElement->FirstChild()->Value();
      const TiXmlElement *pChildPASSElement = pChildElement->FirstChildElement("password");
      if (pChildPASSElement && pChildPASSElement->FirstChild())
        LoginData.strPassword = pChildPASSElement->FirstChild()->Value();

      m_mapLoginData [strWebSitePrefix] = LoginData;
      CLog::Log(logINFO, "HTTPHandler: found login credentials for website prefix: %s", strWebSitePrefix.c_str());
    }
    pChildElement = pChildElement->NextSiblingElement("website");
  }

  return true;
};

CLoginData CHTTPHandler::GetCredentials (std::string strURL)
{
  CLoginData LoginData;
  for (itMapLoginData = m_mapLoginData.begin(); itMapLoginData != m_mapLoginData.end(); itMapLoginData++)
  {
    if (strURL.find(itMapLoginData->first) != std::string::npos)
    {
      LoginData = itMapLoginData->second;
      return LoginData;
    }
  }

  return LoginData;
};

std::string CHTTPHandler::URLEncode (std::string strURL)
{
  std::string strOut;
  for (std::string::iterator it = strURL.begin(); it != strURL.end(); it++)
  {
    if (*it == ' ')
      strOut += "%20";
    else
      strOut += *it;
  }
  return strOut;
}



bool CHTTPHandler::PutFileToURL(std::string const &sPOFile, std::string const &strURL, bool &bUploaded,
                                size_t &iAddedNew, size_t &iUpdated)
{
  std::string strCacheFileName = CacheFileNameFromURL(strURL);
  strCacheFileName = m_strCacheDir + "PUT/" + strCacheFileName;

  std::string sCacheFile;
  if (g_File.FileExist(strCacheFileName))
    sCacheFile = g_File.ReadFileToStr(strCacheFileName);

  if (sCacheFile == sPOFile)
  {
    CLog::Log(logINFO, "HTTPHandler::PutFileToURL: not necesarry to upload file as it has not changed from last upload.");
    return true;
  }


  long result = curlPUTPOStrToURL(sPOFile, strURL, iAddedNew, iUpdated);
  if (result < 200 || result >= 400)
  {
    CLog::Log(logERROR, "HTTPHandler::PutFileToURL: File upload was unsuccessful, http errorcode: %i", result);
    return false;
  }

  CLog::Log(logINFO, "HTTPHandler::PutFileToURL: File upload was successful so creating a copy at the .httpcache directory");
  g_File.WriteFileFromStr(strCacheFileName, sPOFile);

  bUploaded = true;

  return true;
};

long CHTTPHandler::curlPUTPOStrToURL(std::string const &strPOFile, std::string const &cstrURL, size_t &stradded, size_t &strupd)
{
  CURLcode curlResult;

  std::string strURL = URLEncode(cstrURL);

  std::string strServerResp;
  CLoginData LoginData = GetCredentials(strURL);

  if(m_curlHandle) 
  {
    int nretry = 0;
    bool bSuccess;
    long http_code = 0;
    do
    {
      strServerResp.clear();
      if (nretry > 0)
        HTTPRetry(nretry);

      struct curl_httppost *post1;
      struct curl_httppost *postend;

      post1 = NULL;
      postend = NULL;
      curl_formadd(&post1, &postend,
                  CURLFORM_COPYNAME, "file",
                  CURLFORM_BUFFER, "strings.po",
                  CURLFORM_BUFFERPTR, &strPOFile[0],
                  CURLFORM_BUFFERLENGTH, strPOFile.size(),
                  CURLFORM_CONTENTTYPE, "application/octet-stream",
                  CURLFORM_END);

      curl_easy_setopt(m_curlHandle, CURLOPT_WRITEFUNCTION, Write_CurlData_String);
      curl_easy_setopt(m_curlHandle, CURLOPT_URL, strURL.c_str());
      curl_easy_setopt(m_curlHandle, CURLOPT_NOPROGRESS, 1L);
      curl_easy_setopt(m_curlHandle, CURLOPT_HEADER, 1L);
      curl_easy_setopt(m_curlHandle, CURLOPT_FOLLOWLOCATION, 1L);
      curl_easy_setopt(m_curlHandle, CURLOPT_HTTPPOST, post1);
      curl_easy_setopt(m_curlHandle, CURLOPT_USERAGENT, strUserAgent.c_str());
      curl_easy_setopt(m_curlHandle, CURLOPT_MAXREDIRS, 50L);
      curl_easy_setopt(m_curlHandle, CURLOPT_CUSTOMREQUEST, "PUT");

      if (!LoginData.strLogin.empty())
      {
        curl_easy_setopt(m_curlHandle, CURLOPT_USERNAME, LoginData.strLogin.c_str());
        curl_easy_setopt(m_curlHandle, CURLOPT_PASSWORD, LoginData.strPassword.c_str());
      }
      curl_easy_setopt(m_curlHandle, CURLOPT_WRITEDATA, &strServerResp);
      curl_easy_setopt(m_curlHandle, CURLOPT_SSL_VERIFYPEER, 0);
      curl_easy_setopt(m_curlHandle, CURLOPT_VERBOSE, 0);
      curl_easy_setopt(m_curlHandle, CURLOPT_SSL_VERIFYHOST, 0);

      curlResult = curl_easy_perform(m_curlHandle);

      curl_easy_getinfo (m_curlHandle, CURLINFO_RESPONSE_CODE, &http_code);

      curl_formfree(post1);
      post1 = NULL;

      bSuccess = (curlResult == 0 && http_code >= 200 && http_code < 400);
      nretry++;
    }
    while (nretry < 5 && !bSuccess);

    if (bSuccess)
      CLog::Log(logINFO, "HTTPHandler::curlFileToURL finished with success from File to URL %s", strURL.c_str());
    else
      CLog::Log(logERROR, "HTTPHandler::curlFileToURL finished with error: \ncurl error: %i, %s\nhttp error: %i%s\nURL: %s\n",
                curlResult, curl_easy_strerror(curlResult), http_code, GetHTTPErrorFromCode(http_code).c_str(), strURL.c_str());

    size_t jsonPos = strServerResp.find_first_of("{");
    if (jsonPos == std::string::npos)
      CLog::Log(logERROR, "HTTPHandler::curlFileToURL no valid Transifex server response received");

    strServerResp = strServerResp.substr(jsonPos);
    ParseUploadedStringsData(strServerResp, stradded, strupd);

    return http_code;
  }
  else
    CLog::Log(logERROR, "HTTPHandler::curlFileToURL failed because Curl was not initalized");
  return 700;
};

// bool CHTTPHandler::ComparePOFiles(std::string strPOFilePath1, std::string strPOFilePath2) const
// {
//  CPOHandler POHandler1, POHandler2;
//  POHandler1.ParsePOStrToMem(g_File.ReadFileToStr(strPOFilePath1), strPOFilePath1);
//  POHandler2.ParsePOStrToMem(g_File.ReadFileToStr(strPOFilePath2), strPOFilePath2);
//  return ComparePOFilesInMem(&POHandler1, &POHandler2, false);
//}

/*
  for (size_t POEntryIdx = 0; POEntryIdx != pPOHandler1->GetNumEntriesCount(); POEntryIdx++)
  {
    CPOEntry POEntry1 = *(pPOHandler1->GetNumPOEntryByIdx(POEntryIdx));
    const CPOEntry * pPOEntry2 = pPOHandler2->GetNumPOEntryByID(POEntry1.numID);
    if (!pPOEntry2)
      return false;
    CPOEntry POEntry2 = *pPOEntry2;

    if (bLangIsEN)
    {
      POEntry1.msgStr.clear();
      POEntry1.msgStrPlural.clear();
      POEntry2.msgStr.clear();
      POEntry2.msgStrPlural.clear();
    }

    if (!(POEntry1 == POEntry2))
      return false;
  }
*/

bool CHTTPHandler::CreateNewResource(const std::string& sPOFile, const CXMLResdata& XMLResData, size_t &iAddedNew)
{

  std::string sURLCreateRes = "https://www.transifex.com/api/2/project/" + XMLResData.strTargetProjectName + "/resources/";

  std::string sURLSRCRes = "https://www.transifex.com/api/2/project/" + XMLResData.strTargetProjectName + "/resource/" +
                           XMLResData.strTargetTXName + "/translation/" +
                           g_LCodeHandler.GetLangFromLCode(XMLResData.strSourceLcode, XMLResData.strTargTXLFormat) + "/";

  std::string sCacheFile = CacheFileNameFromURL(sURLSRCRes);
  sCacheFile = m_strCacheDir + "PUT/" + sCacheFile;

  CURLcode curlResult;

  sURLCreateRes = URLEncode(sURLCreateRes);

  std::string strPOJson = CreateNewresJSONStrFromPOStr(XMLResData.strTXName, sPOFile);

  std::string strServerResp;
  CLoginData LoginData = GetCredentials(sURLCreateRes);

  if(m_curlHandle)
  {
    int nretry = 0;
    bool bSuccess;
    long http_code = 0;
    do
    {
      Tputstrdata PutStrData;
      PutStrData.pPOString = &strPOJson;
      PutStrData.pos = 0;
      strServerResp.clear();

      if (nretry > 0)
        HTTPRetry(nretry);

      struct curl_slist *headers=NULL;
      headers = curl_slist_append( headers, "Content-Type: application/json");
      headers = curl_slist_append( headers, "charsets: utf-8");

      curl_easy_setopt(m_curlHandle, CURLOPT_READFUNCTION, Read_CurlData_String);
      curl_easy_setopt(m_curlHandle, CURLOPT_WRITEFUNCTION, Write_CurlData_String);
      curl_easy_setopt(m_curlHandle, CURLOPT_URL, sURLCreateRes.c_str());
      curl_easy_setopt(m_curlHandle, CURLOPT_POST, 1L);
      if (!LoginData.strLogin.empty())
      {
        curl_easy_setopt(m_curlHandle, CURLOPT_USERNAME, LoginData.strLogin.c_str());
        curl_easy_setopt(m_curlHandle, CURLOPT_PASSWORD, LoginData.strPassword.c_str());
      }
      curl_easy_setopt(m_curlHandle, CURLOPT_FAILONERROR, true);
      curl_easy_setopt(m_curlHandle, CURLOPT_READDATA, &PutStrData);
      curl_easy_setopt(m_curlHandle, CURLOPT_WRITEDATA, &strServerResp);
      curl_easy_setopt(m_curlHandle, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)strPOJson.size());
      curl_easy_setopt(m_curlHandle, CURLOPT_USERAGENT, strUserAgent.c_str());
      curl_easy_setopt(m_curlHandle, CURLOPT_HTTPHEADER, headers);
      curl_easy_setopt(m_curlHandle, CURLOPT_SSL_VERIFYPEER, 0);
      curl_easy_setopt(m_curlHandle, CURLOPT_SSL_VERIFYHOST, 0);
      curl_easy_setopt(m_curlHandle, CURLOPT_VERBOSE, 0);

      curlResult = curl_easy_perform(m_curlHandle);

      curl_easy_getinfo (m_curlHandle, CURLINFO_RESPONSE_CODE, &http_code);

      bSuccess = (curlResult == 0 && http_code >= 200 && http_code < 400);
      nretry++;
    }
    while (nretry < 5 && !bSuccess);

    if (bSuccess)
      CLog::Log(logINFO, "CHTTPHandler::CreateNewResource finished with success for resource %s from EN PO file %s to URL %s",
                XMLResData.strResName.c_str(), sURLSRCRes.c_str(), sURLCreateRes.c_str());
    else
      CLog::Log(logERROR, "CHTTPHandler::CreateNewResource finished with error:\ncurl error: %i, %s\nhttp error: %i%s\nURL: %s\nlocaldir: %s\nREsource: %s",
                curlResult, curl_easy_strerror(curlResult), http_code, GetHTTPErrorFromCode(http_code).c_str(), sURLCreateRes.c_str(),
                sURLSRCRes.c_str(), XMLResData.strResName.c_str());

    g_File.CopyFile(sURLCreateRes, sCacheFile);
    ParseUploadedStrForNewRes(strServerResp, iAddedNew);

    return true;
  }
  else
    CLog::Log(logERROR, "CHTTPHandler::CreateNewResource failed because Curl was not initalized");

  return false;
};

void CHTTPHandler::DeleteCachedFile (std::string const &strURL, std::string strPrefix)
{
  std::string strCacheFile = CacheFileNameFromURL(strURL);

  strCacheFile = m_strCacheDir + strPrefix + "/" + strCacheFile;
  if (g_File.FileExist(strCacheFile))
    g_File.DeleteFile(strCacheFile);
}

void CHTTPHandler::GetGithubData (const std::string &strURL, CGithubURLData &GithubURLData)
{
  if (strURL.find("//") >> 7)
    CLog::Log(logERROR, "CHTTPHandler::ParseGitHUBURL: Internal error: // found in Github URL");

  size_t pos1, pos2, pos3, pos4, pos5;

  if (strURL.find("raw.github.com/") != std::string::npos)
    pos1 = strURL.find("raw.github.com/")+15;
  else if (strURL.find("raw2.github.com/") != std::string::npos)
    pos1 = strURL.find("raw2.github.com/")+16;
  else if (strURL.find("raw.githubusercontent.com/") != std::string::npos)
    pos1 = strURL.find("raw.githubusercontent.com/")+26;
  else
    CLog::Log(logERROR, "ResHandler: Wrong Github URL format given");

  pos2 = strURL.find("/", pos1+1);
  pos3 = strURL.find("/", pos2+1);
  pos4 = strURL.find("/", pos3+1);

  GithubURLData.strOwner = strURL.substr(pos1, pos2-pos1);
  GithubURLData.strRepo = strURL.substr(pos2, pos3-pos2);
  GithubURLData.strPath = strURL.substr(pos4, strURL.size() - pos4 - 1);
  GithubURLData.strGitBranch = strURL.substr(pos3+1, pos4-pos3-1);

  if ((pos5 = GithubURLData.strPath.find_last_of("(")) != std::string::npos)
  {
    GithubURLData.strPath = GithubURLData.strPath.substr(0,pos5);
    GithubURLData.strPath = GithubURLData.strPath.substr(0, GithubURLData.strPath.find_last_of("/"));
  }
}

std::string CHTTPHandler::GetGitHUBAPIURL(std::string const & strURL)
{
  CGithubURLData GithubURLData;
  GetGithubData(strURL, GithubURLData);

  std::string strGitHubURL = "https://api.github.com/repos/" + GithubURLData.strOwner + GithubURLData.strRepo;
  strGitHubURL += "/contents";
  strGitHubURL += GithubURLData.strPath;
  strGitHubURL += "?ref=" + GithubURLData.strGitBranch;

  return strGitHubURL;
}

void CHTTPHandler::GetGitCloneURL(std::string const & strURL, std::string &strGitHubURL, CGithubURLData &GithubURLData)
{
  GetGithubData(strURL, GithubURLData);
  strGitHubURL = "git@github.com:" + GithubURLData.strOwner + GithubURLData.strRepo + ".git";
}

bool CHTTPHandler::UploadTranslatorsDatabase(std::string strJson, std::string strURL)
{

  CURLcode curlResult;

  strURL = URLEncode(strURL);


  std::string strServerResp;
  CLoginData LoginData = GetCredentials(strURL);

  if(m_curlHandle) 
  {
    int nretry = 0;
    bool bSuccess;
    long http_code = 0;
    do
    {
      Tputstrdata PutStrData;
      PutStrData.pPOString = &strJson;
      PutStrData.pos = 0;
      strServerResp.clear();

      if (nretry > 0)
        HTTPRetry(nretry);

      struct curl_slist *headers=NULL;
      headers = curl_slist_append( headers, "Content-Type: application/json");
      headers = curl_slist_append( headers, "Accept: text/plain");

      curl_easy_setopt(m_curlHandle, CURLOPT_READFUNCTION, Read_CurlData_String);
      curl_easy_setopt(m_curlHandle, CURLOPT_WRITEFUNCTION, Write_CurlData_String);
      curl_easy_setopt(m_curlHandle, CURLOPT_URL, strURL.c_str());
      curl_easy_setopt(m_curlHandle, CURLOPT_POST, 1L);
      if (!LoginData.strLogin.empty())
      {
        curl_easy_setopt(m_curlHandle, CURLOPT_USERNAME, LoginData.strLogin.c_str());
        curl_easy_setopt(m_curlHandle, CURLOPT_PASSWORD, LoginData.strPassword.c_str());
      }
      curl_easy_setopt(m_curlHandle, CURLOPT_FAILONERROR, true);
      curl_easy_setopt(m_curlHandle, CURLOPT_READDATA, &PutStrData);
      curl_easy_setopt(m_curlHandle, CURLOPT_WRITEDATA, &strServerResp);
      curl_easy_setopt(m_curlHandle, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)strJson.size());
      curl_easy_setopt(m_curlHandle, CURLOPT_USERAGENT, strUserAgent.c_str());
      curl_easy_setopt(m_curlHandle, CURLOPT_HTTPHEADER, headers);
      curl_easy_setopt(m_curlHandle, CURLOPT_SSL_VERIFYPEER, 0);
      curl_easy_setopt(m_curlHandle, CURLOPT_SSL_VERIFYHOST, 0);
      curl_easy_setopt(m_curlHandle, CURLOPT_VERBOSE, 0);
      //      curl_easy_setopt(m_curlHandle, CURLOPT_SSLVERSION, 3);

      curlResult = curl_easy_perform(m_curlHandle);

      curl_easy_getinfo (m_curlHandle, CURLINFO_RESPONSE_CODE, &http_code);

      bSuccess = (curlResult == 0 && http_code >= 200 && http_code < 400);
      nretry++;
    }
    while (nretry < 5 && !bSuccess);

//    if (bSuccess)
//      CLog::Log(logINFO, "CHTTPHandler::CreateNewResource finished with success for resource %s from EN PO file %s to URL %s",
//                strResname.c_str(), strENPOFilePath.c_str(), strURL.c_str());
//      else
//        CLog::Log(logERROR, "CHTTPHandler::CreateNewResource finished with error:\ncurl error: %i, %s\nhttp error: %i%s\nURL: %s\nlocaldir: %s\nREsource: %s",
//                  curlResult, curl_easy_strerror(curlResult), http_code, GetHTTPErrorFromCode(http_code).c_str(), strURL.c_str(), strENPOFilePath.c_str(), strResname.c_str());

    return http_code;
  }
  else
    CLog::Log(logERROR, "CHTTPHandler::CreateNewResource failed because Curl was not initalized");
  return 700;
};

std::string CHTTPHandler::CreateNewresJSONStrFromPOStr(std::string strTXResname, std::string const &strPO)
{
  Json::Value root;
  root["content"] = strPO;
  root["slug"] = std::string(strTXResname);
  root["name"] = std::string(strTXResname);
  root["i18n_type"] = std::string("PO");
  Json::StyledWriter writer;
  std::string strJSON = writer.write(root);
  return strJSON;
};

void CHTTPHandler::ParseUploadedStringsData(std::string const &strJSON, size_t &stradded, size_t &strupd)
{
  Json::Value root;   // will contains the root value after parsing.
  Json::Reader reader;

  bool parsingSuccessful = reader.parse(strJSON, root );
  if ( !parsingSuccessful )
  {
    CLog::Log(logERROR, "JSONHandler::ParseUploadedStringsData: Parse upload tx server response: no valid JSON data");
    return;
  }

  stradded = root.get("strings_added", 0).asInt();
  strupd = root.get("strings_updated", 0).asInt();
  return;
};

void CHTTPHandler::ParseUploadedStrForNewRes(std::string const &strJSON, size_t &stradded)
{
  Json::Value root;   // will contains the root value after parsing.
  Json::Reader reader;

  bool parsingSuccessful = reader.parse(strJSON, root );
  if ( !parsingSuccessful )
  {
    CLog::Log(logERROR, "JSONHandler::ParseUploadedStrForNewRes: Parse upload tx server response: no valid JSON data");
    return;
  }

  stradded = root[0].asInt();

  return;
};
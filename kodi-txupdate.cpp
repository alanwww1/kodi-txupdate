/*
 *      Copyright (C) 2014 Team Kodi
 *      http://xbmc.org
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

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#if defined( WIN32 ) && defined( TUNE )
  #include <crtdbg.h>
  _CrtMemState startMemState;
  _CrtMemState endMemState;
#endif

#include <string>
#include <stdio.h>
#include "lib/ProjectHandler.h"
#include "lib/HTTPUtils.h"
#include "lib/Langcodes.h"
#include "jsoncpp/json/json.h"
#include "lib/Log.h"
#include "lib/FileUtils/FileUtils.h"

using namespace std;

void PrintUsage()
{
  printf
  (
  "Usage: kodi-txpudate PROJECTDIR [working mode]\n\n"
  "PROJECTDIR: the directory which contains the kodi-txupdate.xml settings file and the .passwords file.\n"
  "            This will be the directory where your merged and transifex update files get generated.\n\n"
  "Working modes:\n"
  "     -d   Only download to local cache, without performing a merge.\n"
  "     -dm  Download and create merged and tx update files, but no upload performed.\n"
  "     -dmu Download, merge and upload the new files to transifex.\n"
  "     -u   Only upload the previously prepared files. Note that this needs download and merge ran before.\n\n"
  "     No working mode arguments used, performs as -dm\n\n"
  );
  #ifdef _MSC_VER
  printf
  (
  "Note for Windows users: In case you have whitespace or any special character\n"
  "in the directory argument, please use apostrophe around them. For example:\n"
  "kodi-txupdate.exe \"C:\\kodi dir\\language\"\n"
  );
  #endif
  return;
};

int main(int argc, char* argv[])
{
  setbuf(stdout, NULL);
  if (argc > 3 || argc < 2)
  {
    printf ("\nBad arguments given\n\n");
    PrintUsage();
    return 1;
  }

  std::string WorkingDir, strMode;
  bool bDownloadNeeded = false;
  bool bMergeNeeded = false;
  bool bUploadNeeded = false;
  bool bTransferTranslators = false;
  bool bInfiniteCacheTime = false;

  if (argv[1])
   WorkingDir = argv[1];
  if (WorkingDir.empty() || !g_File.DirExists(WorkingDir))
  {
    printf ("\nMissing or wrong project directory specified: %s, stopping.\n\n", WorkingDir.c_str());
    PrintUsage();
    return 1;
  }

  if (argc == 3)
  {
    if (argv[2])
      strMode = argv[2];
    if (strMode.empty() && strMode[0] != '-')
    {
      printf ("\nMissing or wrong working mode format used. Stopping.\n\n");
      PrintUsage();
      return 1;
    }
    if (strMode == "-d")
      bDownloadNeeded = true;
    else if (strMode == "-dm" || strMode == "-m" || strMode == "-dmic")
      {
        bDownloadNeeded = true; bMergeNeeded = true;
        if (strMode == "-dmic")
          bInfiniteCacheTime = true;
      }
    else if (strMode == "-dmu")
      {bDownloadNeeded = true; bMergeNeeded = true; bUploadNeeded = true;}
    else if (strMode == "-u")
      bUploadNeeded = true;
    else if (strMode == "-ttr")
      bTransferTranslators = true;

    else
    {
      printf ("\nWrong working mode arguments used. Stopping.\n\n");
      PrintUsage();
      return 1;
    }
  }
  if (argc == 2)
    {bDownloadNeeded = true; bMergeNeeded = true;}

  printf("\nKODI-TXUPDATE v%s by Team Kodi\n", VERSION.c_str());

  try
  {
    if (WorkingDir[WorkingDir.length()-1] != DirSepChar)
      WorkingDir.append(&DirSepChar);

    CLog::SetbWriteSyntaxLog(bDownloadNeeded);
    CLog::Init(WorkingDir + "kodi-txupdate.log", WorkingDir + "txupdate-syntax.log");
    CLog::Log(logINFO, "Root Directory: %s", WorkingDir.c_str());

    g_HTTPHandler.LoadCredentials(WorkingDir + ".passwords.xml");
    g_HTTPHandler.SetCacheDir(WorkingDir + ".httpcache");

    CProjectHandler TXProject;
    TXProject.SetProjectDir(WorkingDir);
    TXProject.LoadUpdXMLToMem();

    if (bInfiniteCacheTime)
      g_HTTPHandler.SetHTTPCacheExpire ((size_t)-1);

    TXProject.InitLCodeHandler();

    if (bDownloadNeeded && !bTransferTranslators)
    {
//      if (!g_File.FileExist(WorkingDir + ".httpcache" + DirSepChar + ".lastdloadtime") ||
//          g_File.GetFileAge(WorkingDir + ".httpcache" + DirSepChar + ".lastdloadtime") > g_Settings.GetHTTPCacheExpire() * 60)
//      {
//        g_File.DeleteDirectory(WorkingDir + ".httpcache"); // Clean the complete cache as all our files in there are outdated
//        g_HTTPHandler.SetCacheDir(WorkingDir + ".httpcache");
//      }

      g_File.WriteFileFromStr(WorkingDir + ".httpcache" + DirSepChar + ".dload_merge_status", "fail");
      g_File.WriteFileFromStr(WorkingDir + ".httpcache" + DirSepChar + ".lastdloadtime", "Last download time: " + g_File.GetCurrTime());

      printf("\n\n%s", KGRN);
      printf("----------------------------------------\n");
      printf("DOWNLOADING RESOURCES FROM TRANSIFEX.NET\n");
      printf("----------------------------------------%s\n", RESET);

      CLog::Log(logLINEFEED, "");
      CLog::Log(logINFO, "****************************************");
      CLog::Log(logINFO, "DOWNLOADING RESOURCES FROM TRANSIFEX.NET");
      CLog::Log(logINFO, "****************************************");

      TXProject.FetchResourcesFromTransifex();

      printf("\n%s", KGRN);
      printf("-----------------------------------\n");
      printf("DOWNLOADING RESOURCES FROM UPSTREAM\n");
      printf("-----------------------------------%s\n", RESET);

      CLog::Log(logLINEFEED, "");
      CLog::Log(logINFO, "***********************************");
      CLog::Log(logINFO, "DOWNLOADING RESOURCES FROM UPSTREAM");
      CLog::Log(logINFO, "***********************************");

      TXProject.FetchResourcesFromUpstream();

      if (bMergeNeeded)
      {

        TXProject.CreateMergedResources();

        printf("\n%s", KGRN);
        printf("--------------------------------------------\n");
        printf("WRITING MERGED AND TXUPDATE RESOURCES TO HDD\n");
        printf("--------------------------------------------%s\n", RESET);

        CLog::Log(logLINEFEED, "");
        CLog::Log(logINFO, "********************************************");
        CLog::Log(logINFO, "WRITING MERGED AND TXUPDATE RESOURCES TO HDD");
        CLog::Log(logINFO, "********************************************");

        TXProject.WriteResourcesToFile(WorkingDir);
        g_File.CopyFile(WorkingDir + "kodi-txupdate.xml", WorkingDir + ".httpcache" + DirSepChar + ".last_kodi-txupdate.xml");
      }
      g_File.WriteFileFromStr(WorkingDir + ".httpcache" + DirSepChar + ".dload_merge_status", "ok");
    }

    bUploadNeeded = true;
    if (bUploadNeeded && !bTransferTranslators)
    {

      printf("\n%s", KGRN);
      printf("-----------------------------------------\n");
      printf("UPLOADING LANGUAGE FILES TO TRANSIFEX.NET\n");
      printf("-----------------------------------------%s\n", RESET);

      TXProject.UploadTXUpdateFiles(WorkingDir);
    }
    if (bTransferTranslators)
    {
      printf("\n%s", KGRN);
      printf("-------------------------------\n");
      printf("GET TRANSLATION GROUPS\n");
      printf("-------------------------------%s\n", RESET);

      TXProject.MigrateTranslators();
    }

    CLog::SetbWriteSyntaxLog(bDownloadNeeded);

    if (CLog::GetWarnCount() ==0)
    {
      printf("\n%s", KGRN);
      printf("--------------------------------------------\n");
      printf("PROCESS FINISHED SUCCESFULLY WITHOUT WARNINGS\n");
      printf("SYNTAX WARNINGS %s%i%s\n", KRED, CLog::GetSyntaxWarnCount(), KGRN);
      printf("--------------------------------------------%s\n\n", RESET);
    }
    else
    {
      printf("\n");
      printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
      printf("PROCESS FINISHED WITH %i WARNINGS\n", CLog::GetWarnCount());
      printf("SYNTAX WARNINGS %i\n", CLog::GetSyntaxWarnCount());
      printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n");
    }

    CLog::Close();
    g_HTTPHandler.Cleanup();
    return 0;
  }
  catch (const int calcError)
  {
    g_HTTPHandler.Cleanup();
    CLog::Close();
    return 100;
  }
}

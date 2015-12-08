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
  CLog::Log(logPRINT,
  "Usage: kodi-txpudate PROJECTDIR (CMDARG)\n\n"
  "PROJECTDIR: the directory which contains the kodi-txupdate.conf settings file.\n"
  "            This will be the directory where your merged and transifex update files get generated.\n"
  "CMDARG:     Command line argument to control special modes. Available:\n"
  "            -c or --ForceUseCache : ensures to use the previously cached UPS, TRX files.\n\n"

  );
  return;
};

int main(int argc, char* argv[])
{
  setbuf(stdout, NULL);
  if (argc > 3 || argc < 2)
  {
    CLog::Log(logPRINT, "\nBad arguments given\n\n");
    PrintUsage();
    return 1;
  }

  std::string WorkingDir, sCMDOption;

  bool bTransferTranslators = false; // In a VERY special case when transfer of translator database needed, set this to true
  bool bForceUseCache = false;

  if (argv[1])
   WorkingDir = argv[1];
  if (WorkingDir.empty() || !g_File.DirExists(WorkingDir))
  {
    CLog::Log(logPRINT, "\nMissing or wrong project directory specified: %s, stopping.\n\n", WorkingDir.c_str());
    PrintUsage();
    return 1;
  }

  if (argc == 3) //we have an additional command line argument
  {
    if ( argv[2])
      sCMDOption = argv[2];
    if (WorkingDir.empty())
    {
      CLog::Log(logPRINT, "\nMissing command line argument, stopping.\n\n");
      PrintUsage();
      return 1;
    }
    if (sCMDOption == "-c" || sCMDOption == "--ForceUseCache")
      bForceUseCache = true;
    else
    {
      CLog::Log(logPRINT, "\nWrong command line argument: %s, stopping.\n\n", sCMDOption.c_str());
      PrintUsage();
      return 1;
    }
  }

  if (WorkingDir.find('/') != 0) //We have a relative path, make it absolute
  {
    std::string sCurrentPath = g_File.getcwd_string();
    WorkingDir = sCurrentPath + "/" + WorkingDir;
  }

  CLog::Log(logPRINT, "\nKODI-TXUPDATE v%s by Team Kodi\n", VERSION.c_str());

  try
  {
    if (WorkingDir[WorkingDir.length()-1] != DirSepChar)
      WorkingDir.append(&DirSepChar);

    CLog::Log(logDEBUG, "Root Directory: %s", WorkingDir.c_str());

    std::string sHomePath = g_File.GetHomePath();

    g_HTTPHandler.LoadCredentials(sHomePath + "/.config/kodi-txupdate/passwords.xml");
    g_HTTPHandler.SetCacheDir(WorkingDir + ".httpcache");

    CProjectHandler TXProject;
    TXProject.SetProjectDir(WorkingDir);
    TXProject.LoadConfigToMem(bForceUseCache);

    TXProject.InitLCodeHandler();

    if (!bTransferTranslators)
    {
      CLog::Log(LogHEADLINE, "DOWNLOADING RESOURCES FROM TRANSIFEX.NET\n");
      TXProject.FetchResourcesFromTransifex();

      CLog::Log(LogHEADLINE, "DOWNLOADING RESOURCES FROM UPSTREAM\n");
      TXProject.FetchResourcesFromUpstream();

      TXProject.CreateMergedResources();

      CLog::Log(LogHEADLINE, "WRITING MERGED RESOURCES TO HDD\n");
      TXProject.WriteResourcesToFile(WorkingDir);

      CLog::Log(LogHEADLINE, "UPLOADING LANGUAGE FILES TO TRANSIFEX.NET\n");
      TXProject.UploadTXUpdateFiles(WorkingDir);

      CLog::Log(LogHEADLINE, "WRITING RESOURCES TO LOCAL GITHUB REPOS\n");
      TXProject.WriteResourcesToLOCGitRepos(WorkingDir);
    }

    if (bTransferTranslators)
    {
      CLog::Log(LogHEADLINE, "GET TRANSLATION GROUPS\n");
      TXProject.MigrateTranslators();
    }

    CLog::Log(LogHEADLINE, "CLEANING CACHE FILE DIRECTORY FROM UNUSED FILES\n");
    g_HTTPHandler.CleanCacheFiles();

    if (CLog::GetWarnCount() ==0)
      CLog::Log(LogHEADLINE, "PROCESS FINISHED SUCCESFULLY WITHOUT WARNINGS\n");
    else
      CLog::Log(LogHEADLINE, "%sPROCESS FINISHED WITH %i WARNINGS%s\n", KRED, CLog::GetWarnCount(), KGRN);

    g_HTTPHandler.Cleanup();
    return 0;
  }
  catch (const int calcError)
  {
    g_HTTPHandler.Cleanup();
    return 100;
  }
}

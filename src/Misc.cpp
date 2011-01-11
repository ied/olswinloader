//
// MISC.CPP
// Code contained in this file is hereby released to the public domain. 
// Ian Davis, 1/8/2011
//
#include "stdafx.h"
#include "Misc.h"

//
// Add slash to trailing edge of string if missing...
//
CString AddSlash (CString target)
{
  if (target.GetLength()>0)
    if (target.Right(1) != "\\") 
      target += "\\";
  return (target);
}


//
// Return path of EXE file...
//
CString GetProgramPath()
{
  char exepath[MAX_PATH];

  exepath[0]=0;
  int len = GetModuleFileName(GetModuleHandle(NULL),exepath,sizeof(exepath));

  // If did get EXE pathname, remove name of application (leaving just path)...
  // Should there be no path (just a drive designation), don't use it.
  if (len>0) {
    for (len = strlen(exepath); len>0; len--) {
      if (exepath[len-1]=='\\') {
        exepath[len] = 0;
        break;
      }
      if (exepath[len-1]==':') {
        exepath[0] = 0;
        break;
      }
    }
    if (len<1) exepath[0] = 0;
  }      

  // If can't get the EXE pathname, then use current path instead...
  if (strlen(exepath) == 0) 
    getcwd(exepath, _MAX_PATH);

  return (AddSlash(exepath));
}



//
// Convert any forward slashes into backslashes...
//
char *resolve_directory_separator(char *deststr, const char *srcstr)
{
  int i;
  for (i=0; srcstr[i]; i++) deststr[i] = (srcstr[i]=='/') ? '\\' : srcstr[i];
  deststr[i] = 0;
  return deststr;
}


void resolve_directory_separator(CString &deststr, CString srcstr)
{
  deststr = srcstr;
  resolve_directory_separator (deststr.GetBuffer(deststr.GetLength()+1), srcstr);
}



//
// Return path portion of given pathname...
//   ie:  Given "\aaa\bbb.txt", returns "\aaa\"
//
CString GetPathnamePortion(CString pathstr)
{
  CString path;
  int len;

  resolve_directory_separator(path,pathstr);

  int pathlen = path.GetLength();
  char *pathptr = path.GetBuffer(pathlen+1);

  for (len = pathlen; len>0; len--)
    if ((pathptr[len-1]=='\\') || (pathptr[len-1]==':')) {
      pathptr[len] = 0;
      break;
    }

  if (len<1) pathptr[0] = 0;
  path.ReleaseBuffer();
  return (path);
}


//
// Return filename portion of given pathname...
//   ie:  Given "\aaa\bbb.txt", returns "bbb.txt"
//
CString GetFilenamePortion(CString pathstr)
{
  CString path;
  int len;

  resolve_directory_separator(path,pathstr);
  for (len = strlen(path); len>0; len--)
    if ((path[len-1]=='\\') || (path[len-1]==':')) {
      CString temp(pathstr.Mid(len));
      path = temp;
      break;
    }

  return (path);
}


//
// Return base of filename portion of given pathname...
//   ie:  Given "\aaa.bbb\ccc.txt", returns "ccc"
//
CString GetBasenamePortion(CString pathstr)
{
  CString basename;
  resolve_directory_separator(basename,GetFilenamePortion(pathstr));

  int basenamelen = basename.GetLength();
  char *basenameptr = basename.GetBuffer(basenamelen+1);

  int i=basenamelen-1;
  while (i>=0) {
    if ((basenameptr[i]=='\\') || (basenameptr[i]==':')) {
      basename.Empty();
      return(basename);
    }
    if (basenameptr[i]=='.') {
      basenameptr[i]=0;
      basename.ReleaseBuffer();
      return(basename);
    }
    i--;
  }
  return (basename);
}


//
// Return base of filename portion of given pathname...
//   ie:  Given "\aaa.bbb\ccc.txt", returns "txt"
//
CString GetExtensionPortion(CString pathstr)
{
  CString basename;
  resolve_directory_separator(basename,GetFilenamePortion(pathstr));

  int i=strlen(basename)-1;
  while (i>=0) {
    if ((basename[i]=='\\') || (basename[i]==':')) {
      basename.Empty();
      return(basename);
    }
    if (basename[i]=='.') {
      CString temp;
      return basename.Mid(i);
    }
    i--;
  }
  return ("");
}



//
// Return extension given filetype...
//
CString GetFiletypeExtension(int resourceid)
{
  TCHAR szFullText[256];
  CString strFilterExt;
  AfxLoadString(resourceid, szFullText, sizeof(szFullText));
  AfxExtractSubString(strFilterExt, szFullText, 4, '\n');
  return strFilterExt;
}



//
// Get port string name from number...
//
CString GetPortDeviceName(int portnum)
{
  CString sPort;
  sPort.Format(_T("\\\\.\\COM%d"),portnum);
  return sPort;
}



//
// See if file exists...
//
bool FileExists (const char *targetfile)
{
	FILE *fileid = fopen(targetfile, "r");
	if (fileid == NULL) return false;
  fclose(fileid);
  return true;
}



//
// Look for entry in string table...
//
int strtablecompare (const char *tstr, strtabletype *strtable)
{
  int index = 0;
  int tlen = strlen(tstr);
  while (strlen(strtable->target)>0) {
    int stlen = strlen(strtable->target);
    if (tlen==stlen) {
      bool match = true;
      for (int i=0; match && (i<tlen); i++) {
        char tchar = toupper(tstr[i]);
        char stchar = toupper(strtable->target[i]);
        if (tchar=='_') tchar = '-';
        if (stchar=='_') stchar = '-';
        if (tchar != stchar) match = false;
      }
      if (match) return(strtable->result);
    }

    strtable++;
    index++;
  }

  return(-1);
}
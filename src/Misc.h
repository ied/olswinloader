#ifndef _MISC_H_
#define _MISC_H_

typedef struct strtabletype {
  char *target;
  int result;
} strtabletype;


CString AddSlash (CString target);
CString GetProgramPath();
char *resolve_directory_separator(char *deststr, const char *srcstr);
void resolve_directory_separator(CString &deststr, CString srcstr);
CString GetPathnamePortion(CString pathstr);
CString GetBasenamePortion(CString pathstr);
CString GetFilenamePortion(CString pathstr);
CString GetExtensionPortion(CString pathstr);
CString GetFiletypeExtension(int resourceid);
CString GetPortDeviceName (int portnum);
bool FileExists (const char *targetfile);
int strtablecompare (const char *tstr, strtabletype *strtable);

#endif // _MISC_H_
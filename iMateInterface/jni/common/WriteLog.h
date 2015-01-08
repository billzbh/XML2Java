#ifndef WRITELOG_H
#define WRITELOG_H

extern void vSetWriteLog(int iOnOff);
extern void vWriteLogHex(char *pszTitle, void *pLog, int iLength);
extern void vWriteLogTxt(char *pszFormat, ...);

#endif

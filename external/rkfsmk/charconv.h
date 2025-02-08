#ifndef _CHARCONV_H
#define _CHARCONV_H

#define DEFAULT_DOS_CODEPAGE 437

int SetDosCodepage(int codepage);
int DosCharToPrintable(char **p, unsigned char c);

#endif

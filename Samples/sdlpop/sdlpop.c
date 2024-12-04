#include "printf.h"
#include "common.h"
#include <xtl.h>

void _putchar(char character)
{
	DbgPrint("%c", character);
	// send char to console etc.
}

typedef struct _STRING {
	USHORT Length;
	USHORT MaximumLength;
	PCHAR  Buffer;
} STRING, * PSTRING;
typedef STRING ANSI_STRING, * PANSI_STRING;
typedef LONG NTSTATUS;
typedef const char* PCSZ, * PCSTR, * LPCSTR;
NTSYSAPI NTSTATUS NTAPI IoCreateSymbolicLink(IN PANSI_STRING SymbolicLinkName, IN PANSI_STRING DeviceName);
NTSYSAPI VOID NTAPI RtlInitAnsiString(OUT PANSI_STRING DestinationString, IN PCSZ SourceString);

int main(int argc, char* argv[])
{
	const char* source = "\\Device\\Harddisk0\\Partition1";
	const char* dest = "\\??\\E:";
	ANSI_STRING device_name;
	ANSI_STRING link_name;
	RtlInitAnsiString(&device_name, source);
	RtlInitAnsiString(&link_name, dest);
	IoCreateSymbolicLink(&link_name, &device_name);

#ifdef XBOX
	CreateDirectoryA("E:\\UDATA", NULL);
	CreateDirectoryA(savePath, NULL);
	CreateDirectoryA(settingsPath, NULL);
	CreateDirectoryA(popSavePath, NULL);
	CreateDirectoryA(scorePath, NULL);

	FILE* fp;

	fp = fopen("E:\\UDATA\\PoPX\\TitleMeta.xbx", "wb");
	if (fp) {
		fprintf(fp, "TitleName=Prince of Persia\r\n");
		fclose(fp);
	}

	fp = fopen("E:\\UDATA\\PoPX\\Settings\\SaveMeta.xbx", "wb");
	if (fp)
	{
		fprintf(fp, "Name=Settings\r\n");
		fclose(fp);
	}

	fp = fopen("E:\\UDATA\\PoPX\\Replays\\SaveMeta.xbx", "wb");
	if (fp) {
		fprintf(fp, "Name=Replays\r\n");
		fclose(fp);
	}

	fp = fopen("E:\\UDATA\\PoPX\\Saves\\SaveMeta.xbx", "wb");
	if (fp) {
		fprintf(fp, "Name=Saves\r\n");
		fclose(fp);
	}

	fp = fopen("E:\\UDATA\\PoPX\\Highscores\\SaveMeta.xbx", "wb");
	if (fp) {
		fprintf(fp, "Name=Highscores\r\n");
		fclose(fp);
	}
#endif

	g_argc = argc;
	g_argv = argv;
	pop_main();
	return 0;
}
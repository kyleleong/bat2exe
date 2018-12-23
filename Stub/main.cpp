#include <Windows.h>

constexpr const int BATCH_FILE_RESOURCE_ID = 42069;
constexpr const int RANDOM_FILE_NAME_LENGTH = 20;

/* Simple way to get randomness without having to resort to C library functions.
   Modified from: https://stackoverflow.com/a/7603688.
   
   I tried using MSVCRT.dll's rand() but it keeps outputting the same character,
   even if I seed the pRNG. Maybe I'm calling the function too quickly? Whatever,
   I know that this implementation works.*/
char GetRandomChar()
{
	/* We want values should persist after each call to this function. */
	static unsigned short lfsr = 0xACE1u;
	static unsigned short bit;

	bit = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5)) & 1;
	lfsr = (lfsr >> 1) | (bit << 15);

	return 'a' + (lfsr % 26);
}

bool GetRandomBatchFilePath(char* outputString)
{
	/* If outputString is null do not continue b/c null pointer. */
	if (!outputString)
		return false;

	/* The temp path shouldn't be more than 160 characters anyway so save the rest of the 160
	   for the file name (which I guarantee won't take the remainder of the string. */
	char tmpPath[MAX_PATH + 1];
	int startPos = GetTempPathA(MAX_PATH - 100, (LPSTR)&tmpPath);
	if (!startPos)
		return false;
	
	/* Concatenate some additional random characters. */
	for (int i = 0; i < RANDOM_FILE_NAME_LENGTH; i++)
		tmpPath[startPos + i] = GetRandomChar();

	/* Append the .bat extension and null pointer (can't do strcat b/c doesn't end with '\0'). */
	tmpPath[startPos + RANDOM_FILE_NAME_LENGTH + 0] = '.';
	tmpPath[startPos + RANDOM_FILE_NAME_LENGTH + 1] = 'b';
	tmpPath[startPos + RANDOM_FILE_NAME_LENGTH + 2] = 'a';
	tmpPath[startPos + RANDOM_FILE_NAME_LENGTH + 3] = 't';
	tmpPath[startPos + RANDOM_FILE_NAME_LENGTH + 4] = '\0';

	strcpy(outputString, tmpPath);

	return true;
}

/* Preface: There used to be a function here called ZeroMemory which basically re-implemented Win32's version
   which under the hood calls memset. I couldn't use memset because I did not want to link up against the C
   runtime library, but no matter what I did the linker kept complaining about not resolving where memset was.

   Reimplementation of Win32 macro ZeroMemory which calls RtlZeroMemory which calls memset which we can't use.
   The reasons for which are explained above. Undefining ZeroMemory didn't work for whatever reason so I guess
   I'll just resort to using a unique name :(.
   (There used to be a function implementation of ZeroMemory() here but since that didn't work I removed it.)
   
   EDIT: Okay, so after checking the assembly output I figured out that the line that assigns the character to
   the null byte actually calls _memset which is in MSVCRT.dll which is not loaded by default (The other Win32
   functions don't make the linker poop itself because KERNEL32.dll is loaded by default in every process -- I
   believe.) I guess I'll call _memset cuz there's no way around it even though I specifically avoided calling
   it so I wouldn't have to link against the C library but I guess I'll have to dynamically load and call
   _memset manually since there's no way around it.
   
   EDIT 2: I just read a post on Microsoft's forums saying the _memset calls are a result of my optimization
   for size, but it's too late I've already almost implemented dynamically grabbing the function.
   
   EDIT 3: No matter what I tried, I could not optimize out the calls to _memset. I tried disabling
   optimizations, disabling intrinsics, favoring neither size nor speed, and disabling whole program
   optimization. So I ended up running dumpbin /exports msvcrt.dll, then removing all the cruft so I was left
   with just the name of the exports, and then running lib /def:msvcrt.def /out:msvcrt.lib which gave me a lib
   file which I could then instruct the stub to link to. Unfortunately, the stub has now increased in size
   by 2 KB. I'm not sure if that's a result of the import library (I don't think it is since that was 250 KB),
   or my additional code, but regardless I am disappointed that I wasted three hours on a fruitless effort. */

int main()
{
	/* Make sure the random path generator doesn't fail for some reason. */
	char randPath[MAX_PATH + 1];
	if (!GetRandomBatchFilePath(randPath))
		return 1;

	/* Ensure the data regarding batch file is present. */
	HRSRC batchFileInfo = FindResourceA(0, MAKEINTRESOURCEA(BATCH_FILE_RESOURCE_ID), RT_RCDATA);
	if (!batchFileInfo)
		return 2;

	/* Is the batch file actually present? */
	DWORD batchFileSize = SizeofResource(0, batchFileInfo);
	if (!batchFileSize)
		return 3;

	/* Acquire handle to batch file. */
	HGLOBAL hBatchFileRsrc = LoadResource(0, batchFileInfo);
	if (!hBatchFileRsrc)
		return 4;

	/* Obtain a pointer to the batch file itself, finally. */
	LPVOID pBatchFileRsrc = LockResource(hBatchFileRsrc);
	if (!pBatchFileRsrc)
		return 5;

	/* Obtain a handle to the file on disk. */
	HANDLE hBatchFileDisk = CreateFileA(randPath, GENERIC_ALL, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hBatchFileDisk == INVALID_HANDLE_VALUE)
		return 6;

	/* Write the batch file to disk. */
	DWORD bytesWritten;
	BOOL writeSuccess = WriteFile(hBatchFileDisk, pBatchFileRsrc, batchFileSize, &bytesWritten, NULL);
	if (!writeSuccess)
		return 7;
	if (bytesWritten != batchFileSize)
		return 8;

	/* Close the handle that we created earlier. */
	CloseHandle(hBatchFileDisk);

	/* Set up command line necessary for launching batch files that is necessary for CreateProcess. */
	//char commandLine[MAX_PATH + 1] = "/c \"";
	char commandLine[MAX_PATH + 1] = "cmd.exe /c \"";
	strcat(commandLine, randPath);
	strcat(commandLine, "\"");

	/* More stuff for CreateProcess. */
	char currentDirectory[MAX_PATH + 1];
	if (!GetCurrentDirectoryA(MAX_PATH, currentDirectory))
		return 9;

	/* Even more stuff for CreateProcess. */
	STARTUPINFOA startupInfo;
	memset((void*)&startupInfo, 0, sizeof(STARTUPINFOA));
	startupInfo.cb = sizeof(STARTUPINFOA);
	PROCESS_INFORMATION createProcResult;

	/* Actually create the process. */
	//MessageBoxA(NULL, commandLine, "NYX", MB_OK);
	//BOOL processSpawned = CreateProcessA("cmd.exe", commandLine, NULL, NULL, FALSE, 0, NULL, currentDirectory, &startupInfo, &createProcResult);
	BOOL processSpawned = CreateProcessA(nullptr, commandLine, NULL, NULL, FALSE, 0, NULL, currentDirectory, &startupInfo, &createProcResult);
	if (!processSpawned)
		return 11;

	/* Wait for the spawned batch file to exit. */
	WaitForSingleObject(createProcResult.hProcess, INFINITE);

	/* Close handles. */
	CloseHandle(createProcResult.hThread);
	CloseHandle(createProcResult.hProcess);

	/* Delete the batch file. */
	if (!DeleteFileA(randPath))
		return 12;

	/* We are done, exit cleanly. */
	return 0;
}

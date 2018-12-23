#include <Windows.h>
#include <Shlwapi.h>

#include <iostream>
#include <string>

#include "resource.h"

constexpr const int BATCH_FILE_RESOURCE_ID = 42069;

/* Adapted from: https://stackoverflow.com/a/8520815 */
std::string GetFileNameFromPath(const std::string& filePath)
{
	std::string filePathCpy = filePath;
	int idxLastSlash = filePathCpy.find_last_of("\\/");
	
	/* Remove string up to and including the last slash. */
	if (idxLastSlash != std::string::npos)
		filePathCpy.erase(0, idxLastSlash + 1);

	/* Remove extension if present. */
	int idxDot = filePathCpy.rfind('.');
	if (idxDot != std::string::npos)
		filePathCpy.erase(idxDot);

	return filePathCpy;
}

/* Returns 0 if converted successfully, nonzero otherwise. */
int ConvertToExecutable(std::string filePath)
{
	/* Obtain a handle to the file on disk. */
	std::string executableName = GetFileNameFromPath(filePath) + ".exe";
	HANDLE stubHandle = CreateFileA(executableName.c_str(), GENERIC_ALL, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (stubHandle == INVALID_HANDLE_VALUE)
		return 1;

	/* Ensure data regarding stub is present. */
	HRSRC hStubInfo = FindResourceA(0, MAKEINTRESOURCEA(IDR_RCDATA1), RT_RCDATA);
	if (!hStubInfo)
		return 2;

	/* Is the data specificed by the data present. */
	DWORD stubSize = SizeofResource(0, hStubInfo);
	if (!stubSize)
		return 3;

	/* Acquire handle to the batch file. */
	HGLOBAL hStubRsrc = LoadResource(0, hStubInfo);
	if (!hStubRsrc)
		return 4;

	/* Obtain a pointer to where we will write to disk. */
	LPVOID pStubRsrc = LockResource(hStubRsrc);
	if (!pStubRsrc)
		return 5;

	/* Write the embedded stub to the disk. */
	DWORD bytesWritten;
	bool writeSuccess = WriteFile(stubHandle, pStubRsrc, stubSize, &bytesWritten, NULL);
	if (!writeSuccess)
		return 6;
	if (bytesWritten != stubSize)
		return 7;

	/* We need to close handle here becauase we're done writing to it, and otherwise BeginUpdateResourceA() fails. */
	CloseHandle(stubHandle);

	/* Obtain handle to the newly written stub. */
	HANDLE stubUpdateHandle = BeginUpdateResourceA(executableName.c_str(), TRUE);
	if (stubUpdateHandle == NULL)
		return 8;

	/* Obtain handle to batch file on disk. */
	HANDLE batchHandle = CreateFile(filePath.c_str(), GENERIC_ALL, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!batchHandle)
		return 9;

	/* Allocate dynamic buffer to hold the data that we will read from the batch file. */
	DWORD batchFileSize = GetFileSize(batchHandle, NULL);
	if (batchFileSize == INVALID_FILE_SIZE)
		return 10;
	char* bufBatchFile = new char[batchFileSize];
	if (bufBatchFile == nullptr)
		return 11;

	/* Load the contents of the buffer with the batch file's data. */
	DWORD bytesRead;
	bool readSuccess = ReadFile(batchHandle, bufBatchFile, batchFileSize, &bytesRead, FALSE);
	if (!readSuccess)
		return 12;
	if (bytesRead != batchFileSize)
		return 13;
	
	/* Update the resource of the stub to include the contents of the batch file.*/
	bool updateSuccess = UpdateResourceA(stubUpdateHandle, RT_RCDATA, MAKEINTRESOURCE(42069), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), bufBatchFile, batchFileSize);
	if (!updateSuccess) {
		return GetLastError();
	}

	/* Commit the resource update to disk. */
	bool commitSuccess = EndUpdateResourceA(stubUpdateHandle, FALSE);
	if (!commitSuccess)
		return 15;

	/* Clean up whatever we allocated / acquired handles to earlier. */
	CloseHandle(batchHandle);
	delete[] bufBatchFile;
	bufBatchFile = nullptr;

	return 0;
}

int main(int argc, char** argv)
{
	/* Ensure that the program has the correct arguments passed to it. */
	if (argc < 2) {
		std::cout << "Bat2Exe Converter" << std::endl;
		std::cout << "Convert batch files to executables to deter tampering of scripts." << std::endl << std::endl;
		std::cout << "Usage: bat2exe filelist..." << std::endl;
		return 1;
	}

	/* Loop through each batch file supplied in argument list, and attempt to convert it. */
	for (int curFile = 1; curFile < argc; curFile++) {
		int result = ConvertToExecutable(argv[curFile]);
		if (result == 0)
			std::cout << "Conversion success: " << argv[curFile] << std::endl;
		else
			std::cout << "Conversion failure [" << result << "]: " << argv[curFile] << std::endl;
	}
}
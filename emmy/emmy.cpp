// emmy.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <tchar.h>
#include <Windows.h>
#include <stdio.h>
#include <compressapi.h>

const int ACTION_COMPRESS = 0;
const int ACTION_DECOMPRESS = 1;

// compress the input file to the output file
BOOL compressFile(_In_ PBYTE InputBuffer, _In_ DWORD InputFileSize, _In_ HANDLE in, _In_ HANDLE out)
{
	double TimeDuration;
	BOOL Success;
	COMPRESSOR_HANDLE Compressor = NULL;
	DWORD ByteWritten;
	PBYTE CompressedBuffer = NULL;
	SIZE_T CompressedBufferSize, CompressedDataSize;
	ULONGLONG StartTime, EndTime;

	Success = CreateCompressor(
		COMPRESS_ALGORITHM_XPRESS_HUFF,
		NULL,
		&Compressor);

	if (!Success)
	{
		wprintf(L"Unable to create compressor\n");
		return FALSE;
	}

	// Query compressed buffer size
	Success = Compress(
		Compressor,
		InputBuffer,
		InputFileSize,
		NULL,
		0,
		&CompressedBufferSize);

	// Allocate memory for compressed buffer
	if (!Success)
	{
		DWORD ErrorCode = GetLastError();

		if (ErrorCode != ERROR_INSUFFICIENT_BUFFER)
		{
			wprintf(L"Cannot compress data: %d.\n", ErrorCode);
			return FALSE;
		}

		CompressedBuffer = (PBYTE)malloc(CompressedBufferSize);
		if (!CompressedBuffer)
		{
			wprintf(L"Cannot allocate memory for compressed buffer.\n");
			return FALSE;
		}
	}

	StartTime = GetTickCount64();
	// Call Compress() again to actually perform compression
	// output data to CompressedBuffer
	Success = Compress(
		Compressor,
		InputBuffer,
		InputFileSize,
		CompressedBuffer,
		CompressedBufferSize,
		&CompressedDataSize);

	if (!Success)
	{
		wprintf(L"Cannot compress data: %d\n", GetLastError());
		return FALSE;
	}

	EndTime = GetTickCount64();

	TimeDuration = (EndTime - StartTime) / 1000.0;

	// write compressed data to output file
	Success = WriteFile(
		out,
		CompressedBuffer,
		CompressedDataSize,
		&ByteWritten,
		NULL);

	if ((ByteWritten != CompressedDataSize) || (!Success))
	{
		wprintf(L"Cannot write compressed data to file %d\n", GetLastError());
		return FALSE;
	}

	wprintf(
		L"Input file size: %d; Compressed Size: %d\n",
		InputFileSize,
		CompressedDataSize);
	wprintf(L"Compression Time: %.2fs\n", TimeDuration);
	wprintf(L"File Compression Complete.\n");

	return TRUE;
}

BOOL decompressFile(_In_ PBYTE InputBuffer, _In_ DWORD InputFileSize, _In_ HANDLE in, _In_ HANDLE out)
{
	double TimeDuration;
	BOOL Success;
	DECOMPRESSOR_HANDLE Decompressor = NULL;
	DWORD ByteRead, ByteWritten;
	PBYTE DecompressedBuffer = NULL;
	PBYTE CompressedBuffer = NULL;
	SIZE_T DecompressedBufferSize, DecompressedDataSize;
	ULONGLONG StartTime, EndTime;

	// Create XpressHuff decompressor
	Success = CreateDecompressor(
		COMPRESS_ALGORITHM_XPRESS_HUFF,
		NULL,
		&Decompressor);

	if (!Success)
	{
		wprintf(L"Cannot create decompressor: %d\n", GetLastError());
		return FALSE;
	}

	// get compressed (input) buffer size
	Success = Decompress(
		Decompressor,
		InputBuffer,
		InputFileSize,
		NULL,
		0,
		&DecompressedBufferSize);

	// allocate memory for decompression buffer
	if (!Success)
	{
		DWORD ErrorCode = GetLastError();
		if (ErrorCode != ERROR_INSUFFICIENT_BUFFER)
		{
			wprintf(L"Cannot decompress data: %d.\n", ErrorCode);
			return FALSE;
		}
	}

	DecompressedBuffer = (PBYTE)malloc(DecompressedBufferSize);
	if (!DecompressedBuffer)
	{
		wprintf(L"Cannot allocate memory for decompress buffer\n");
		return FALSE;
	}

	StartTime = GetTickCount64();
	Success = Decompress(
		Decompressor,
		CompressedBuffer,
		InputFileSize,
		DecompressedBuffer,
		DecompressedBufferSize,
		&DecompressedDataSize);

	if (!Success)
	{
		wprintf(L"Cannot decompress data: %d\n", GetLastError());
		return FALSE;
	}
	EndTime = GetTickCount64();
	TimeDuration = (EndTime - StartTime) / 1000.0;

	// write decompressed data to output file
	Success = WriteFile(
		out,
		DecompressedBuffer,
		DecompressedDataSize,
		&ByteWritten,
		NULL);
	if ((ByteWritten != DecompressedDataSize) || (!Success))
	{
		wprintf(L"Cannot write decompressed data to file\n");
		return FALSE;
	}

	wprintf(
		L"Compressed size: %d; Decompressed Size: %d\n",
		InputFileSize,
		DecompressedDataSize);
	wprintf(L"Decompression Time: %.2fs\n", TimeDuration);
	wprintf(L"File decompressed.\n");

	return TRUE;
}

void printUsage(_In_ WCHAR *name)
{
	wprintf(L"Usage:\n\t%s [-c|-d] <input file> <output file>\n", name);
	wprintf(L"\t\t-c: compress input file to output file\n");
	wprintf(L"\t\t-d: decompress input file to output file\n");
}


int wmain(_In_ int argc, _In_ WCHAR *argv[])
{
	PBYTE InputBuffer = NULL;
	HANDLE InputFile = INVALID_HANDLE_VALUE;
	HANDLE OutputFile = INVALID_HANDLE_VALUE;
	BOOL DeleteTargetFile = FALSE;
	BOOL Success;
	DWORD InputFileSize, ByteRead;
	LARGE_INTEGER FileSize;
	const wchar_t *ActionDescription = NULL;

	if (argc != 4)
	{
		printUsage(argv[0]);
		return 1;
	}

	// verify valid action provided
	WCHAR *Action = argv[1];
	int ActionFlag = NULL;
	if (wcscmp(Action, L"-c") == 0)
	{
		ActionFlag = ACTION_COMPRESS;
		ActionDescription = L"compression";
	}
	else if (wcscmp(Action, L"-d") == 0)
	{
		ActionFlag = ACTION_DECOMPRESS;
		ActionDescription = L"decompression";
	}
	else
	{
		ActionFlag = -1;
		wprintf(L"Invalid action provided\n");
		printUsage(argv[0]);
		return 1;
	}


	// assign variables for arguments
	WCHAR *ProgramName = argv[0];
	WCHAR *InputFileName = argv[2];
	WCHAR *OutputFileName = argv[3];

	// open input file for reading
	InputFile = CreateFile(
		argv[1],
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (InputFile == INVALID_HANDLE_VALUE)
	{
		wprintf(L"Cannot open \t%s\n", InputFileName);
		goto done;
	}

	// get input file size
	Success = GetFileSizeEx(InputFile, &FileSize);
	if ((!Success) || (FileSize.QuadPart > 0xFFFFFFFF))
	{
		wprintf(L"Cannot get input file size or file is larger than 4GB\n");
		goto done;
	}
	InputFileSize = FileSize.LowPart;

	// Allocate memory for file contents
	InputBuffer = (PBYTE)malloc(InputFileSize);
	if (!InputBuffer)
	{
		wprintf(L"Cannot allocate memory for uncompressed buffer.\n");
		goto done;
	}

	// Read input file
	Success = ReadFile(InputFile, InputBuffer, InputFileSize, &ByteRead, NULL);
	if ((!Success) || (ByteRead != InputFileSize))
	{
		wprintf(L"Cannot read from \t%s\n", InputFileName);
		goto done;
	}

	// Open emtpy file for writing, if exists, overwrite
	OutputFile = CreateFile(
		OutputFileName,
		GENERIC_WRITE | DELETE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (OutputFile == INVALID_HANDLE_VALUE)
	{
		wprintf(L"Cannot create output file \t%s\n", OutputFileName);
		goto done;
	}

	if (ActionFlag == ACTION_COMPRESS)
	{
		Success = compressFile(InputBuffer, InputFileSize, InputFile, OutputFile);
	}
	else if (ActionFlag == ACTION_DECOMPRESS)
	{
		Success = decompressFile(InputBuffer, InputFileSize, InputFile, OutputFile);
	}

	if (!Success)
	{
		wprintf(L"%s failed.", ActionDescription);
		DeleteTargetFile = TRUE;
		goto done;
	}

done:
	if (InputBuffer)
	{
		free(InputBuffer);
	}

	if (InputFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(InputFile);
	}

	if (OutputFile != INVALID_HANDLE_VALUE)
	{
		if (DeleteTargetFile)
		{
			FILE_DISPOSITION_INFO fdi;
			fdi.DeleteFile = TRUE;      //  Marking for deletion
			Success = SetFileInformationByHandle(
				OutputFile,
				FileDispositionInfo,
				&fdi,
				sizeof(FILE_DISPOSITION_INFO));
			if (!Success) {
				wprintf(L"Cannot delete corrupted output file.\n");
			}
		}
		CloseHandle(OutputFile);
	}

	return 0;
}

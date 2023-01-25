#pragma once

#include <chrono>
#include <vector>
#include <stdio.h>
#include <Windows.h>

#define PRINT_ERROR(NAME) { printf("Err: %s, Line: %d, LastError: %d\n", NAME, __LINE__, GetLastError()); return 0; }
#define CHECK_ERROR(FCT, VAL) if(FCT == VAL) PRINT_ERROR(#FCT)
#define CHECK_ERROR2(FCT, VAL) if(FCT != VAL) PRINT_ERROR(#FCT)

#define HW_DIR "C:\\Facultate\\CSSO\\Week6\\"
#define DATA_DIR HW_DIR "date\\"
#define REZULTATE HW_DIR "rezultate\\"
#define REZULTATE_SECVENTIAL REZULTATE "secvential\\"
#define REZULTATE_STATIC REZULTATE "static2\\"
#define REZULTATE_DINAMIC REZULTATE "dinamic2\\"

int Setup();
int CreateDirectories(const char* path);
int GetCores();
int MatrixPair(long n, int k, int index);
int CreateMatrix(int* matrix, long n, int k, int index);
int ResetMatrix(int* matrix, long n);
int MultiplyMatrixSecv(int* matrix_C, int* matrix_A, int* matrix_B, long n);
int MultiplyMatrixStatic(int* matrix_C, int* matrix_A, int* matrix_B, long n, int workers);
int MultiplyMatrixDynamic(int* matrix_C, int* matrix_A, int* matrix_B, long n, int workers);
DWORD WINAPI WorkerThread(LPVOID thread_data);
int GenerateResultFilename(char* filename, int k, int index, int workers, int a, long long execution_time);
int GenerateDataFilename(char* filename, int k, int index, int a);
int WriteMatrix(int* matrix, long n, char* filename);
int lltostring(long long ll, char* string);

typedef struct {
	int* matrix_C;
	int* matrix_A;
	int* matrix_B;
	long n;
	long load;
	long start;
} THREAD_DATA, *PTHREAD_DATA;

extern int P;


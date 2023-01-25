#include "p2.h"

int P = 12;

int main() {
	CHECK_ERROR(Setup(), 0);
	long n = 10;
	for (int k = 1; k <= 4; ++k) {
		for (int index = 0; index < 15; ++index) {
			CHECK_ERROR(MatrixPair(n, k, index), 0);
		}
		n *= 10;
	}
}

int MatrixPair(long n, int k, int index) {
	char filename[MAX_PATH];
	int* matrix_A;
	int* matrix_B;
	int* matrix_C;
	std::chrono::high_resolution_clock::time_point start;
	std::chrono::high_resolution_clock::duration end;
	long long micros;
	start = std::chrono::high_resolution_clock::now();
	CHECK_ERROR((matrix_A = (int*)malloc(n * n * 4L)), NULL);
	CHECK_ERROR((matrix_B = (int*)malloc(n * n * 4L)), NULL);
	CHECK_ERROR((matrix_C = (int*)malloc(n * n * 4L)), NULL);
	CreateMatrix(matrix_A, n, k, index);
	CreateMatrix(matrix_B, n, k, index);
	end = std::chrono::high_resolution_clock::now() - start;
	micros = std::chrono::duration_cast<std::chrono::microseconds>(end).count();

	ResetMatrix(matrix_C, n);
	start = std::chrono::high_resolution_clock::now();
	MultiplyMatrixSecv(matrix_C, matrix_A, matrix_B, n);
	end = std::chrono::high_resolution_clock::now() - start;
	micros = std::chrono::duration_cast<std::chrono::microseconds>(end).count();
	GenerateResultFilename(filename, k, index, 0, 0, micros);
	WriteMatrix(matrix_C, n, filename);
	for (int i = 1; i <= P; ++i) {
		ResetMatrix(matrix_C, n);
		start = std::chrono::high_resolution_clock::now();
		MultiplyMatrixStatic(matrix_C, matrix_A, matrix_B, n, i);
		end = std::chrono::high_resolution_clock::now() - start;
		micros = std::chrono::duration_cast<std::chrono::microseconds>(end).count();
		GenerateResultFilename(filename, k, index, i, 1, micros);
		WriteMatrix(matrix_C, n, filename);
		ResetMatrix(matrix_C, n);
		start = std::chrono::high_resolution_clock::now();
		MultiplyMatrixDynamic(matrix_C, matrix_A, matrix_B, n, i);
		end = std::chrono::high_resolution_clock::now() - start;
		micros = std::chrono::duration_cast<std::chrono::microseconds>(end).count();
		GenerateResultFilename(filename, k, index, i, 2, micros);
		WriteMatrix(matrix_C, n, filename);
	}
	free(matrix_A);
	free(matrix_B);
	free(matrix_C);
	return 1;
}

int CreateMatrix(int* matrix, long n, int k, int index) {
	HANDLE file_handle;
	char buffer[BUFSIZ] = "";
	char filename[MAX_PATH];
	char number[15];
	int len = 0;
	int num_len;
	long max = n * n;
	long i = 0;
	GenerateDataFilename(filename, k, index, 0);
	file_handle = CreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	CHECK_ERROR(file_handle, INVALID_HANDLE_VALUE);
	while (i < max) {
		*(matrix + i) = rand();
		_itoa(*(matrix + i), number, 10);
		num_len = (int)strlen(number) + 1;
		if (len + num_len + 2 >= BUFSIZ) {
			CHECK_ERROR(WriteFile(file_handle, buffer, len, NULL, NULL), FALSE);
			strcpy(buffer, "");
			len = 0;
			++i;
		} else {
			++i;
			if (!(i % n)) {
				strcat(number, "\n");
			} else {
				strcat(number, " ");
			}
			strcat(buffer, number);
			len += num_len;
		}
	}
	CloseHandle(file_handle);
	return 1;
}

int ResetMatrix(int* matrix, long n) {
	long max = n * n;
	for (long i = 0; i < max; ++i) {
		*(matrix + i) = 0;
	}
	return 1;
}

int MultiplyMatrixSecv(int* matrix_C, int* matrix_A, int* matrix_B, long n) {
	int* cursor_A = matrix_A;
	int* cursor_B = matrix_B;
	int* cursor_C = matrix_C;
	for (long x = 0, y = 0, z = 0; x < n; ++z) {
		if (z == n) { z = 0; ++y; ++cursor_C; ++cursor_B; }
		if (y == n) { y = 0; ++x; cursor_A += n; cursor_B -= n; }
		*cursor_C += *(cursor_A + z) * *(cursor_B + z * n);
	}
	return 1;
}

int MultiplyMatrixStatic(int* matrix_C, int* matrix_A, int* matrix_B, long n, int workers) {
	std::vector<HANDLE> threads(workers);
	PTHREAD_DATA tdata;
	PTHREAD_DATA tdata_cursor;
	long iterations = n * n;
	long load = iterations / workers;
	long starting_point = 0;
	int extra = iterations % workers;
	CHECK_ERROR((tdata = (PTHREAD_DATA)malloc(sizeof(THREAD_DATA) * workers)), NULL);
	tdata_cursor = tdata;
	for (int i = 0; i < workers; ++i) {
		tdata_cursor->matrix_A = matrix_A;
		tdata_cursor->matrix_B = matrix_B;
		tdata_cursor->matrix_C = matrix_C;
		tdata_cursor->load = load + (extra > 0);
		tdata_cursor->start = starting_point;
		tdata_cursor->n = n;
		CHECK_ERROR((threads[i] = CreateThread(NULL, 0, WorkerThread, tdata_cursor, 0, NULL)), NULL);
		starting_point += tdata_cursor->load;
		iterations -= tdata_cursor->load;
		++tdata_cursor;
		--extra;
	}
	CHECK_ERROR(WaitForMultipleObjects(workers, threads.data(), TRUE, INFINITE), WAIT_FAILED);
	for (int i = 0; i < workers; ++i) {
		CloseHandle(threads[i]);
	}
	free(tdata);
	return 1;
}

int MultiplyMatrixDynamic(int* matrix_C, int* matrix_A, int* matrix_B, long n, int workers) {
	std::vector<HANDLE> threads(workers);
	PTHREAD_DATA tdata;
	PTHREAD_DATA tdata_cursor;
	int div = workers * 2;
	long iterations = n * n;
	long load = iterations / div;
	long starting_point = 0;
	int extra = iterations % div;
	CHECK_ERROR((tdata = (PTHREAD_DATA)malloc(sizeof(THREAD_DATA) * workers)), NULL);
	tdata_cursor = tdata;
	for (int i = 0; i < workers; ++i) {
		tdata_cursor->matrix_A = matrix_A;
		tdata_cursor->matrix_B = matrix_B;
		tdata_cursor->matrix_C = matrix_C;
		tdata_cursor->load = load + (extra > 0);
		tdata_cursor->start = starting_point;
		tdata_cursor->n = n;
		CHECK_ERROR((threads[i] = CreateThread(NULL, 0, WorkerThread, tdata_cursor, 0, NULL)), NULL);
		starting_point += tdata_cursor->load;
		iterations -= tdata_cursor->load;
		++tdata_cursor;
		--extra;
	}
	while (iterations > 0) {
		int th;
		CHECK_ERROR((th = WaitForMultipleObjects(workers, threads.data(), FALSE, INFINITE)), WAIT_FAILED);
		CloseHandle(threads[th]);
		tdata_cursor = tdata + th;
		tdata_cursor->start = starting_point;
		if (iterations < 11) {
			load = iterations;
			iterations = 0;
		}
		else {
			load = iterations / div + iterations % div;
		}
		tdata_cursor->load = load;
		starting_point += load;
		iterations -= load;
		CHECK_ERROR((threads[th] = CreateThread(NULL, 0, WorkerThread, tdata_cursor, 0, NULL)), NULL);
	}
	CHECK_ERROR(WaitForMultipleObjects(workers, threads.data(), TRUE, INFINITE), WAIT_FAILED);
	for (int i = 0; i < workers; ++i) {
		CloseHandle(threads[i]);
	}
	free(tdata);
	return 1;
}

DWORD WINAPI WorkerThread(LPVOID tdata) {
	PTHREAD_DATA thread_data = (PTHREAD_DATA)tdata;
	int* cursor_A = thread_data->matrix_A;
	int* cursor_B = thread_data->matrix_B;
	int* cursor_C = thread_data->matrix_C;
	long n = thread_data->n;
	long x, y, z, max_i, max_j;
	max_i = thread_data->start;
	x = max_i / n;
	y = max_i % n;
	z = 0;
	max_i += thread_data->load;
	max_j = max_i % n;
	max_i = max_i / n;
	cursor_C += n * x + y;
	cursor_A += n * x;
	cursor_B += y;
	do {
		*cursor_C += *(cursor_A + z) * *(cursor_B + z * n);
		++z;
		if (z == n) { z = 0; ++y; ++cursor_C; ++cursor_B; }
		if (y == n) { y = 0; ++x; cursor_A += n; cursor_B -= n; }
	} while (x < max_i);
	while (x == max_i && y < max_j) {
		*cursor_C += *(cursor_A + z) * *(cursor_B + z * n);
		++z;
		if (z == n) { z = 0; ++y; ++cursor_C; ++cursor_B; }
		if (y == n) { y = 0; ++x; cursor_A += n; cursor_B -= n; }
	}
	return 1;
}


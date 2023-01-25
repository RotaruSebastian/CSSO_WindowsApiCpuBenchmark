#include "p2.h"

int Setup() {
	srand((unsigned int)time(0));
	CHECK_ERROR(CreateDirectories(DATA_DIR), 0);
	CHECK_ERROR(CreateDirectories(REZULTATE_SECVENTIAL), 0);
	CHECK_ERROR(CreateDirectories(REZULTATE_STATIC), 0);
	CHECK_ERROR(CreateDirectories(REZULTATE_DINAMIC), 0);
	CHECK_ERROR((P = GetCores()), 0);
	return 1;
}

int CreateDirectories(const char* path) {
	char copy1[MAX_PATH];
	char copy2[MAX_PATH];
	char* token;
	memset(copy1, 0, MAX_PATH);
	memset(copy2, 0, MAX_PATH);
	strcpy(copy1, path);
	token = strtok(copy1, "\\");
	strcat(copy2, token);
	while ((token = strtok(NULL, "\\")) != NULL) {
		strcat(copy2, "\\");
		strcat(copy2, token);
		if (CreateDirectory(copy2, NULL) == 0) {
			int error = GetLastError();
			if (error != ERROR_ALREADY_EXISTS) {
				printf("Err: CreateDirectory - %d\n", error);
				_exit(0);
			}
		}
	}
	return 1;
}

int GetCores() {
	HANDLE file_handle;
	DWORD size = 0;
	DWORD bytes_read = 0;
	char* buffer;
	char* token;
	int count = 0;
	CHECK_ERROR((file_handle = CreateFile(HW_DIR "info.txt", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)), INVALID_HANDLE_VALUE);
	CHECK_ERROR((size = GetFileSize(file_handle, NULL)), INVALID_FILE_SIZE);
	CHECK_ERROR((buffer = (char*)malloc(size + 1)), NULL);
	CHECK_ERROR(ReadFile(file_handle, buffer, size, &bytes_read, NULL), FALSE);
	*(buffer + bytes_read) = '\0';
	token = buffer;
	while ((token = strstr(token, "nucleu"))) {
		++token;
		++count;
	}
	free(buffer);
	CloseHandle(file_handle);
	return count * 2;
}

int GenerateDataFilename(char* filename, int k, int index, int a) {
	char number[5];
	strcpy(filename, DATA_DIR);
	_itoa(k, number, 10);
	strcat(filename, number);
	strcat(filename, "_");
	_itoa(index, number, 10);
	strcat(filename, number);
	if (!a) {
		strcat(filename, "_A.txt");
	} else {
		strcat(filename, "_B.txt");
	}
	return 1;
}

int GenerateResultFilename(char* filename, int k, int index, int workers, int a, long long execution_time) {
	char number[30];
	switch (a) {
	case 0:
		strcpy(filename, REZULTATE_SECVENTIAL "Seq_");
		break;
	case 1:
		_itoa(workers, number, 10);
		strcat(number, "_");
		strcpy(filename, REZULTATE_STATIC);
		strcat(filename, number);
		break;
	case 2:
		_itoa(workers, number, 10);
		strcat(number, "_");
		strcpy(filename, REZULTATE_DINAMIC);
		strcat(filename, number);
		break;
	}
	_itoa(k, number, 10);
	strcat(number, "_");
	strcat(filename, number);
	_itoa(index, number, 10);
	strcat(number, "_");
	strcat(filename, number);
	lltostring(execution_time, number);
	strcat(number, "_.txt");
	strcat(filename, number);
	return 1;
}

int lltostring(long long ll, char* string) {
	if (!ll) {
		string[0] = '0';
		string[1] = '\0';
		return 1;
	}
	int i = 0;
	bool sign = false;
	if (ll < 0) {
		ll = -ll;
		sign = true;
	}
	while (ll) {
		*(string + i) = ll % 10 + 48;
		ll /= 10;
		++i;
	}
	if (sign) {
		*(string + i) = '-';
		++i;
	}
	*(string + i) = '\0';
	_strrev(string);
	return 1;
}

int WriteMatrix(int* matrix, long n, char* filename) {
	HANDLE file_handle;
	char buffer[BUFSIZ] = "";
	char number[15];
	int len = 0;
	int num_len;
	long max = n * n;
	file_handle = CreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	CHECK_ERROR(file_handle, INVALID_HANDLE_VALUE);
	for (long i = 0; i < max; ++i) {
		_itoa(*(matrix + i), number, 10);
		num_len = (int)strlen(number) + 1;
		if (len + num_len + 2 >= BUFSIZ) {
			CHECK_ERROR(WriteFile(file_handle, buffer, len, NULL, NULL), FALSE);
			strcpy(buffer, "");
			len = 0;
		}
		if (!((i + 1) % n)) {
			strcat(number, "\n");
		} else {
			strcat(number, " ");
		}
		strcat(buffer, number);
		len += num_len;
	}
	CHECK_ERROR(WriteFile(file_handle, buffer, len, NULL, NULL), FALSE);
	CloseHandle(file_handle);
	return 1;
}

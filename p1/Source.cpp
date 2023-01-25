#include <set>
#include <stdio.h>
#include <Windows.h>

#define PRINT_ERROR(NAME) { printf("Err: %s, Line: %d, LastError: %d\n", NAME, __LINE__, GetLastError()); return 0; }
#define CHECK_ERROR(FCT, VAL) if(FCT == VAL) PRINT_ERROR(#FCT)
#define CHECK_ERROR2(FCT, VAL) if(FCT != VAL) PRINT_ERROR(#FCT)

#define HWDIR "C:\\Facultate\\CSSO\\Week6"
#define LOG_PATH HWDIR "\\info.txt"


int CreateLog();
int HyperthreadedSystems();
template <typename T>
int GetCpus(int cpus[], T mask);
int WriteHTInfo(int cpus[], int count, LOGICAL_PROCESSOR_RELATIONSHIP lpr, int cache_level);
int NumaNodes();
int CpuSets();
void WriteCpuSetInformation(PSYSTEM_CPU_SET_INFORMATION cursor, char* buffer);

HANDLE file_handle;

int main() {
	CHECK_ERROR(CreateLog(), 0);
	file_handle = CreateFile(LOG_PATH, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	CHECK_ERROR(file_handle, INVALID_HANDLE_VALUE);
	CHECK_ERROR(HyperthreadedSystems(), 0);
	CHECK_ERROR(NumaNodes(), 0);
	CHECK_ERROR(CpuSets(), 0);
	CloseHandle(file_handle);
}

int CreateLog() {
	char copy1[MAX_PATH];
	char copy2[MAX_PATH];
	char* token;
	memset(copy1, 0, MAX_PATH);
	memset(copy2, 0, MAX_PATH);
	strcpy(copy1, HWDIR);
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

int HyperthreadedSystems() {
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION slpi = NULL;
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION cursor = NULL;
	DWORD length = 0;
	int cpus[64];
	int count;
	int cache = -1;
	GetLogicalProcessorInformation(slpi, &length);
	CHECK_ERROR((slpi = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(length)), NULL);
	cursor = slpi;
	CHECK_ERROR(GetLogicalProcessorInformation(slpi, &length), FALSE);
	CHECK_ERROR(WriteFile(file_handle, "\nA:\n", 4, NULL, NULL), FALSE);
	for (DWORD offset = 0; offset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= length; offset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION)) {
		count = GetCpus(cpus, cursor->ProcessorMask);
		if (cache != cursor->Cache.Level || cursor->Relationship != 2) {
			cache = cursor->Cache.Level;
			CHECK_ERROR(WriteHTInfo(cpus, count, cursor->Relationship, cursor->Cache.Level), 0);
		}
		if (cursor->Relationship == 2) {
			cache = cursor->Cache.Level;
		}
		else {
			cache = -1;
		}
		++cursor;
	}
	free(slpi);
	return 1;
}

template <typename T>
int GetCpus(int cpus[], T mask) {
	DWORD index = 0;
	DWORD lshift = sizeof(T) * 8 - 1;
	T test = (T)1 << lshift;
	for (DWORD i = 0; i <= lshift; ++i) {
		if (mask & test) {
			//int x = i - 64;
			int x = i;
			cpus[index] = x > 0 ? x : -x;
			++index;
		}
		test /= 2;
	}
	return index;
}

int WriteHTInfo(int cpus[], int count, LOGICAL_PROCESSOR_RELATIONSHIP lpr, int cache_level) {
	char buffer[BUFSIZ] = "Procesoarele logice: ";
	char cpu_string[10];
	for (int i = 0; i < count; ++i) {
		_itoa(cpus[i], cpu_string, 10);
		strcat(cpu_string, ", ");
		strcat(buffer, cpu_string);
	}
	switch (lpr) {
	case RelationCache:
		_itoa(cache_level, cpu_string, 10);
		strcat(buffer, "folosesc acelasi cache L");
		strcat(buffer, cpu_string);
		strcat(buffer, "\n");
		break;
	case RelationNumaNode:
		strcat(buffer, "fac parte din acelasi nod NUMA\n");
		break;
	case RelationProcessorCore:
		strcat(buffer, "fac parte din acelasi nucleu \n");
		break;
	case RelationProcessorPackage:
		strcat(buffer, "fac parte din acelasi procesor\n");
		break;
	}
	CHECK_ERROR(WriteFile(file_handle, buffer, (DWORD)strlen(buffer), NULL, NULL), FALSE);
	return 1;
}

int NumaNodes() {
	CHECK_ERROR(WriteFile(file_handle, "\nB:\n", 4, NULL, NULL), FALSE);
	char buffer[BUFSIZ] = "Nr. total noduri: ";
	char number[10];
	int cpus[64];
	int count;
	HANDLE process_handle;
	DWORD_PTR process_affinity;
	DWORD_PTR system_affinity;
	UCHAR node_number;
	std::set<UCHAR> numa_set;
	for (UCHAR i = 0; i < 64; ++i) {
		if (GetNumaProcessorNode(i, &node_number) != FALSE && node_number != 0xff) {
			numa_set.insert(node_number);
		}
	}
	_itoa(numa_set.size(), number, 10);
	strcat(buffer, number);
	process_handle = GetCurrentProcess();
	CHECK_ERROR(GetProcessAffinityMask(process_handle, &process_affinity, &system_affinity), FALSE);
	strcat(buffer, "\nProcesoare pe care procesul curent poate rula: ");
	memset(cpus, 0, sizeof(cpus));
	count = GetCpus(cpus, process_affinity);
	for (int i = 0; i < count; ++i) {
		_itoa(cpus[i], number, 10);
		strcat(number, ", ");
		strcat(buffer, number);
	}
	strcat(buffer, "\nProcesoare configurate pe sistem: ");
	memset(cpus, 0, sizeof(cpus));
	count = GetCpus(cpus, system_affinity);
	for (int i = 0; i < count; ++i) {
		_itoa(cpus[i], number, 10);
		strcat(number, ", ");
		strcat(buffer, number);
	}
	CHECK_ERROR(WriteFile(file_handle, buffer, strlen(buffer), NULL, NULL), FALSE);
	return 1;
}

int CpuSets() {
	CHECK_ERROR(WriteFile(file_handle, "\n\nC:\n", 5, NULL, NULL), FALSE);
	char buffer[BUFSIZ] = "Numar seturi cpu: ";
	char number[10];
	HANDLE process_handle;
	ULONG cpu_set_ids;
	ULONG required_id_count;
	ULONG length = 0;
	PSYSTEM_CPU_SET_INFORMATION pcsi = NULL;
	PSYSTEM_CPU_SET_INFORMATION cursor = NULL;
	int cpus[64];
	int count;
	process_handle = GetCurrentProcess();
	GetProcessDefaultCpuSets(process_handle, NULL, 0, &required_id_count);
	CHECK_ERROR(GetProcessDefaultCpuSets(process_handle, &cpu_set_ids, sizeof(ULONG), &required_id_count), FALSE);
	count = GetCpus(cpus, cpu_set_ids);
	_itoa(count, number, 10);
	strcat(buffer, number);
	strcat(buffer, "\nId seturi: ");
	for (int i = 0; i < count; ++i) {
		_itoa(cpus[i], number, 10);
		strcat(buffer, number);
		strcat(buffer, ", ");
	}
	GetSystemCpuSetInformation(NULL, 0, &length, process_handle, 0);
	CHECK_ERROR((pcsi = (PSYSTEM_CPU_SET_INFORMATION)malloc(length)), NULL);
	CHECK_ERROR(GetSystemCpuSetInformation(pcsi, length, &length, process_handle, 0), FALSE);
	strcat(buffer, "\nNumar structuri obtinute: ");
	_itoa(length / sizeof(SYSTEM_CPU_SET_INFORMATION), number, 10);
	strcat(buffer, number);
	cursor = pcsi;
	CHECK_ERROR(WriteFile(file_handle, buffer, strlen(buffer), NULL, NULL), FALSE);
	for (DWORD offset = 0; offset + sizeof(SYSTEM_CPU_SET_INFORMATION) <= length; offset += sizeof(SYSTEM_CPU_SET_INFORMATION)) {
		WriteCpuSetInformation(cursor, buffer);
		CHECK_ERROR(WriteFile(file_handle, buffer, strlen(buffer), NULL, NULL), FALSE);
		++cursor;
	}
	free(pcsi);
	return 1;
}

void WriteCpuSetInformation(PSYSTEM_CPU_SET_INFORMATION cursor, char* buffer) {
	char number[15];
	strcpy(buffer, "\nType: CpuSetInformation\nId: ");
	_itoa(cursor->CpuSet.Id, number, 10);
	strcat(buffer, number);
	strcat(buffer, "\nGroup: ");
	_itoa(cursor->CpuSet.Group, number, 10);
	strcat(buffer, number);
	strcat(buffer, "\nLogicalProcessorIndex: ");
	_itoa(cursor->CpuSet.LogicalProcessorIndex, number, 10);
	strcat(buffer, number);
	strcat(buffer, "\nCoreIndex: ");
	_itoa(cursor->CpuSet.CoreIndex, number, 10);
	strcat(buffer, number);
	strcat(buffer, "\nLastLevelCacheIndex: ");
	_itoa(cursor->CpuSet.LastLevelCacheIndex, number, 10);
	strcat(buffer, number);
	strcat(buffer, "\nNumaNodeIndex: ");
	_itoa(cursor->CpuSet.NumaNodeIndex, number, 10);
	strcat(buffer, number);
	strcat(buffer, "\nEfficiencyClass: ");
	_itoa(cursor->CpuSet.EfficiencyClass, number, 10);
	strcat(buffer, number);
}

#include "pch.h"
#include "Memory.h"

#include <thread>
#include <iostream>

Memory::Memory()
{
	LOG("loading libraries...\n");
	modules.VMM = LoadLibraryA("vmm.dll");
	modules.FTD3XX = LoadLibraryA("FTD3XX.dll");
	modules.LEECHCORE = LoadLibraryA("leechcore.dll");

	if (!modules.VMM || !modules.FTD3XX || !modules.LEECHCORE)
	{
		LOG("vmm: %p\n", modules.VMM);
		LOG("ftd: %p\n", modules.FTD3XX);
		LOG("leech: %p\n", modules.LEECHCORE);
		THROW("[!] Could not load a library\n");
	}

	this->key = std::make_shared<c_keys>();

	LOG("Successfully loaded libraries!\n");
}

Memory::~Memory()
{
	VMMDLL_Close(this->vHandle);
	DMA_INITIALIZED = false;
	PROCESS_INITIALIZED = false;
}

bool Memory::DumpMemoryMap(bool debug)
{
	VMM_HANDLE handle = nullptr;

	LPCSTR args[] = {"-device", "fpga", "-waitinitialize", "-norefresh", "", ""};
	int argc = 4;
	if (debug)
	{
		args[argc++] = const_cast<LPCSTR>("-v");
		args[argc++] = const_cast<LPCSTR>("-printf");
	}

	handle = VMMDLL_Initialize(argc, args);
	if (!handle)
	{
		LOG(("[-] DMA initialization failed, power cycle PC.\n"));
		return false;
	}

	PVMMDLL_MAP_PHYSMEM pPhysMemMap = NULL;
	if (!VMMDLL_Map_GetPhysMem(handle, &pPhysMemMap))
	{
		LOG("[!] Failed to get physical memory map\n");
		VMMDLL_Close(handle);
		return false;
	}

	if (pPhysMemMap->dwVersion != VMMDLL_MAP_PHYSMEM_VERSION)
	{
		LOG("[!] Invalid VMM Map Version\n");
		VMMDLL_MemFree(pPhysMemMap);
		VMMDLL_Close(handle);
		return false;
	}

	if (pPhysMemMap->cMap == 0)
	{
		printf("[!] Failed to get physical memory map\n");
		VMMDLL_MemFree(pPhysMemMap);
		VMMDLL_Close(handle);
		return false;
	}
	//Dump map to file
	std::stringstream sb;
	for (DWORD i = 0; i < pPhysMemMap->cMap; i++)
	{
		sb << std::hex << pPhysMemMap->pMap[i].pa << " " << (pPhysMemMap->pMap[i].pa + pPhysMemMap->pMap[i].cb - 1) << std::endl;
	}

	auto temp_path = std::filesystem::temp_directory_path();
	std::ofstream nFile(temp_path.string() + "mmap.txt");
	nFile << sb.str();
	nFile.close();

	VMMDLL_MemFree(pPhysMemMap);
	LOG("Successfully dumped memory map to file!\n");
	//Little sleep to make sure it's written to file.
	Sleep(3000);
	VMMDLL_Close(handle);
	return true;
}

unsigned char abort2[4] = {0x10, 0x00, 0x10, 0x00};

bool Memory::SetFPGA()
{
	bool result;
	ULONG64 qwID, qwVersionMajor, qwVersionMinor;
	if (VMMDLL_ConfigGet(this->vHandle, LC_OPT_FPGA_FPGA_ID, &qwID) && VMMDLL_ConfigGet(this->vHandle, LC_OPT_FPGA_VERSION_MAJOR, &qwVersionMajor) && VMMDLL_ConfigGet(this->vHandle, LC_OPT_FPGA_VERSION_MINOR, &qwVersionMinor))
	{
		LOG("[+] VMMDLL_ConfigGet");
		LOG(" ID = %lli", qwID);
		LOG(" VERSION = %lli.%lli\n", qwVersionMajor, qwVersionMinor);

		if ((qwVersionMajor >= 4) && ((qwVersionMajor >= 5) || (qwVersionMinor >= 7)))
		{
			HANDLE handle;
			LC_CONFIG config = {.dwVersion = LC_CONFIG_VERSION, .szDevice = "existing"};
			handle = LcCreate(&config);
			if (handle)
			{
				LcCommand(handle, LC_CMD_FPGA_CFGREGPCIE_MARKWR | 0x002, 4, (PBYTE)&abort2, NULL, NULL);
				LOG("[-] Register auto cleared\n");
				LcClose(handle);
			}
		}
	}
	else
	{
		LOG("[!] Failed to lookup FPGA device, Attempting to proceed\n\n");
		return false;
	}
	return true;
}

bool Memory::Init(std::string process_name, bool memMap, bool debug)
{
	if (!DMA_INITIALIZED)
	{
		LOG("inizializing...\n");

		LPCSTR args[] = {"-device", "fpga://algo=0", "-waitinitialize", "-memmap", "auto", "", "", ""};
		int argc = 5;
		if (debug)
		{
			args[argc++] = const_cast<LPCSTR>("-v");
			//    args[argc++] = const_cast<LPSTR>("-vv");
			args[argc++] = const_cast<LPCSTR>("-printf");
		}

		auto temp_path = std::filesystem::temp_directory_path();
		std::string path = (temp_path.string() + "mmap.txt");
		if (std::filesystem::exists(path))
		{
			LOG("[-] Using Memory Map from %s\n", path.c_str());
			args[4] = path.c_str();
		}
		else
		{
			LOG("[-] No Memory Map found, Dumping our own memory map.\n");
			if (DumpMemoryMap(debug))
			{
				args[4] = path.c_str();
				LOG("[+] Successfully dumped memory map\n");
			}
			else
			{
				LOG("[-] Failed to dump memory map, We're gonna attempt using memmap auto\n");
			}
		}

		this->vHandle = VMMDLL_InitializeEx(argc, args, NULL);
		if (!this->vHandle)
		{
			LOG("[-] DMA initialization failed with mmap, attempting without mmap.\n");
			LPCSTR args2[] = {"-device", "fpga://algo=0", "-waitinitialize", "", "", ""};
			int argc2 = 3;
			if (debug)
			{
				args2[argc2++] = const_cast<LPCSTR>("-v");
				args2[argc2++] = const_cast<LPCSTR>("-printf");
			}

			this->vHandle = VMMDLL_InitializeEx(argc2, args2, NULL);
			if (!this->vHandle)
			{
				LOG("[-] DMA initialization failed without mmap, power cycle PC.\n");
				return false;
			}
		}

		ULONG64 FPGA_ID = 0, DEVICE_ID = 0;

		VMMDLL_ConfigGet(this->vHandle, LC_OPT_FPGA_FPGA_ID, &FPGA_ID);
		VMMDLL_ConfigGet(this->vHandle, LC_OPT_FPGA_DEVICE_ID, &DEVICE_ID);

		LOG("FPGA ID: %llu\n", FPGA_ID);
		LOG("DEVICE ID: %llu\n", DEVICE_ID);
		LOG("success!\n");

		if (!this->SetFPGA())
		{
			LOG("[!] Could not set FPGA!\n");
			VMMDLL_Close(this->vHandle);
			return false;
		}

		DMA_INITIALIZED = TRUE;
	}
	else
		LOG("DMA already initialized!\n");

	if (PROCESS_INITIALIZED)
	{
		LOG("Process already initialized!\n");
		return true;
	}

	current_process.PID = GetPidFromName(process_name);
	if (!current_process.PID)
	{
		LOG("[!] Could not get PID from name!\n");
		return false;
	}
	current_process.process_name = process_name;
	if (!mem.FixCr3())
		std::cout << "Failed to fix CR3" << std::endl;
	else
		std::cout << "CR3 fixed" << std::endl;

	current_process.base_address = GetBaseDaddy(process_name);
	if (!current_process.base_address)
	{
		LOG("[!] Could not get base address!\n");
		return false;
	}

	current_process.base_size = GetBaseSize(process_name);
	if (!current_process.base_size)
	{
		LOG("[!] Could not get base size!\n");
		return false;
	}

	LOG("Process information of %s\n", process_name.c_str());
	LOG("PID: %i\n", current_process.PID);
	LOG("Base Address: 0x%llx\n", current_process.base_address);
	LOG("Base Size: 0x%llx\n", current_process.base_size);

	PROCESS_INITIALIZED = TRUE;

	return true;
}

DWORD Memory::GetPidFromName(std::string process_name)
{
	DWORD pid = 0;
	VMMDLL_PidGetFromName(this->vHandle, (LPSTR)process_name.c_str(), &pid);
	return pid;
}

std::vector<int> Memory::GetPidListFromName(std::string name)
{
	PVMMDLL_PROCESS_INFORMATION process_info = NULL;
	DWORD total_processes = 0;
	std::vector<int> list = { };

	if (!VMMDLL_ProcessGetInformationAll(this->vHandle, &process_info, &total_processes))
	{
		LOG("[!] Failed to get process list\n");
		return list;
	}

	for (size_t i = 0; i < total_processes; i++)
	{
		auto process = process_info[i];
		if (strstr(process.szNameLong, name.c_str()))
			list.push_back(process.dwPID);
	}

	return list;
}

std::vector<std::string> Memory::GetModuleList(std::string process_name)
{
	std::vector<std::string> list = { };
	PVMMDLL_MAP_MODULE module_info = NULL;
	if (!VMMDLL_Map_GetModuleU(this->vHandle, current_process.PID, &module_info, VMMDLL_MODULE_FLAG_NORMAL))
	{
		LOG("[!] Failed to get module list\n");
		return list;
	}

	for (size_t i = 0; i < module_info->cMap; i++)
	{
		auto module = module_info->pMap[i];
		list.push_back(module.uszText);
	}

	return list;
}

VMMDLL_PROCESS_INFORMATION Memory::GetProcessInformation()
{
	VMMDLL_PROCESS_INFORMATION info = { };
	SIZE_T process_information = sizeof(VMMDLL_PROCESS_INFORMATION);
	ZeroMemory(&info, sizeof(VMMDLL_PROCESS_INFORMATION));
	info.magic = VMMDLL_PROCESS_INFORMATION_MAGIC;
	info.wVersion = VMMDLL_PROCESS_INFORMATION_VERSION;

	if (!VMMDLL_ProcessGetInformation(this->vHandle, current_process.PID, &info, &process_information))
	{
		LOG("[!] Failed to find process information\n");
		return { };
	}

	LOG("[+] Found process information\n");
	return info;
}

PEB Memory::GetProcessPeb()
{
	auto info = GetProcessInformation();
	if (info.win.vaPEB)
	{
		LOG("[+] Found process PEB ptr at 0x%p\n", info.win.vaPEB);
		return Read<PEB>(info.win.vaPEB);
	}
	LOG("[!] Failed to find the processes PEB\n");
	return { };
}

size_t Memory::GetBaseDaddy(std::string module_name)
{
	std::wstring str(module_name.begin(), module_name.end());

	PVMMDLL_MAP_MODULEENTRY module_info;
	if (!VMMDLL_Map_GetModuleFromNameW(this->vHandle, current_process.PID, const_cast<LPWSTR>(str.c_str()), &module_info, VMMDLL_MODULE_FLAG_NORMAL))
	{
		LOG("[!] Couldn't find Base Address for %s\n", module_name.c_str());
		return 0;
	}

	LOG("[+] Found Base Address for %s at 0x%p\n", module_name.c_str(), module_info->vaBase);
	return module_info->vaBase;
}

size_t Memory::GetBaseSize(std::string module_name)
{
	std::wstring str(module_name.begin(), module_name.end());

	PVMMDLL_MAP_MODULEENTRY module_info;
	auto bResult = VMMDLL_Map_GetModuleFromNameW(this->vHandle, current_process.PID, const_cast<LPWSTR>(str.c_str()), &module_info, VMMDLL_MODULE_FLAG_NORMAL);
	if (bResult)
	{
		LOG("[+] Found Base Size for %s at 0x%p\n", module_name.c_str(), module_info->cbImageSize);
		return module_info->cbImageSize;
	}
	return 0;
}

uintptr_t Memory::GetExportTableAddress(std::string import, std::string process, std::string module)
{
	PVMMDLL_MAP_EAT eat_map = NULL;
	PVMMDLL_MAP_EATENTRY export_entry = NULL;
	bool result = VMMDLL_Map_GetEATU(mem.vHandle, mem.GetPidFromName(process) /*| VMMDLL_PID_PROCESS_WITH_KERNELMEMORY*/, const_cast<LPSTR>(module.c_str()), &eat_map);
	if (!result)
	{
		LOG("[!] Failed to get Export Table\n");
		return 0;
	}

	if (eat_map->dwVersion != VMMDLL_MAP_EAT_VERSION)
	{
		VMMDLL_MemFree(eat_map);
		eat_map = NULL;
		LOG("[!] Invalid VMM Map Version\n");
		return 0;
	}

	uintptr_t addr = 0;
	for (int i = 0; i < eat_map->cMap; i++)
	{
		export_entry = eat_map->pMap + i;
		if (strcmp(export_entry->uszFunction, import.c_str()) == 0)
		{
			addr = export_entry->vaFunction;
			break;
		}
	}

	VMMDLL_MemFree(eat_map);
	eat_map = NULL;

	return addr;
}

uintptr_t Memory::GetImportTableAddress(std::string import, std::string process, std::string module)
{
	PVMMDLL_MAP_IAT iat_map = NULL;
	PVMMDLL_MAP_IATENTRY import_entry = NULL;
	bool result = VMMDLL_Map_GetIATU(mem.vHandle, mem.GetPidFromName(process) /*| VMMDLL_PID_PROCESS_WITH_KERNELMEMORY*/, const_cast<LPSTR>(module.c_str()), &iat_map);
	if (!result)
	{
		LOG("[!] Failed to get Import Table\n");
		return 0;
	}

	if (iat_map->dwVersion != VMMDLL_MAP_IAT_VERSION)
	{
		VMMDLL_MemFree(iat_map);
		iat_map = NULL;
		LOG("[!] Invalid VMM Map Version\n");
		return 0;
	}

	uintptr_t addr = 0;
	for (int i = 0; i < iat_map->cMap; i++)
	{
		import_entry = iat_map->pMap + i;
		if (strcmp(import_entry->uszFunction, import.c_str()) == 0)
		{
			addr = import_entry->vaFunction;
			break;
		}
	}

	VMMDLL_MemFree(iat_map);
	iat_map = NULL;

	return addr;
}

std::vector<uintptr_t> Memory::GetAllDtbs()
{
	std::vector<uintptr_t> dtbs = { };
	PMMPFNOB_MAP SystemPfns = NULL;
	DWORD progress = 0;

	if (!VMMDLL_Map_GetSystemPfn(mem.vHandle, &SystemPfns, TRUE, &progress))
	{
		printf("Failed to get systems PFN\n");
		return dtbs;
	}

	for (int i = 0; i < SystemPfns->cMap; i++)
	{
		PVMM_PROCESS ProcessObject = VMMDLL_ProcessGet(mem.vHandle, SystemPfns->pMap[i].AddressInfo.dwPid);
		//If object is null then it means our good friends at EAC did some fuckery wucky with it
		if (!ProcessObject)
		{
			dtbs.push_back((QWORD)SystemPfns->pMap[i].dwPfn << 12);
			continue;
		}
		Ob_DECREF_NULL(&ProcessObject);
	}
	return dtbs;
}

bool Memory::FixCr3()
{
	PVMMDLL_MAP_MODULEENTRY module_entry;
	bool result = VMMDLL_Map_GetModuleFromNameU(this->vHandle, this->current_process.PID, (LPSTR)this->current_process.process_name.c_str(), &module_entry, NULL);
	if (result)
		return true; //Doesn't need to be patched lol

	auto possible_dtbs = GetAllDtbs();
	for (size_t i = 0; i < possible_dtbs.size(); i++)
	{
		auto dtb = possible_dtbs[i];
		VMMDLL_ConfigSet(this->vHandle, VMMDLL_OPT_PROCESS_DTB | this->current_process.PID, dtb);
		result = VMMDLL_Map_GetModuleFromNameU(this->vHandle, this->current_process.PID, (LPSTR)this->current_process.process_name.c_str(), &module_entry, NULL);
		if (result)
		{
			LOG("[+] Patched DTB\n");
			return true;
		}
	}

	LOG("[-] Failed to patch module\n");
	return false;
}

bool Memory::DumpMemory(uintptr_t address, std::string path)
{
	LOG("[!] Memory dumping currently does not rebuild the IAT table, imports will be missing from the dump.\n");
	IMAGE_DOS_HEADER dos { };
	Read(address, &dos, sizeof(IMAGE_DOS_HEADER));

	//Check if memory has a PE 
	if (dos.e_magic != 0x5A4D) //Check if it starts with MZ
	{
		LOG("[-] Invalid PE Header\n");
		return false;
	}

	IMAGE_NT_HEADERS64 nt;
	Read(address + dos.e_lfanew, &nt, sizeof(IMAGE_NT_HEADERS64));

	//Sanity check
	if (nt.Signature != IMAGE_NT_SIGNATURE || nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
	{
		LOG("[-] Failed signature check\n");
		return false;
	}
	//Shouldn't change ever. so const 
	const size_t target_size = nt.OptionalHeader.SizeOfImage;
	//Crashes if we don't make it a ptr :(
	auto target = std::unique_ptr<uint8_t[]>(new uint8_t[target_size]);

	//Read whole modules memory
	Read(address, target.get(), target_size);
	auto nt_header = (PIMAGE_NT_HEADERS64)(target.get() + dos.e_lfanew);
	auto sections = (PIMAGE_SECTION_HEADER)(target.get() + dos.e_lfanew + FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader) + nt.FileHeader.SizeOfOptionalHeader);

	for (size_t i = 0; i < nt.FileHeader.NumberOfSections; i++, sections++)
	{
		//Rewrite the file offsets to the virtual addresses
		LOG("[!] Rewriting file offsets at 0x%p size 0x%p\n", sections->VirtualAddress, sections->Misc.VirtualSize);
		sections->PointerToRawData = sections->VirtualAddress;
		sections->SizeOfRawData = sections->Misc.VirtualSize;
	}

	//Find all modules used by this process
	//auto descriptor = Read<IMAGE_IMPORT_DESCRIPTOR>(address + ntHeader->OptionalHeader.DataDirectory[1].VirtualAddress);

	//int descriptor_count = 0;
	//int thunk_count = 0;

	/*std::vector<ModuleData> modulelist;
	while (descriptor.Name) {
		auto first_thunk = Read<IMAGE_THUNK_DATA>(moduleAddr + descriptor.FirstThunk);
		auto original_first_thunk = Read<IMAGE_THUNK_DATA>(moduleAddr + descriptor.OriginalFirstThunk);
		thunk_count = 0;

		char ModuleName[256];
		ReadMemory(moduleAddr + descriptor.Name, (void*)&ModuleName, 256);

		std::string DllName = ModuleName;

		ModuleData tmpModuleData;

		//if(std::find(modulelist.begin(), modulelist.end(), tmpModuleData) == modulelist.end())
		//	modulelist.push_back(tmpModuleData);
		while (original_first_thunk.u1.AddressOfData) {
			char name[256];
			ReadMemory(moduleAddr + original_first_thunk.u1.AddressOfData + 0x2, (void*)&name, 256);

			std::string str_name = name;
			auto thunk_offset{ thunk_count * sizeof(uintptr_t) };

			//if (str_name.length() > 0)
			//	imports[str_name] = moduleAddr + descriptor.FirstThunk + thunk_offset;

			++thunk_count;
			first_thunk = Read<IMAGE_THUNK_DATA>(moduleAddr + descriptor.FirstThunk + sizeof(IMAGE_THUNK_DATA) * thunk_count);
			original_first_thunk = Read<IMAGE_THUNK_DATA>(moduleAddr + descriptor.OriginalFirstThunk + sizeof(IMAGE_THUNK_DATA) * thunk_count);
		}

		++descriptor_count;
		descriptor = Read<IMAGE_IMPORT_DESCRIPTOR>(moduleAddr + ntHeader->OptionalHeader.DataDirectory[1].VirtualAddress + sizeof(IMAGE_IMPORT_DESCRIPTOR) * descriptor_count);
	}*/

	//Rebuild import table

	//LOG("[!] Creating new import section\n");

	//Create New Import Section

	//Build new import Table

	//Dump file
	const auto dumped_file = CreateFileW(std::wstring(path.begin(), path.end()).c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_COMPRESSED, NULL);
	if (dumped_file == INVALID_HANDLE_VALUE)
	{
		LOG("[!] Failed creating file: %i\n", GetLastError());
		return false;
	}

	if (!WriteFile(dumped_file, target.get(), static_cast<DWORD>(target_size), NULL, NULL))
	{
		LOG("[!] Failed writing file: %i\n", GetLastError());
		CloseHandle(dumped_file);
		return false;
	}

	LOG("[+] Successfully dumped memory at %s\n", path.c_str());
	CloseHandle(dumped_file);
	return true;
}

static const char* hexdigits =
	"\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
	"\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
	"\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
	"\000\001\002\003\004\005\006\007\010\011\000\000\000\000\000\000"
	"\000\012\013\014\015\016\017\000\000\000\000\000\000\000\000\000"
	"\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
	"\000\012\013\014\015\016\017\000\000\000\000\000\000\000\000\000"
	"\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
	"\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
	"\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
	"\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
	"\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
	"\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
	"\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
	"\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
	"\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000";

static uint8_t GetByte(const char* hex)
{
	return static_cast<uint8_t>((hexdigits[hex[0]] << 4) | (hexdigits[hex[1]]));
}

uint64_t Memory::FindSignature(const char* signature, uint64_t range_start, uint64_t range_end, int PID)
{
	if (!signature || signature[0] == '\0' || range_start >= range_end)
		return 0;

	if (PID == 0)
		PID = current_process.PID;

	std::vector<uint8_t> buffer(range_end - range_start);
	if (!VMMDLL_MemReadEx(this->vHandle, PID, range_start, buffer.data(), buffer.size(), 0, VMMDLL_FLAG_NOCACHE))
		return 0;

	const char* pat = signature;
	uint64_t first_match = 0;
	for (uint64_t i = range_start; i < range_end; i++)
	{
		if (*pat == '?' || buffer[i - range_start] == GetByte(pat))
		{
			if (!first_match)
				first_match = i;

			if (!pat[2])
				break;

			pat += (*pat == '?') ? 2 : 3;
		}
		else
		{
			pat = signature;
			first_match = 0;
		}
	}

	return first_match;
}

bool Memory::ValidPtr(uintptr_t addr, bool kernel) const
{
	if (kernel)
	{
		if (addr > 0x2000000)
			return true;
		return false;
	}
	if (addr > 0x2000000 && addr < 0x7FFFFFFFFFFF)
		return true;
	return false;
}

bool Memory::Write(uintptr_t address, void* buffer, size_t size) const
{
	if (!ValidPtr(address))
		return false;

	if (!VMMDLL_MemWrite(this->vHandle, current_process.PID, address, static_cast<PBYTE>(buffer), size))
	{
		LOG("[!] Failed to write Memory at 0x%p\n", address);
		return false;
	}
	return true;
}

bool Memory::Write(uintptr_t address, void* buffer, size_t size, int pid) const
{
	if (!ValidPtr(address))
		return false;

	if (!VMMDLL_MemWrite(this->vHandle, pid, address, static_cast<PBYTE>(buffer), size))
	{
		LOG("[!] Failed to write Memory at 0x%p\n", address);
		return false;
	}
	return true;
}

bool Memory::Read(uintptr_t address, void* buffer, size_t size) const
{
	if (!ValidPtr(address))
		return false;

	DWORD read_size = 0;
	if (!VMMDLL_MemReadEx(this->vHandle, current_process.PID, address, static_cast<PBYTE>(buffer), size, &read_size, VMMDLL_FLAG_NOCACHE))
	{
		LOG("[!] Failed to read Memory at 0x%p\n", address);
		return false;
	}

	return (read_size == size);
}

bool Memory::Read(uintptr_t address, void* buffer, size_t size, int pid, bool kernel) const
{
	if (!ValidPtr(address, kernel))
		return false;

	DWORD read_size = 0;
	if (!VMMDLL_MemReadEx(this->vHandle, pid, address, static_cast<PBYTE>(buffer), size, &read_size, VMMDLL_FLAG_NOCACHE))
	{
		LOG("[!] Failed to read Memory at 0x%p\n", address);
		return false;
	}
	return (read_size == size);
}

VMMDLL_SCATTER_HANDLE Memory::CreateScatterHandle() const
{
	const VMMDLL_SCATTER_HANDLE ScatterHandle = VMMDLL_Scatter_Initialize(this->vHandle, current_process.PID, VMMDLL_FLAG_NOCACHE);
	if (!ScatterHandle)
		LOG("[!] Failed to create scatter handle\n");
	return ScatterHandle;
}

VMMDLL_SCATTER_HANDLE Memory::CreateScatterHandle(int pid) const
{
	const VMMDLL_SCATTER_HANDLE ScatterHandle = VMMDLL_Scatter_Initialize(this->vHandle, pid, VMMDLL_FLAG_NOCACHE);
	if (!ScatterHandle)
		LOG("[!] Failed to create scatter handle\n");
	return ScatterHandle;
}

void Memory::CloseScatterHandle(VMMDLL_SCATTER_HANDLE handle)
{
	VMMDLL_Scatter_CloseHandle(handle);
}

void Memory::AddScatterReadRequest(VMMDLL_SCATTER_HANDLE handle, uint64_t address, void* buffer, size_t size)
{
	if (!VMMDLL_Scatter_PrepareEx(handle, address, size, static_cast<PBYTE>(buffer), NULL))
	{
		LOG("[!] Failed to prepare scatter read at 0x%p\n", address);
	}
}

void Memory::AddScatterWriteRequest(VMMDLL_SCATTER_HANDLE handle, uint64_t address, void* buffer, size_t size)
{
	if (!VMMDLL_Scatter_PrepareWrite(handle, address, static_cast<PBYTE>(buffer), size))
	{
		LOG("[!] Failed to prepare scatter write at 0x%p\n", address);
	}
}

void Memory::ExecuteReadScatter(VMMDLL_SCATTER_HANDLE handle, int pid)
{
	if (pid == 0)
		pid = current_process.PID;

	if (!VMMDLL_Scatter_ExecuteRead(handle))
	{
		//LOG("[-] Failed to Execute Scatter Read\n");
	}
	//Clear after using it
	if (!VMMDLL_Scatter_Clear(handle, pid, VMMDLL_FLAG_NOCACHE))
	{
		LOG("[-] Failed to clear Scatter\n");
	}
}

void Memory::ExecuteWriteScatter(VMMDLL_SCATTER_HANDLE handle, int pid)
{
	if (pid == 0)
		pid = current_process.PID;

	if (!VMMDLL_Scatter_Execute(handle))
	{
		//	LOG("[-] Failed to Execute Scatter Read\n");
	}
	//Clear after using it
	if (!VMMDLL_Scatter_Clear(handle, pid, VMMDLL_FLAG_NOCACHE))
	{
		LOG("[-] Failed to clear Scatter\n");
	}
}

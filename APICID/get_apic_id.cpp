/**
    This file prints out the ACPI ID for each (logical/virtual) core/CPU
    Tested on AMD Ryzen 5900X
    Tested on Intel 14900KF
    Untested for systems with multiple CPUs
    Author: sp00n <sp.00.n@gmx.net>
    https://github.com/sp00n
    License: MIT
*/
#include <iostream>
#include <vector>
#include <thread>
#include <windows.h>

#ifdef _WIN32
#include <intrin.h>
#else
#include <cpuid.h>
#endif



// Function to get the APIC ID for a logical core
uint32_t getApicId() {
    uint32_t eax, ebx, ecx, edx;

#ifdef _WIN32
    int cpuInfo[4];
    __cpuid(cpuInfo, 1);
    eax = cpuInfo[0];
    ebx = cpuInfo[1];
    ecx = cpuInfo[2];
    edx = cpuInfo[3];
#else
    // Non-Windows systems, not really implemented here (and not needed I guess)
    __cpuid(1, eax, ebx, ecx, edx);
#endif

    // Extract the APIC ID from the EBX register
    int apicId = (ebx >> 24) & 0xFF;
    return apicId;
}



// Structure to hold information about each logical core
struct LogicalCoreInfo {
    int logicalCoreId;
    int physicalCoreId;
    int apicId;
    std::string coreType;
};



// Function to get the physical and logical core ID for each logical processor
std::vector<LogicalCoreInfo> getCoreInfo() {
    std::vector<LogicalCoreInfo> coreInfo;
    int logicalCoreIndex = 0;
    int physicalCoreIndex = 0;

    DWORD bufferLength = 0;

    // First call to GetLogicalProcessorInformationEx to get the required buffer size
    GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &bufferLength);
    std::vector<BYTE> buffer(bufferLength);

    // Second call to actually fill the buffer
    if (!GetLogicalProcessorInformationEx(RelationProcessorCore, reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data()), &bufferLength)) {
        std::cerr << "Failed to get logical processor information. Error: " << GetLastError() << std::endl;
        return {};
    }

    // Process each block of information in the buffer
    for (DWORD offset = 0; offset < bufferLength; ) {
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX info = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(&buffer[offset]);
        
        if (info->Relationship == RelationProcessorCore) {
            // There may be multiple processor groups
            for (int processorGroup = 0; processorGroup < info->Processor.GroupCount; ++processorGroup) {
                // Max 64 CPUs per processor group
                for (int curCpuInGroup = 0; curCpuInGroup < 64; ++curCpuInGroup) {
                    if (info->Processor.GroupMask[processorGroup].Mask & (1ULL << curCpuInGroup)) {
                        LogicalCoreInfo lci;
                        lci.logicalCoreId = logicalCoreIndex++;
                        lci.physicalCoreId = physicalCoreIndex;
                        lci.coreType = (info->Processor.Flags == LTP_PC_SMT) ? "SMT On" : "SMT Off";
                        
                        coreInfo.push_back(lci);
                    }
                }
            }
            
            physicalCoreIndex++;
        }
        
        // Move to the next block of information
        offset += info->Size;
    }

    return coreInfo;
}



// Our main function
int main() {
    // Get the number of logical cores
    // TODO: Does this work for multi-processor systems?
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);

    // The number of logical cores (i.e. virtual CPUs)
    int numCores = sysinfo.dwNumberOfProcessors;

    // Get our info
    std::vector<LogicalCoreInfo> coreInfo = getCoreInfo();


    // Iterate through each logical core
    for (int core = 0; core < numCores; ++core) {
        // Set thread affinity to the current logical core / virtual CPU to be able to get the APIC ID
        HANDLE thread = GetCurrentThread();
        DWORD_PTR affinityMask = (static_cast<unsigned long long>(1) << core);
        
        SetThreadAffinityMask(thread, affinityMask);

        // Get the APIC ID for the current core
        coreInfo[core].apicId = getApicId();
    }

    // Our output
    for (const auto& info : coreInfo) {
        std::cout << "Logical CPU " << info.logicalCoreId << " - Physical Core " << info.physicalCoreId << " - APIC ID " << info.apicId << " - " << info.coreType << std::endl;
    }

    return 0;
}

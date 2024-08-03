/**
    This file prints out the ACPI ID for each core
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



uint32_t get_apic_id() {
    uint32_t eax, ebx, ecx, edx;

#ifdef _WIN32
    int cpuInfo[4];
    __cpuid(cpuInfo, 1);
    eax = cpuInfo[0];
    ebx = cpuInfo[1];
    ecx = cpuInfo[2];
    edx = cpuInfo[3];
#else
    __cpuid(1, eax, ebx, ecx, edx);
#endif

    uint32_t apic_id = (ebx >> 24) & 0xFF;
    return apic_id;
}



int main() {
    // Get the number of logical processors
    // TODO: Does this work for multi-processor systems?
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);

    // The number of logical cores (i.e. virtual CPUs)
    int numCores = sysinfo.dwNumberOfProcessors;

    std::vector<uint32_t> apic_ids(numCores);

    // Iterate through each logical processor
    for (int core = 0; core < numCores; ++core) {
        // Set thread affinity to the current core to be able to get the APIC ID
        HANDLE thread = GetCurrentThread();
        DWORD_PTR affinityMask = (static_cast<unsigned long long>(1) << core);
        
        SetThreadAffinityMask(thread, affinityMask);

        // Get the APIC ID for the current core
        apic_ids[core] = get_apic_id();
    }

    // Output the APIC IDs
    for (int core = 0; core < numCores; ++core) {
        std::cout << "Core " << core << ": APIC ID = " << apic_ids[core] << std::endl;
    }

    return 0;
}

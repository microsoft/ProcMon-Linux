#include <iostream>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

#include "installer.h"

extern "C"
{
    #include "libsysinternalsEBPF.h"
}

extern char _binary_procmonEBPFkern4_17_5_1_o_start[];
extern char _binary_procmonEBPFkern4_17_5_1_o_end[];
extern char _binary_procmonEBPFkern5_2_o_start[];
extern char _binary_procmonEBPFkern5_2_o_end[];
extern char _binary_procmonEBPFkern5_3_5_5_o_start[];
extern char _binary_procmonEBPFkern5_3_5_5_o_end[];
extern char _binary_procmonEBPFkern5_6__o_start[];
extern char _binary_procmonEBPFkern5_6__o_end[];
extern char _binary_procmonEBPFkern4_17_5_1_core_o_start[];
extern char _binary_procmonEBPFkern4_17_5_1_core_o_end[];
extern char _binary_procmonEBPFkern5_2_core_o_start[];
extern char _binary_procmonEBPFkern5_2_core_o_end[];
extern char _binary_procmonEBPFkern5_3_5_5_core_o_start[];
extern char _binary_procmonEBPFkern5_3_5_5_core_o_end[];
extern char _binary_procmonEBPFkern5_6__core_o_start[];
extern char _binary_procmonEBPFkern5_6__core_o_end[];

namespace
{
    std::string installDirectory;

    const struct
    {
        const char* name;
        const char* start;
        const char* end;
    } programs[] = {
        {KERN_4_17_5_1_OBJ, _binary_procmonEBPFkern4_17_5_1_o_start, _binary_procmonEBPFkern4_17_5_1_o_end},
        {KERN_5_2_OBJ, _binary_procmonEBPFkern5_2_o_start, _binary_procmonEBPFkern5_2_o_end},
        {KERN_5_3_5_5_OBJ, _binary_procmonEBPFkern5_3_5_5_o_start, _binary_procmonEBPFkern5_3_5_5_o_end},
        {KERN_5_6__OBJ, _binary_procmonEBPFkern5_6__o_start, _binary_procmonEBPFkern5_6__o_end},
        {KERN_4_17_5_1_CORE_OBJ, _binary_procmonEBPFkern4_17_5_1_core_o_start, _binary_procmonEBPFkern4_17_5_1_core_o_end},
        {KERN_5_2_CORE_OBJ, _binary_procmonEBPFkern5_2_core_o_start, _binary_procmonEBPFkern5_2_core_o_end},
        {KERN_5_3_5_5_CORE_OBJ, _binary_procmonEBPFkern5_3_5_5_core_o_start, _binary_procmonEBPFkern5_3_5_5_core_o_end},
        {KERN_5_6__CORE_OBJ, _binary_procmonEBPFkern5_6__core_o_start, _binary_procmonEBPFkern5_6__core_o_end}
    };
}

const std::string& GetEBPFInstallDirectory()
{
    return installDirectory;
}

//--------------------------------------------------------------------
//
// ExtractEBPFPrograms
//
// Extracts the EBPF programs from the procmon binary and places them
// in a private, per-process directory under /tmp.
//
//--------------------------------------------------------------------
bool ExtractEBPFPrograms()
{
    if (!installDirectory.empty())
    {
        std::cerr << "eBPF programs have already been extracted\n";
        return false;
    }

    char directory[] = "/tmp/procmon-XXXXXX";
    const char* path = mkdtemp(directory);
    if (path == nullptr)
    {
        std::cerr << "Cannot create private eBPF directory: " << strerror(errno) << '\n';
        return false;
    }
    installDirectory = path;

    for (const auto& program : programs)
    {
        const std::string filepath = installDirectory + "/" + program.name;
        if (!dropFile(filepath.c_str(), program.start, program.end, false, S_IRUSR | S_IWUSR))
        {
            std::cerr << "Cannot extract eBPF program: " << filepath << '\n';
            DeleteEBPFPrograms();
            return false;
        }
    }

    return true;
}

//--------------------------------------------------------------------
//
// DeleteEBPFPrograms
//
// Deletes only this process's extracted eBPF programs and private directory.
//
//--------------------------------------------------------------------
bool DeleteEBPFPrograms()
{
    if (installDirectory.empty())
    {
        return true;
    }

    bool success = true;
    for (const auto& program : programs)
    {
        const std::string filepath = installDirectory + "/" + program.name;
        if (unlink(filepath.c_str()) != 0 && errno != ENOENT)
        {
            std::cerr << "Cannot remove eBPF program " << filepath << ": " << strerror(errno) << '\n';
            success = false;
        }
    }

    if (rmdir(installDirectory.c_str()) != 0)
    {
        std::cerr << "Cannot remove eBPF directory " << installDirectory << ": " << strerror(errno) << '\n';
        return false;
    }
    installDirectory.clear();

    return success;
}
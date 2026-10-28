#include "../src/installer.h"

#include <cassert>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#define EMBEDDED(name) \
    char _binary_##name##_start[] = #name; \
    char _binary_##name##_end[1];

EMBEDDED(procmonEBPFkern4_17_5_1_o)
EMBEDDED(procmonEBPFkern5_2_o)
EMBEDDED(procmonEBPFkern5_3_5_5_o)
EMBEDDED(procmonEBPFkern5_6__o)
EMBEDDED(procmonEBPFkern4_17_5_1_core_o)
EMBEDDED(procmonEBPFkern5_2_core_o)
EMBEDDED(procmonEBPFkern5_3_5_5_core_o)
EMBEDDED(procmonEBPFkern5_6__core_o)

namespace fs = std::filesystem;

static fs::path fixture;
static bool failDirectory = false;
static int failDrop = -1;
static int drops = 0;
static std::string lastDirectory;

extern "C" char* __real_mkdtemp(char*);

extern "C" char* __wrap_mkdtemp(char* name)
{
    assert(std::string(name) == "/tmp/procmon-XXXXXX");
    if (failDirectory)
    {
        errno = EACCES;
        return nullptr;
    }
    // Keep all test files in the runner's disposable fixture, not shared /tmp.
    std::string pattern = (fixture / "procmon-XXXXXX").string();
    assert(__real_mkdtemp(pattern.data()) != nullptr);
    lastDirectory = pattern;
    return lastDirectory.data();
}

extern "C" bool dropFile(const char* path, const char* start,
    const char* end, bool force, mode_t permissions)
{
    const fs::path file(path);
    struct stat directoryStat;
    assert(file.parent_path() == lastDirectory);
    assert(stat(lastDirectory.c_str(), &directoryStat) == 0);
    assert(S_ISDIR(directoryStat.st_mode));
    assert((directoryStat.st_mode & 0777) == 0700);
    assert(directoryStat.st_uid == geteuid());
    assert(!force);
    assert(permissions == 0600);
    assert(start != nullptr && end != nullptr);
    const bool fail = drops++ == failDrop;

    // Deliberately follow links, as an unhardened dependency could.
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, permissions);
    assert(fd >= 0);
    const size_t size = fail ? 1 : strlen(start);
    assert(write(fd, start, size) == static_cast<ssize_t>(size));
    assert(close(fd) == 0);
    return !fail;
}

static std::string contents(const fs::path& file)
{
    std::ifstream stream(file);
    return std::string(std::istreambuf_iterator<char>(stream), {});
}

int main(int argc, char** argv)
{
    if (argc == 3 && std::string(argv[1]) == "--child")
    {
        fixture = fs::path(argv[2]).parent_path();
        assert(GetEBPFInstallDirectory().empty());
        assert(ExtractEBPFPrograms());
        assert(GetEBPFInstallDirectory() != argv[2]);
        assert(DeleteEBPFPrograms());
        return 0;
    }
    assert(argc == 2);
    fixture = fs::canonical(argv[1]);
    fs::current_path(fixture);
    const fs::path sentinel = fixture / "sentinel";
    std::ofstream(sentinel) << "unchanged";
    fs::create_directory("procmonEBPF");
    for (const char* name : {
        KERN_4_17_5_1_OBJ, KERN_5_2_OBJ, KERN_5_3_5_5_OBJ, KERN_5_6__OBJ,
        KERN_4_17_5_1_CORE_OBJ, KERN_5_2_CORE_OBJ, KERN_5_3_5_5_CORE_OBJ, KERN_5_6__CORE_OBJ})
    {
        fs::create_symlink(sentinel, fixture / name);
        std::ofstream(fixture / "procmonEBPF" / name) << "untrusted object";
    }
    std::ofstream("procmon-unrelated") << "keep";

    assert(GetEBPFInstallDirectory().empty());
    assert(DeleteEBPFPrograms());
    failDirectory = true;
    assert(!ExtractEBPFPrograms());
    assert(GetEBPFInstallDirectory().empty());
    failDirectory = false;

    // Every partial extraction must fail closed and remove its own files.
    for (failDrop = 0; failDrop < 8; ++failDrop)
    {
        drops = 0;
        assert(!ExtractEBPFPrograms());
        assert(GetEBPFInstallDirectory().empty());
        assert(!fs::exists(lastDirectory));
    }
    failDrop = -1;
    drops = 0;
    assert(ExtractEBPFPrograms());
    const std::string first = GetEBPFInstallDirectory();
    assert(drops == 8);
    assert(std::distance(fs::directory_iterator(first), fs::directory_iterator{}) == 8);
    for (const auto& file : fs::directory_iterator(first))
    {
        struct stat fileStat;
        assert(lstat(file.path().c_str(), &fileStat) == 0);
        assert(S_ISREG(fileStat.st_mode));
        assert((fileStat.st_mode & 0777) == 0600);
        std::string expected = file.path().filename().string();
        for (char& c : expected)
            if (c == '.' || c == '-') c = '_';
        assert(contents(file.path()) == expected);
    }
    assert(!ExtractEBPFPrograms());
    assert(GetEBPFInstallDirectory() == first);
    assert(contents(sentinel) == "unchanged");

    // A new process must not share or delete the parent's extracted objects.
    pid_t child = fork();
    assert(child >= 0);
    if (child == 0)
    {
        execl(argv[0], argv[0], "--child", first.c_str(), nullptr);
        _exit(127);
    }
    int status;
    assert(waitpid(child, &status, 0) == child);
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    assert(fs::exists(fs::path(first) / KERN_5_6__OBJ));

    // Cleanup never sweeps arbitrary files, even inside the private directory.
    std::ofstream(fs::path(first) / "unexpected") << "keep";
    assert(!DeleteEBPFPrograms());
    assert(contents(fs::path(first) / "unexpected") == "keep");
    fs::remove(fs::path(first) / "unexpected");
    assert(DeleteEBPFPrograms());
    assert(DeleteEBPFPrograms());
    assert(!fs::exists(first));
    assert(contents(fixture / "procmon-unrelated") == "keep");
    assert(fs::is_symlink(fixture / KERN_5_6__OBJ));
    assert(contents(sentinel) == "unchanged");

    assert(ExtractEBPFPrograms());
    assert(GetEBPFInstallDirectory() != first);
    assert(DeleteEBPFPrograms());
    std::cout << "Private extraction, failure cleanup, and isolation checks passed\n";
}

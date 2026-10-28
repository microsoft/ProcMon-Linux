# Build
Please see the history of this file for instructions for older, unsupported versions.

## Prerequisites
- SysinternalsEBPF being installed:
library `libsysinternalsEBPF.so`, header `libsysinternalsEBPF.h`, plus
resource files in `/opt/sysinternalsEBPF`. These can be installed from
the
[SysinternalsEBPF](https://github.com/Sysinternals/SysinternalsEBPF)
project or via the `sysinternalsebpf` DEB package from the
_packages.microsoft.com_ repository (see [INSTALL.md](INSTALL.md)).
If you installed SysinternalsEBPF via make install, you may need to add /usr/local/lib to the loader library path (LD_LIBRARY_PATH).

- clang/llvm v10+

### Azure Linux
```
sudo dnf update
sudo dnf install gcc gcc-c++ make cmake llvm clang elfutils-libelf-devel rpm-build json-glib-devel python3 libxml2-devel openssl-devel ncurses-devel
```

### Ubuntu 20.04+
```
sudo apt update
sudo apt -y install build-essential gcc g++ make cmake libelf-dev llvm clang libxml2 libxml2-dev libzstd1 git libgtest-dev apt-transport-https dirmngr libjson-glib-dev libssl-dev
```

### Rocky 9+
```
sudo dnf install dnf-plugins-core
sudo dnf config-manager --set-enabled crb
sudo dnf install epel-release

sudo dnf update
sudo yum install gcc gcc-c++ make cmake llvm clang elfutils-libelf-devel rpm-build json-glib-devel python3 libxml2-devel openssl-devel ncurses-devel
```

### Rocky 8
```
sudo dnf install dnf-plugins-core
sudo dnf install epel-release
sudo dnf config-manager --set-enabled powertools

sudo dnf update
sudo yum install gcc gcc-c++ make cmake llvm clang elfutils-libelf-devel rpm-build json-glib-devel python3 libxml2-devel openssl-devel ncurses-devel
```

### Fedora
```
sudo yum install gcc gcc-c++ make cmake llvm clang elfutils-libelf-devel rpm-build json-glib-devel python3 libxml2-devel openssl-devel ncurses-devel
```

### Debian 11
```
wget https://packages.microsoft.com/config/debian/11/packages-microsoft-prod.deb -O packages-microsoft-prod.deb
sudo dpkg -i packages-microsoft-prod.deb
rm packages-microsoft-prod.deb
sudo apt update
sudo apt -y install build-essential gcc g++ make cmake libelf-dev llvm clang libzstd1 git libjson-glib-dev libxml2 libxml2-dev libssl-dev
```

## Build
```
cd
git clone https://github.com/Sysinternals/ProcMon-for-Linux.git
cd ProcMon-for-Linux
mkdir build
cd build
cmake ..
make
```

## Run
```
sudo ./procmon
```

Procmon extracts its embedded eBPF objects into a fresh, owner-only
`/tmp/procmon-XXXXXX` directory for each invocation and loads objects only from
that directory. Objects in the working directory, `./procmonEBPF`, or directly
under `/tmp` are not used. To test modified eBPF code, rebuild Procmon so the
updated objects are embedded. Normal exit removes the extracted objects;
forced termination (for example, `SIGKILL`) can leave the private directory.

### Extraction regression tests

On Linux, run `python3 tests/installer.test.py` with `g++` installed. The tests
compile the real extraction code against a deliberately unhardened `dropFile`
test double, without requiring root or loading eBPF. They check directory and
file permissions, all eight embedded objects, partial failures, concurrent
process isolation, and scoped cleanup. Source-contract checks cover the
loader's sole search path and the startup failure gate.

For end-to-end verification on a disposable Linux VM, build and run
`sudo ./build/procmon -c /absolute/path/to/capture.db`, generate filesystem
activity, stop with Ctrl+C, and inspect the capture with
`sudo ./build/procmon -f /absolute/path/to/capture.db`. Confirm the live
extraction directory is root-owned with mode `0700` and disappears on normal
exit. Repeat with two instances and ensure stopping one does not remove the
other's objects. In an isolated test `/tmp`, place harmless invalid objects in
the former search locations and symlinks at the former extraction filenames
pointing to a disposable sentinel: capture must still work and the sentinel
must remain unchanged.

## Make Packages
Packages can be generated with:
```
make deb
```
or
```
make rpm
```

The directories build/deb and build/rpm will be populated with the required
files. If dpkg-deb is available, the build/deb directory will be used to create
a deb package. Similarly if rpmbuild is available, the build/rpm directory will
be used to create an rpm package.

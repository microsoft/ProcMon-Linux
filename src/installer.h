/*
    Procmon-for-Linux

    Copyright (c) Microsoft Corporation

    All rights reserved.

    MIT License

    Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the ""Software""), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#ifndef PROCMON_INSTALLER_H
#define PROCMON_INSTALLER_H

#include <sys/types.h>
#include <sys/stat.h>
#include <string>

#define KERN_4_16_OBJ           "procmonEBPFkern4.16.o"
#define KERN_4_17_5_1_OBJ       "procmonEBPFkern4.17-5.1.o"
#define KERN_5_2_OBJ            "procmonEBPFkern5.2.o"
#define KERN_5_3_5_5_OBJ        "procmonEBPFkern5.3-5.5.o"
#define KERN_5_6__OBJ           "procmonEBPFkern5.6-.o"
#define KERN_4_15_CORE_OBJ      "procmonEBPFkern4.15_core.o"
#define KERN_4_16_CORE_OBJ      "procmonEBPFkern4.16_core.o"
#define KERN_4_17_5_1_CORE_OBJ  "procmonEBPFkern4.17-5.1_core.o"
#define KERN_5_2_CORE_OBJ       "procmonEBPFkern5.2_core.o"
#define KERN_5_3_5_5_CORE_OBJ   "procmonEBPFkern5.3-5.5_core.o"
#define KERN_5_6__CORE_OBJ      "procmonEBPFkern5.6-_core.o"

bool ExtractEBPFPrograms();
bool DeleteEBPFPrograms();
const std::string& GetEBPFInstallDirectory();

#endif
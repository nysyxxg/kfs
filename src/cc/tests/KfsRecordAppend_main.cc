//---------------------------------------------------------- -*- Mode: C++ -*-
// $Id: KfsRecordAppend_main.cc 3277 2011-10-27 19:12:23Z sriramr $
//
// Created 2006/06/23
//
// Copyright 2008 Quantcast Corp.
// Copyright 2006-2008 Kosmix Corp.
//
// This file is part of Kosmos File System (KFS).
//
// Licensed under the Apache License, Version 2.0
// (the "License"); you may not use this file except in compliance with
// the License. You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied. See the License for the specific language governing
// permissions and limitations under the License.
//
// \brief Test atomic record append API in KFS.
//
//----------------------------------------------------------------------------

#include <iostream>    
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fstream>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "libkfsClient/KfsClient.h"
#include "common/log.h"

using std::cout;
using std::endl;
using std::ifstream;
using std::string;

using namespace KFS;

int numReplicas = 3;
KfsClientPtr gKfsClient;
static bool doMkdirs(const char *dirname);
static off_t doWrite(const string &kfspathname, int numMBytes, 
                     size_t writeSizeBytes, double sleepSec, char record);

int
main(int argc, char **argv)
{
    char optchar;
    string kfspathname = "";
    char *kfsPropsFile = NULL;
    int numMBytes = 1;
    size_t writeSizeBytes = 65536;
    bool help = false;
    double sleepSec = -1;
    char record('x');

    while ((optchar = getopt(argc, argv, "f:p:m:b:r:S:c:")) != -1) {
        switch (optchar) {
            case 'c':
                record = *optarg;
                break;
            case 'f':
                kfspathname = optarg;
                break;
            case 'b':
                writeSizeBytes = atoll(optarg);
                break;
            case 'p':
                kfsPropsFile = optarg;
                break;
            case 'm':
                numMBytes = atoi(optarg);
                break;
            case 'r':
                numReplicas = atoi(optarg);
                break;
            case 'S':
                sleepSec = atof(optarg);
                break;
            default:
                cout << "Unrecognized flag: " << optchar << endl;
                help = true;
                break;
        }
    }

    if (help || (kfsPropsFile == NULL) || (kfspathname == "")) {
        cout << "Usage: " << argv[0] << " -p <Kfs Client properties file> "
             << " -m <# of MB to write> -b <write size in bytes> -f <Kfs file> "
             << " -S <sleep between writes> -c <char for the record>"
             << endl;
        exit(0);
    }

    cout << "Doing writes to: " << kfspathname << " # MB = " << numMBytes;
    cout << " # of bytes per write: " << writeSizeBytes << endl;

    gKfsClient = getKfsClientFactory()->GetClient(kfsPropsFile);
    if (!gKfsClient) {
        cout << "kfs client failed to initialize...exiting" << endl;
        exit(-1);
    }

    KFS::MsgLogger::SetLevel(KFS::MsgLogger::kLogLevelDEBUG);

    string kfsdirname, kfsfilename;
    string::size_type slash = kfspathname.rfind('/');
    
    if (slash == string::npos) {
        cout << "Bad kfs path: " << kfsdirname << endl;
        exit(-1);
    }

    kfsdirname.assign(kfspathname, 0, slash);
    kfsfilename.assign(kfspathname, slash + 1, kfspathname.size());
    doMkdirs(kfsdirname.c_str());

    struct timeval startTime, endTime;
    double timeTaken;
    off_t bytesWritten;

    gettimeofday(&startTime, NULL);

    bytesWritten = doWrite(kfspathname, numMBytes, writeSizeBytes, sleepSec, record);

    gettimeofday(&endTime, NULL);

    timeTaken = (endTime.tv_sec - startTime.tv_sec) +
        (endTime.tv_usec - startTime.tv_usec) * 1e-6;

    cout << "Write rate: " << (((double) bytesWritten * 8.0) / timeTaken) / (1024.0 * 1024.0) << " (Mbps)" << endl;
    cout << "Write rate: " << ((double) bytesWritten / timeTaken) / (1024.0 * 1024.0) << " (MBps)" << endl;
    return 0;
}

bool
doMkdirs(const char *dirname)
{
    int fd;

    cout << "Making dir: " << dirname << endl;

    fd = gKfsClient->Mkdirs(dirname);
    if (fd < 0) {
        cout << "Mkdir failed: " << fd << endl;
        return false;
    }
    cout << "Mkdir returned: " << fd << endl;
    return fd > 0;
}

off_t
doWrite(const string &filename, int numMBytes, size_t writeSizeBytes, double sleepSec, char record)
{
    const size_t mByte = 1024 * 1024;
    char dataBuf[mByte];
    int res, fd;
    size_t bytesWritten = 0;
    int nMBytes = 0;
    off_t nwrote = 0;

    if (writeSizeBytes > mByte) {
        cout << "Setting write size to: " << mByte << endl;
        writeSizeBytes = mByte;
    }
    
    for (bytesWritten = 4; bytesWritten < writeSizeBytes; bytesWritten++) {
        dataBuf[bytesWritten] = record;
    }

    // fd = gKfsClient->Open(filename.c_str(), O_CREAT|O_RDWR);
    fd = gKfsClient->Create(filename.c_str(), numReplicas, true);
    if ((fd == -EEXIST) || (fd > 0)) {
        fd = gKfsClient->Open(filename.c_str(), O_RDWR|O_APPEND);
    } 
    if (fd < 0) {
        cout << "Create failed: " << endl;
        exit(-1);
    }

    struct timespec sleepTm;
    const bool doSleep = sleepSec > 0;
    if (doSleep) {
        sleepTm.tv_sec = time_t(sleepSec);
        sleepTm.tv_nsec = long((sleepSec - (double)sleepTm.tv_sec) * 1e9);
    }
    int recordCount = 0;
    for (nMBytes = 0; nMBytes < numMBytes; nMBytes++) {
        for (bytesWritten = 0; bytesWritten < mByte; bytesWritten += writeSizeBytes) {
            snprintf(dataBuf, 4, "%d", recordCount);
            recordCount++;
            res = gKfsClient->AtomicRecordAppend(fd, dataBuf, writeSizeBytes);
            if (res != (int) writeSizeBytes)
                return (bytesWritten + nMBytes * 1024 * 1024);
            nwrote += writeSizeBytes;
        }
        if (doSleep) {

            nanosleep(&sleepTm, 0);
        }
    }
    cout << "write of " << nwrote / (1024 * 1024) << " (MB) is done" << endl;

    sleep(5);

    gKfsClient->Close(fd);

    return nwrote;
}
    

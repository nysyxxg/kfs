#
# $Id: CMakeLists.txt 392 2010-05-27 20:11:26Z sriramsrao $
#
# Created 2006/10/20
#
# Copyright 2006 Kosmix Corp.
#
# This file is part of Kosmos File System (KFS).
#
# Licensed under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
# implied. See the License for the specific language governing
# permissions and limitations under the License.
#
#

set (exe_files
KfsDataGen
KfsDirFileTester
KfsPerfReader
KfsPerfWriter
KfsRecordAppend
KfsReader
KfsSeekWrite
KfsTrunc
KfsWriter
KfsDirScanTest
KfsIndexReader
mkfstree
KfsRW
KfsLogTest
memory_pinner
)

#
# Every executable depends on its namesake source with _main.cc
#
foreach (exe_file ${exe_files})
        add_executable (${exe_file} ${exe_file}_main.cc)
        if (USE_STATIC_LIB_LINKAGE)
                add_dependencies (${exe_file} kfsClient)
                target_link_libraries (${exe_file} tools kfsClient)
        else (USE_STATIC_LIB_LINKAGE)
                add_dependencies (${exe_file} kfsClient-shared)
                target_link_libraries (${exe_file} tools-shared kfsClient-shared)
        endif (USE_STATIC_LIB_LINKAGE)

endforeach (exe_file)

#
install (TARGETS ${exe_files}
        RUNTIME DESTINATION bin/tests)



/**
* @file fuse_service.h
* @author Martin Kosdy
* @author Matus Kysel
* @date 2016
* @brief File FUSE support for this library
*
*/

#ifndef STEGODISK_FUSE_FUSESERVICE_H_
#define STEGODISK_FUSE_FUSESERVICE_H_

#define FUSE_USE_VERSION 26
#define _FILE_OFFSET_BITS 64

#ifdef __APPLE__
#include <osxfuse/fuse.h>
#else
#include <fuse.h>
#endif

#include <string>
#include <thread>

//#include "fuse_service_delegate.h"
#include "logging/logger.h"
#include "stego_storage.h"

namespace Stego {
    static struct fuse_operations stegofs_ops;

    class FuseService {

    public:
        FuseService();

//  static FuseServiceDelegate* delegate_;
        static stego_disk::StegoStorage *stego_storage_;
        static stego_disk::uint64 capacity_;
        static bool fuse_mounted_;
        static const char *virtual_file_name_;

        static int Init(stego_disk::StegoStorage *stego_storage);

        static int MountFuse(const std::string &mount_point);

        static void UnmountFuse(const std::string &mount_point);

    };
}

#endif // STEGODISK_FUSE_FUSESERVICE_H_


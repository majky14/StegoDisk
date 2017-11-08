/**
* @file fuse_service.cc
* @author Martin Kosdy
* @author Matus Kysel
* @date 2016
* @brief File containing implementation of StegoStorage interface.
*
*/

#include "fuse_service.h"

#include <dirent.h>
#include <string.h>
#include <unistd.h>

#include "file_management/carrier_files_manager.h"
#include "encoders/hamming_encoder.h"
#include "permutations/affine_permutation.h"
#include "permutations/feistel_num_permutation.h"
#include "utils/stego_math.h"

namespace Stego {
#ifdef __APPLE__
    static const char  *file_path      = "/virtualdisc.dmg";
    const char* FuseService::virtual_file_name_ = "virtualdisc.dmg";
#else
    static const char *file_path = "/virtualdisc.iso";
    const char *FuseService::virtual_file_name_ = "virtualdisc.iso";
#endif

    static int sfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                           off_t offset, struct fuse_file_info *fi);

    static int sfs_getattr(const char *path, struct stat *stbuf);

    static int sfs_access(const char *path, int mask);

    static int sfs_readlink(const char *path, char *buf, size_t size);

    static int sfs_mkdir(const char *path, mode_t mode);

    static int sfs_unlink(const char *path);

    static int sfs_rmdir(const char *path);

    static int sfs_rename(const char *from, const char *to);

    static int sfs_chmod(const char *path, mode_t mode);

    static int sfs_chown(const char *path, uid_t uid, gid_t gid);

    static int sfs_open(const char *path, struct fuse_file_info *fi);

    static int sfs_create(const char *path, mode_t mode, struct fuse_file_info *fi);

    static int sfs_read(const char *path, char *buf, size_t size, off_t offset,
                        struct fuse_file_info *fi);

    static int sfs_write(const char *path, const char *buf, size_t size,
                         off_t offset, struct fuse_file_info *fi);

    static int sfs_truncate(const char *path, off_t size);

    static void sfs_destroy(void *unused);

    static void *sfs_init(fuse_conn_info *conn);

    stego_disk::StegoStorage *FuseService::stego_storage_ = nullptr;
    stego_disk::uint64 FuseService::capacity_ = 0;
//FuseServiceDelegate* FuseService::delegate_ = nullptr;
    bool FuseService::fuse_mounted_ = false;

// =============================================================================
//      MAIN
// =============================================================================


    int FuseService::Init(stego_disk::StegoStorage *stego_storage) {
        stego_storage_ = stego_storage;

        capacity_ = stego_storage_->GetSize();

        // fuse_operations struct initialization
        stegofs_ops.init = sfs_init;
        stegofs_ops.getattr = sfs_getattr;
        stegofs_ops.access = sfs_access;
        stegofs_ops.readlink = sfs_readlink;
        stegofs_ops.readdir = sfs_readdir;
        stegofs_ops.mkdir = sfs_mkdir;
        stegofs_ops.unlink = sfs_unlink;
        stegofs_ops.rmdir = sfs_rmdir;
        stegofs_ops.rename = sfs_rename;
        stegofs_ops.chmod = sfs_chmod;
        stegofs_ops.chown = sfs_chown;
        stegofs_ops.truncate = sfs_truncate;
        //stegofs_ops.utimens     = sfs_utimens;
        stegofs_ops.create = sfs_create;
        stegofs_ops.open = sfs_open;
        stegofs_ops.read = sfs_read;
        stegofs_ops.write = sfs_write;
        stegofs_ops.destroy = sfs_destroy;

        return 0;
    }

    int FuseService::MountFuse(const std::string &mount_point) {

        fuse_mounted_ = false;

        pid_t pid;
        if ((pid = fork()) < 0) {
            // TODO : ERROR
        }
        if (pid == 0) {
            return 0;
        } else {
            LOG_INFO("parent pid: " << getpid() << " : calling fuse_main");
            char *argv[10];
            memset(argv, 0, 10 * sizeof(char *));

            char *mnt_pt = new(nothrow) char[mount_point.size() + 1];
            if (!mnt_pt) {
                return -1;
            }
            strcpy(mnt_pt, mount_point.c_str());

            LOG_INFO("mount point: " << mnt_pt);

            argv[0] = "fuse";
            argv[1] = mnt_pt;
            argv[2] = "-f"; // zostava bezat na popredi - caka sa na koniec fuse_main
            argv[3] = "-o";
            argv[4] = "allow_other";
            argv[5] = NULL;
            int argc = 5;

//            argv[0] = "fuse";
//            argv[1] = mnt_pt;
//            argv[2] = "-o";
//            argv[3] = "allow_other";
//            argv[4] = NULL;
//            int argc = 4;

            LOG_INFO("calling fuse_main");
            int fuse_stat;

            fuse_stat = fuse_main(argc, argv, &stegofs_ops, nullptr);
            LOG_INFO("fuse_main returned " << fuse_stat);
            return 1;
        }
    }

    void FuseService::UnmountFuse(const std::string &mount_point) {
        LOG_INFO("unmounting: " << mount_point);

//    FuseService::stego_storage_->Save();
        fuse_mounted_ = false;

        fuse_unmount(mount_point.c_str(), nullptr);
    }


// =============================================================================
//      FUSE CALLBACK METHODS
// =============================================================================

    static int sfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                           off_t, struct fuse_file_info *) {// path, buf, filler, off_t offset, fi

        if (strcmp(path, "/") != 0) /* We only recognize the root directory. */
            return -ENOENT;

        if (!FuseService::fuse_mounted_) {
            FuseService::fuse_mounted_ = true;
        }

        filler(buf, ".", NULL, 0);           /* Current directory (.)  */
        filler(buf, "..", NULL, 0);          /* Parent directory (..)  */
        filler(buf, file_path + 1, NULL, 0); /* The only file we have. */

        return 0;
    }

    static int sfs_getattr(const char *path, struct stat *stbuf) {
//  memset(stbuf, 0, sizeof(struct stat));

        if (strcmp(path, "/") == 0) { /* The root directory of our file system. */
            stbuf->st_mode = S_IFDIR | 0755;
            stbuf->st_nlink = 2;
        } else if (strcmp(path, file_path) == 0) { /* The only file we have. */
            stbuf->st_mode = S_IFREG | 0777;
            stbuf->st_nlink = 1;
            stbuf->st_size = FuseService::capacity_;
            //stbuf->st_blocks = (FuseService::capacity_/512) + 1;
        } else /* We reject everything else. */
            return -ENOENT;

        return 0;
    }

    static int sfs_access(const char *, int) { // const char * path, int mask
        return 0;
    }

    static int sfs_readlink(const char *, char *, size_t) { // const char *path, char *buf, size_t size
        return -ENOSYS;
    }

    static int sfs_mkdir(const char *, mode_t) { // const char *path, mode_t mode
        return -EROFS;
    }

    static int sfs_unlink(const char *) { // const char *path
        return 0;
        //return -EROFS;
    }

    static int sfs_rmdir(const char *) { // const char *path
        return -EROFS;
    }

    static int sfs_rename(const char *, const char *) { // const char *from, const char *to
        return -EROFS;
    }

    static int sfs_chmod(const char *, mode_t) { // const char *path, mode_t mode
        return 0;
    }

    static int sfs_chown(const char *, uid_t, gid_t) { // const char *path, uid_t uid, gid_t gid
        return 0;
    }

    static int sfs_truncate(const char *, off_t) {
        return 0;
    }

//static int sfs_utimens(const char *path, const struct timespec ts[2])
//{
//	return 0;
//}

    static int sfs_open(const char *, struct fuse_file_info *) // const chr *path, struct fuse_file_info *fi
    {

        //if (strcmp(path, file_path) != 0) /* We only recognize one file. */
        //    return -ENOENT;

        //if ((fi->flags & O_ACCMODE) != O_RDONLY) /* Only reading allowed. */
        //    return -EACCES;

        return 0;
    }

    static int sfs_create(const char *path, mode_t, struct fuse_file_info *fi) { // path, mode_t mode, fi
        return sfs_open(path, fi);
    }


    static int sfs_read(const char *path, char *buf, size_t size, off_t offset,
                        struct fuse_file_info *) { // struct fuse_file_info *fi

        if (strcmp(path, file_path) != 0)
            return -ENOENT;

        stego_disk::uint64 offset64 = offset;
        stego_disk::uint64 size64 = size;

        if (offset64 >= FuseService::capacity_) /* Trying to read past the end of file. */ {
            LOG_ERROR("fuse service: offset+size > capacity_... offset:"
                              << offset << ", size:" << size << ", cap:"
                              << FuseService::capacity_);
            return 0;
        }

        if (offset64 + size64 > FuseService::capacity_) /* Trim the read to the file size. */ {
            LOG_ERROR("fuse service: offset+size > capacity_... offset:"
                              << offset << ", size:" << size << ", cap:"
                              << FuseService::capacity_);
            size64 = FuseService::capacity_ - offset64;
        }

        //int err = FuseService::virtualDisc->write((uint8*)buf, (uint32)size, offset);
        int err = 0;
        FuseService::stego_storage_->Read(buf, offset64, size64);

        if (err) {
            // TODO: treba nejak rozumne prelozit errory
            //LOG_ERROR("write error: " << err << ", offset: " << offset << ", size: " << size);
            return ENOMEM;
        } else {
            return static_cast<int>(size);
        }
    }

    static int sfs_write(const char *path, const char *buf, size_t size,
                         off_t offset, struct fuse_file_info *) { // struct fuse_file_info *fi

        if (strcmp(path, file_path) != 0)
            return -ENOENT;

        stego_disk::uint64 offset64 = offset;
        stego_disk::uint64 size64 = size;

        if (offset64 >= FuseService::capacity_) /* Trying to read past the end of file. */ {
            LOG_ERROR("fuse service: offset > capacity_... offset:"
                              << offset << ", size:" << size << ", cap:"
                              << FuseService::capacity_);
            return 0;
        }

        if (offset64 + size64 > FuseService::capacity_) /* Trim the read to the file size. */ {
            LOG_ERROR("fuse service: offset+size > capacity_... offset:"
                              << offset << ", size:" << size << ", cap:"
                              << FuseService::capacity_);
            size64 = FuseService::capacity_ - offset64;
        }

        //int err = FuseService::virtualDisc->write((uint8*)buf, (uint32)size, offset);
        int err = 0;
        FuseService::stego_storage_->Write(buf, offset64, size64);

        if (err) {
            // TODO: treba nejak rozumne prelozit errory
            //LOG_ERROR("write error: " << err << ", offset: " << offset << ", size: " << size);
            return ENOMEM;
        } else {
            return static_cast<int>(size);
        }
    }

    static void *sfs_init(struct fuse_conn_info *) {
        LOG_DEBUG("SFS_INIT CALLED");
        return nullptr;
    }

    static void sfs_destroy(void *) {
        FuseService::stego_storage_->Save();
        FuseService::fuse_mounted_ = false;
        LOG_INFO("SFS_DESTROY CALLED @pid: " << getpid());
        LOG_INFO("signaling parrent with id: " << getppid());
    }
}
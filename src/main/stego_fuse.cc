//
// Created by m4jky
//

#include "stego_fuse.h"

bool LoggerInit() {

    std::string logging_level("INFO");

    char *env_logging_level = NULL;
    if ((env_logging_level = getenv("LOGGING_LEVEL"))) {
        logging_level.assign(env_logging_level);
    }

    Logger::SetVerbosityLevel(logging_level, std::string("/tmp/stego_fuse_log.txt"));

    return true;
}

static void PrintHelp(char *name) {
    std::cerr << "Usage: " << name << " <option(s)> \n"
              << "Options:\n"
              << "\t-h,--help\t\tShow this help message\n"
              << "\t-f,--fuse <DIRECTORY>\tSpecify the directory for FUSE, if is blank default is used\n"
              << "\t-i,--img <DIRECTORY>\tSpecify the directory for images, if is blank default is used\n"
              << "\t-p,--password <PASSWORD>\tSpecify if the password should be used, if is blank default is used\n"
              << std::endl;
}

int main(int argc, char *argv[]) {

    if (!LoggerInit()) return -1;

    std::string dir = DST_DIRECTORY;
    std::string images = SRC_DIRECTORY;
    std::string password = PASSWORD;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "-h") || (arg == "--help")) {
            PrintHelp(argv[0]);
            return 0;
        } else if ((arg == "-f") || (arg == "--fuse")) {
            if (++i < argc) {
                dir = argv[i];
            } else {
                LOG_ERROR("--fuse option requires one argument.");
                return -1;
            }
        } else if ((arg == "-i") || (arg == "--img")) {
            if (++i < argc) {
                images = argv[i];
            } else {
                LOG_ERROR("--img option requires one argument.");
                return -1;
            }
        } else if ((arg == "-p") || (arg == "--password")) {
            if (++i < argc) {
                password = argv[i];
            } else {
                LOG_ERROR("--password option requires one argument.");
                return -1;
            }
        } else {
            LOG_ERROR("Unknown argument: " << argv[i]);
        }
    }

    std::unique_ptr<stego_disk::StegoStorage> stego_storage(new stego_disk::StegoStorage());
    stego_storage->Configure();
    LOG_INFO("Opening storage");
    stego_storage->Open(images, password);
    LOG_INFO("Loading storage");
    stego_storage->Load();
    size_t size = stego_storage->GetSize();
    LOG_INFO("Storage size = " << size << "B");

    if (Stego::FuseService::Init(stego_storage.get()) != 0) {
        return false;
    }

    if (Stego::FuseService::MountFuse(dir) != 0) {
        return false;
    }

    return 0;
}
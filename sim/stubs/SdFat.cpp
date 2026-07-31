#include "SdFat.h"
#include <sys/stat.h>
#include <cstdio>

#ifdef _WIN32
#include <direct.h>
#define SIM_MKDIR(p) _mkdir(p)
#else
#include <sys/types.h>
#define SIM_MKDIR(p) mkdir(p, 0755)
#endif

FsDateTime::callback_t FsDateTime::_cb = nullptr;

static std::string g_sdRoot = "sdcard";

const char* simSdRoot() { return g_sdRoot.c_str(); }
void simSetSdRoot(const char* path) { g_sdRoot = path ? path : "sdcard"; }

std::string simSdHostPath(const char* devicePath) {
    std::string p = devicePath ? devicePath : "";
    // Device paths are written both with and without a leading slash in the
    // firmware (that inconsistency is finding M6). Normalise here so the
    // simulator resolves them the same way a FAT root does.
    while (!p.empty() && (p[0] == '/' || p[0] == '\\')) p.erase(p.begin());
    return g_sdRoot + "/" + p;
}

// Creates any parent directories of a host path. Needed for the tile cache,
// which lives at /tiles/{z}/{x}/{y}.raw.
static void ensureParentDirs(const std::string& hostPath) {
    for (size_t i = 0; i < hostPath.size(); ++i) {
        if (hostPath[i] == '/' || hostPath[i] == '\\') {
            std::string dir = hostPath.substr(0, i);
            if (!dir.empty()) SIM_MKDIR(dir.c_str());
        }
    }
}

bool File32::open(const char* path, oflag_t mode) {
    close();
    const std::string host = simSdHostPath(path);

    const char* m;
    if (mode & O_TRUNC)            m = "w+b";   // FIT writer needs read-back
    else if (mode & O_APPEND)      m = "a+b";
    else if (mode & (O_RDWR))      m = "r+b";
    else if (mode & O_WRITE)       m = "r+b";
    else                           m = "rb";

    if (mode & O_CREAT) ensureParentDirs(host);

    _fp = fopen(host.c_str(), m);

    // "r+b" fails if the file does not exist; with O_CREAT the caller expects
    // it to be created.
    if (!_fp && (mode & O_CREAT)) {
        _fp = fopen(host.c_str(), "w+b");
    }
    return _fp != nullptr;
}

void File32::close() {
    if (_fp) { fclose(_fp); _fp = nullptr; }
}

int File32::read() {
    if (!_fp) return -1;
    int c = fgetc(_fp);
    return c == EOF ? -1 : c;
}

uint32_t File32::fileSize() {
    if (!_fp) return 0;
    long cur = ftell(_fp);
    fseek(_fp, 0, SEEK_END);
    long end = ftell(_fp);
    fseek(_fp, cur, SEEK_SET);
    return (uint32_t)end;
}

int File32::available() {
    if (!_fp) return 0;
    long cur = ftell(_fp);
    fseek(_fp, 0, SEEK_END);
    long end = ftell(_fp);
    fseek(_fp, cur, SEEK_SET);
    return (int)(end - cur);
}

int File32::peek() {
    if (!_fp) return -1;
    int c = fgetc(_fp);
    if (c != EOF) ungetc(c, _fp);
    return c == EOF ? -1 : c;
}

bool SdFat32::begin(uint8_t) {
    SIM_MKDIR(g_sdRoot.c_str());
    _mounted = true;
    Serial.print("[sim] SD root: "); Serial.println(g_sdRoot.c_str());
    return true;
}

bool SdFat32::exists(const char* path) {
    struct stat st;
    return stat(simSdHostPath(path).c_str(), &st) == 0;
}

bool SdFat32::remove(const char* path) {
    return ::remove(simSdHostPath(path).c_str()) == 0;
}

bool SdFat32::mkdir(const char* path, bool) {
    const std::string host = simSdHostPath(path);
    ensureParentDirs(host + "/");
    return SIM_MKDIR(host.c_str()) == 0;
}

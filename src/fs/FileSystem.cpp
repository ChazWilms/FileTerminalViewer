#include "FileSystem.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <ctime>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <shlwapi.h>  // PathCanonicalize
#pragma comment(lib, "shlwapi.lib")
#endif

namespace FS {

// -----------------------------------------------------------------------
// Internal helpers (Windows)
// -----------------------------------------------------------------------

#ifdef PLATFORM_WINDOWS

static std::string wstrToStr(const std::wstring& ws) {
    if (ws.empty()) return {};
    int needed = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string out(needed - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, &out[0], needed, nullptr, nullptr);
    return out;
}

static std::wstring strToWstr(const std::string& s) {
    if (s.empty()) return {};
    int needed = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring out(needed - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &out[0], needed);
    return out;
}

static std::string formatFileTime(const FILETIME& ft) {
    SYSTEMTIME st;
    FILETIME localFt;
    FileTimeToLocalFileTime(&ft, &localFt);
    FileTimeToSystemTime(&localFt, &st);
    char buf[32];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d",
             st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);
    return buf;
}

// -----------------------------------------------------------------------
// Directory listing
// -----------------------------------------------------------------------

std::vector<Entry> listDirectory(const std::string& path) {
    std::vector<Entry> dirs, files;

    std::wstring pattern = strToWstr(path + "\\*");
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(pattern.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return {};

    do {
        std::string name = wstrToStr(fd.cFileName);
        if (name == "." || name == "..") continue;

        Entry e;
        e.name     = name;
        e.modified = formatFileTime(fd.ftLastWriteTime);
        e.hidden   = (fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            e.type = EntryType::Directory;
            e.size = -1;
            dirs.push_back(std::move(e));
        } else if (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
            e.type = EntryType::Symlink;
            e.size = static_cast<long long>(fd.nFileSizeLow) |
                     (static_cast<long long>(fd.nFileSizeHigh) << 32);
            files.push_back(std::move(e));
        } else {
            e.type = EntryType::File;
            e.size = static_cast<long long>(fd.nFileSizeLow) |
                     (static_cast<long long>(fd.nFileSizeHigh) << 32);
            files.push_back(std::move(e));
        }
    } while (FindNextFileW(hFind, &fd));

    FindClose(hFind);

    // Sort each group case-insensitively by name.
    auto cmpName = [](const Entry& a, const Entry& b) {
        std::string la = a.name, lb = b.name;
        std::transform(la.begin(), la.end(), la.begin(), ::tolower);
        std::transform(lb.begin(), lb.end(), lb.begin(), ::tolower);
        return la < lb;
    };
    std::sort(dirs.begin(),  dirs.end(),  cmpName);
    std::sort(files.begin(), files.end(), cmpName);

    // Merge: directories first, then files.
    dirs.insert(dirs.end(), files.begin(), files.end());
    return dirs;
}

// -----------------------------------------------------------------------
// Path utilities
// -----------------------------------------------------------------------

std::string resolvePath(const std::string& path) {
    wchar_t buf[MAX_PATH];
    std::wstring wpath = strToWstr(path);
    if (!PathCanonicalizeW(buf, wpath.c_str())) return {};
    // Verify it exists.
    DWORD attr = GetFileAttributesW(buf);
    if (attr == INVALID_FILE_ATTRIBUTES) return {};
    return wstrToStr(buf);
}

std::string parentPath(const std::string& path) {
    if (path.size() <= 3) return path; // e.g. "C:\" is already root
    std::string p = path;
    // Remove trailing backslash if present.
    if (p.back() == '\\' || p.back() == '/') p.pop_back();
    size_t sep = p.find_last_of("\\/");
    if (sep == std::string::npos) return p;
    if (sep == 2 && p[1] == ':') return p.substr(0, 3); // "C:\"
    return p.substr(0, sep);
}

std::string joinPath(const std::string& base, const std::string& name) {
    if (base.empty()) return name;
    char last = base.back();
    if (last == '\\' || last == '/') return base + name;
    return base + "\\" + name;
}

std::string currentDirectory() {
    wchar_t buf[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, buf);
    return wstrToStr(buf);
}

// -----------------------------------------------------------------------
// File operations
// -----------------------------------------------------------------------

bool makeDirectory(const std::string& path) {
    return CreateDirectoryW(strToWstr(path).c_str(), nullptr) != 0;
}

bool makeFile(const std::string& path) {
    HANDLE h = CreateFileW(strToWstr(path).c_str(),
                           GENERIC_WRITE, 0, nullptr,
                           CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    CloseHandle(h);
    return true;
}

bool deleteFile(const std::string& path) {
    return DeleteFileW(strToWstr(path).c_str()) != 0;
}

bool deleteDirectory(const std::string& path) {
    return RemoveDirectoryW(strToWstr(path).c_str()) != 0;
}

bool deleteDirectoryRecursive(const std::string& path) {
    std::wstring wpath = strToWstr(path);
    std::wstring pattern = wpath + L"\\*";
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(pattern.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return false;

    bool ok = true;
    do {
        std::wstring name = fd.cFileName;
        if (name == L"." || name == L"..") continue;
        std::string child = joinPath(path, wstrToStr(name));

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            ok &= deleteDirectoryRecursive(child);
        } else {
            // Clear read-only attribute before deleting.
            SetFileAttributesW(strToWstr(child).c_str(), FILE_ATTRIBUTE_NORMAL);
            ok &= (DeleteFileW(strToWstr(child).c_str()) != 0);
        }
    } while (FindNextFileW(hFind, &fd));
    FindClose(hFind);

    ok &= (RemoveDirectoryW(wpath.c_str()) != 0);
    return ok;
}

static bool copyFileEntry(const std::string& src, const std::string& dst) {
    return CopyFileW(strToWstr(src).c_str(), strToWstr(dst).c_str(), FALSE) != 0;
}

bool copyEntry(const std::string& src, const std::string& dst) {
    if (isDirectory(src)) {
        if (!makeDirectory(dst)) return false;
        auto entries = listDirectory(src);
        bool ok = true;
        for (auto& e : entries) {
            ok &= copyEntry(joinPath(src, e.name), joinPath(dst, e.name));
        }
        return ok;
    }
    return copyFileEntry(src, dst);
}

bool moveEntry(const std::string& src, const std::string& dst) {
    return MoveFileExW(strToWstr(src).c_str(), strToWstr(dst).c_str(),
                       MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED) != 0;
}

bool exists(const std::string& path) {
    DWORD attr = GetFileAttributesW(strToWstr(path).c_str());
    return attr != INVALID_FILE_ATTRIBUTES;
}

bool isDirectory(const std::string& path) {
    DWORD attr = GetFileAttributesW(strToWstr(path).c_str());
    return (attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

#endif // PLATFORM_WINDOWS

} // namespace FS

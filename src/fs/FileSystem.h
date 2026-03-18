#pragma once
#include <string>
#include <vector>

// FileSystem wraps all directory/file operations.
// Platform-specific code is isolated inside FileSystem.cpp so that a
// future Linux port only needs to swap the implementation.

namespace FS {

enum class EntryType {
    Directory,
    File,
    Symlink,
    Unknown,
};

struct Entry {
    std::string name;       // display name (no path prefix)
    EntryType   type;
    long long   size;       // bytes; -1 for directories
    std::string modified;   // formatted last-modified string
    bool        hidden = false; // FILE_ATTRIBUTE_HIDDEN or dot-prefix
};

// Return a sorted list of entries in the given directory.
// Directories come before files; both groups sorted A-Z case-insensitively.
// Returns empty vector on error.
std::vector<Entry> listDirectory(const std::string& path);

// Change to the given path. Returns canonical (resolved) path on success,
// empty string on failure.
std::string resolvePath(const std::string& path);

// Return the parent directory of path. If already at root, return path as-is.
std::string parentPath(const std::string& path);

// Join two path segments with the correct separator.
std::string joinPath(const std::string& base, const std::string& name);

// Return the current working directory.
std::string currentDirectory();

// --- File operations ---

// Create a directory (non-recursive). Returns true on success.
bool makeDirectory(const std::string& path);

// Create an empty file. Returns true on success.
bool makeFile(const std::string& path);

// Delete a single file. Returns true on success.
bool deleteFile(const std::string& path);

// Delete an empty directory. Returns true on success.
bool deleteDirectory(const std::string& path);

// Recursively delete a directory and all its contents. Returns true on success.
bool deleteDirectoryRecursive(const std::string& path);

// Copy src to dst. If src is a directory, copies recursively.
// Returns true on success.
bool copyEntry(const std::string& src, const std::string& dst);

// Move/rename src to dst. Returns true on success.
bool moveEntry(const std::string& src, const std::string& dst);

// Returns true if path exists (file or directory).
bool exists(const std::string& path);

// Returns true if path is a directory.
bool isDirectory(const std::string& path);

} // namespace FS

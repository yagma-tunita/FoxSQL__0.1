#pragma once
#include <string>
#include <windows.h>

namespace FoxSQL {
    namespace fs {

        inline bool createDirectory(const std::string& path) {
            return CreateDirectoryA(path.c_str(), NULL) != 0;
        }

        inline bool createDirectories(const std::string& path) {
            std::string cur;
            size_t pos = 0;
            while ((pos = path.find('\\', pos)) != std::string::npos) {
                cur = path.substr(0, pos);
                if (!cur.empty() && !createDirectory(cur)) {
                    if (GetLastError() != ERROR_ALREADY_EXISTS) return false;
                }
                ++pos;
            }
            return createDirectory(path) || GetLastError() == ERROR_ALREADY_EXISTS;
        }

        inline bool fileExists(const std::string& path) {
            DWORD attr = GetFileAttributesA(path.c_str());
            return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
        }

        inline bool directoryExists(const std::string& path) {
            DWORD attr = GetFileAttributesA(path.c_str());
            return (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
        }

        inline bool removeFile(const std::string& path) {
            return DeleteFileA(path.c_str()) != 0;
        }

        inline std::string getDataDir() {
            return "C:\\FoxOS\\Data\\";
        }

        inline void initDataDir() {
            std::string dataDir = getDataDir();
            createDirectories(dataDir);
        }

    } 
} 
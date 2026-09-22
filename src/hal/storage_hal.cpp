#include "storage_hal.h"
#include <SPIFFS.h>

StorageHAL Storage;

StorageHAL::StorageHAL()
    : _sdMounted(false), _flashMounted(false)
{
}

bool StorageHAL::begin() {
    // 1. SPIFFS / internes Flash initialisieren
    if (SPIFFS.begin(true)) {
        _flashMounted = true;
        Serial.println("[STORAGE] Internal Flash (SPIFFS) mounted.");
    } else {
        _flashMounted = false;
        Serial.println("[STORAGE] Internal Flash mount failed.");
    }

    // 2. MicroSD-Karte initialisieren mit Pins aus hardware_config.h
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    if (SD.begin(SD_CS, SPI, 4000000)) {
        _sdMounted = true;
        Serial.println("[STORAGE] MicroSD Card mounted.");
    } else {
        _sdMounted = false;
        Serial.println("[STORAGE] MicroSD Card not detected.");
    }

    return _sdMounted || _flashMounted;
}

void StorageHAL::scanDirectory(FS& fs, const String& dirname, const String& ext, std::vector<String>& result, const String& prefix) {
    File root = fs.open(dirname);
    if (!root || !root.isDirectory()) return;

    File file = root.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            String name = file.name();
            if (!name.startsWith("/")) {
                name = "/" + name;
            }

            bool match = false;
            if (ext.length() == 0) {
                match = true;
            } else {
                String lowerName = name;
                lowerName.toLowerCase();
                String lowerExt = ext;
                lowerExt.toLowerCase();
                if (lowerName.endsWith(lowerExt)) {
                    match = true;
                }
            }

            if (match) {
                result.push_back(prefix + name);
            }
        }
        file.close();
        file = root.openNextFile();
    }
    root.close();
}

std::vector<String> StorageHAL::listFiles(const String& directory, const String& extension) {
    std::vector<String> result;

    if (_sdMounted) {
        scanDirectory(SD, directory, extension, result, "");
    }

    if (_flashMounted) {
        if (!_sdMounted || directory.startsWith("/flash")) {
            scanDirectory(SPIFFS, "/", extension, result, "");
        }
    }

    return result;
}

std::vector<FileEntry> StorageHAL::listDirectory(const String& path) {
    std::vector<FileEntry> result;

    if (_sdMounted) {
        File root = SD.open(path);
        if (root && root.isDirectory()) {
            File file = root.openNextFile();
            while (file) {
                FileEntry entry;
                entry.name = file.name();
                // Wenn der Name einen führenden Pfad hat, nur den Dateinamen extrahieren
                int lastSlash = entry.name.lastIndexOf('/');
                if (lastSlash >= 0) {
                    entry.name = entry.name.substring(lastSlash + 1);
                }
                entry.size = file.size();
                entry.isDirectory = file.isDirectory();
                result.push_back(entry);
                file.close();
                file = root.openNextFile();
            }
            root.close();
            return result;
        }
    }

    if (_flashMounted && (!_sdMounted || path.startsWith("/flash"))) {
        File root = SPIFFS.open("/");
        if (root) {
            File file = root.openNextFile();
            while (file) {
                FileEntry entry;
                entry.name = file.name();
                if (entry.name.startsWith("/")) entry.name = entry.name.substring(1);
                entry.size = file.size();
                entry.isDirectory = file.isDirectory();
                result.push_back(entry);
                file.close();
                file = root.openNextFile();
            }
            root.close();
        }
    }

    return result;
}

File StorageHAL::openFile(const String& path, const char* mode) {
    if (_sdMounted && SD.exists(path)) {
        return SD.open(path, mode);
    }
    if (_flashMounted && SPIFFS.exists(path)) {
        return SPIFFS.open(path, mode);
    }
    if (_sdMounted) {
        return SD.open(path, mode);
    }
    if (_flashMounted) {
        return SPIFFS.open(path, mode);
    }
    return File();
}

bool StorageHAL::exists(const String& path) {
    if (_sdMounted && SD.exists(path)) return true;
    if (_flashMounted && SPIFFS.exists(path)) return true;
    return false;
}

bool StorageHAL::isDirectory(const String& path) {
    if (path == "/" || path == "") return true;
    if (_sdMounted) {
        File f = SD.open(path);
        if (f) {
            bool isDir = f.isDirectory();
            f.close();
            return isDir;
        }
    }
    return false;
}

bool StorageHAL::makeDirectory(const String& path) {
    if (_sdMounted) {
        return SD.mkdir(path);
    }
    if (_flashMounted) {
        return SPIFFS.mkdir(path);
    }
    return false;
}

bool StorageHAL::removeDirectory(const String& path) {
    if (_sdMounted) {
        return SD.rmdir(path);
    }
    if (_flashMounted) {
        return SPIFFS.rmdir(path);
    }
    return false;
}

bool StorageHAL::remove(const String& path) {
    if (_sdMounted && SD.exists(path)) {
        return SD.remove(path);
    }
    if (_flashMounted && SPIFFS.exists(path)) {
        return SPIFFS.remove(path);
    }
    return false;
}

bool StorageHAL::renameFile(const String& src, const String& dst) {
    if (_sdMounted && SD.exists(src)) {
        return SD.rename(src, dst);
    }
    if (_flashMounted && SPIFFS.exists(src)) {
        return SPIFFS.rename(src, dst);
    }
    return false;
}

bool StorageHAL::copyFile(const String& src, const String& dst) {
    File in = openFile(src, FILE_READ);
    if (!in) return false;

    File out = openFile(dst, FILE_WRITE);
    if (!out) {
        in.close();
        return false;
    }

    uint8_t buffer[512];
    while (in.available()) {
        size_t n = in.read(buffer, sizeof(buffer));
        if (n > 0) {
            out.write(buffer, n);
        }
    }

    in.close();
    out.close();
    return true;
}

String StorageHAL::resolvePath(const String& baseDir, const String& inputPath) {
    String fullPath;
    if (inputPath.startsWith("/")) {
        fullPath = inputPath;
    } else {
        if (baseDir.endsWith("/")) {
            fullPath = baseDir + inputPath;
        } else {
            fullPath = baseDir + "/" + inputPath;
        }
    }

    // Pfad normalisieren (. und .. auflösen)
    std::vector<String> parts;
    int start = 0;
    while (start < (int)fullPath.length()) {
        int end = fullPath.indexOf('/', start);
        if (end == -1) end = fullPath.length();
        String part = fullPath.substring(start, end);
        if (part == ".." && !parts.empty()) {
            parts.pop_back();
        } else if (part != "." && part != "") {
            parts.push_back(part);
        }
        start = end + 1;
    }

    String normalized = "/";
    for (size_t i = 0; i < parts.size(); ++i) {
        normalized += parts[i];
        if (i < parts.size() - 1) normalized += "/";
    }
    return normalized;
}

void StorageHAL::getStorageInfo(uint32_t& totalKB, uint32_t& usedKB) {
    totalKB = 0;
    usedKB = 0;
    if (_sdMounted) {
        totalKB = SD.totalBytes() / 1024;
        usedKB = SD.usedBytes() / 1024;
    } else if (_flashMounted) {
        totalKB = SPIFFS.totalBytes() / 1024;
        usedKB = SPIFFS.usedBytes() / 1024;
    }
}

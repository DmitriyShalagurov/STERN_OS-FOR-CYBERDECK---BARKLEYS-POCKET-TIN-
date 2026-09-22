#ifndef STORAGE_HAL_H
#define STORAGE_HAL_H

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <FS.h>
#include <vector>
#include "hardware_config.h"

// =============================================================================
// StorageHAL: Vereinheitlichter Zugriff auf SD-Karte und internes Flash
// Bietet POSIX-ähnliche Dateisystem-Funktionen für Linux-Shell & Emulatoren
// =============================================================================

struct FileEntry {
    String name;
    size_t size;
    bool isDirectory;
};

class StorageHAL {
public:
    StorageHAL();

    // Initialisiert SD-Karte und internes Flash
    bool begin();

    bool isSdMounted() const { return _sdMounted; }
    bool isFlashMounted() const { return _flashMounted; }

    // Dateisuche mit Filter (z.B. ".nes", ".gb", ".ch8", ".txt")
    std::vector<String> listFiles(const String& directory = "/", const String& extension = "");

    // Detaillierte Verzeichnisauflistung (Dateien & Ordner mit Typ und Größe)
    std::vector<FileEntry> listDirectory(const String& path = "/");

    // Öffnet eine Datei (sucht zuerst auf SD, dann im internen Flash)
    File openFile(const String& path, const char* mode = FILE_READ);

    // Dateiverwaltung
    bool exists(const String& path);
    bool remove(const String& path);
    bool isDirectory(const String& path);
    bool makeDirectory(const String& path);
    bool removeDirectory(const String& path);
    bool copyFile(const String& src, const String& dst);
    bool renameFile(const String& src, const String& dst);

    // Pfadauflösung (unterstützt relative Pfade wie .., ./, /etc)
    String resolvePath(const String& baseDir, const String& relativeOrAbsPath);

    // Gibt den freien und gesamten Speicherplatz zurück (in KB)
    void getStorageInfo(uint32_t& totalKB, uint32_t& usedKB);

private:
    bool _sdMounted;
    bool _flashMounted;

    void scanDirectory(FS& fs, const String& dirname, const String& ext, std::vector<String>& result, const String& prefix);
};

extern StorageHAL Storage;

#endif // STORAGE_HAL_H

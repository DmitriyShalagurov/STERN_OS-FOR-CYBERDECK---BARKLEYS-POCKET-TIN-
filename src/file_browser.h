#ifndef FILE_BROWSER_H
#define FILE_BROWSER_H

#include <Arduino.h>
#include <vector>

extern bool fileBrowserActive;
extern std::vector<String> fileList;
extern int selectedIndex;

void openFileBrowser();
void drawFileBrowser();
void closeFileBrowser();
void confirmAndDeleteFile();
void setRawImageJustViewed();   

#endif
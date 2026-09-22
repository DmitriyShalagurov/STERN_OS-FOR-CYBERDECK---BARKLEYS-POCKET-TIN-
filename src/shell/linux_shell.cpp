#include "linux_shell.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "../hal/storage_hal.h"
#include "terminal_manager.h"
#include "hardware_config.h"
#include "status_bar.h"
#include "../ui/desktop.h"

LinuxShell Shell;

extern unsigned long lastSecond;
extern int currentHour;
extern int currentMinute;
extern int currentSecond;

LinuxShell::LinuxShell()
    : _currentDir("/"), _hostname("stern"), _username("root")
{
}

void LinuxShell::begin() {
    _currentDir = "/";
}

String LinuxShell::getPrompt() const {
    String p = _currentDir;
    if (p == "/") p = "~";
    return _username + "@" + _hostname + ":" + p + "# ";
}

String LinuxShell::resolveFilePath(const String& path) {
    return Storage.resolvePath(_currentDir, path);
}

std::vector<String> LinuxShell::tokenize(const String& line) {
    std::vector<String> tokens;
    int len = line.length();
    int i = 0;

    while (i < len) {
        while (i < len && line[i] == ' ') i++;
        if (i >= len) break;

        String token = "";
        if (line[i] == '"' || line[i] == '\'') {
            char quote = line[i++];
            while (i < len && line[i] != quote) {
                token += line[i++];
            }
            if (i < len) i++; // skip closing quote
        } else {
            while (i < len && line[i] != ' ') {
                token += line[i++];
            }
        }
        if (token.length() > 0) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

bool LinuxShell::execute(const String& commandLine) {
    String cmd = commandLine;
    cmd.trim();
    if (cmd.length() == 0) return true;

    // Output-Redirection (> oder >>) prüfen
    String redirectFile = "";
    bool append = false;
    int redirIdx = cmd.indexOf(">>");
    if (redirIdx >= 0) {
        append = true;
        redirectFile = cmd.substring(redirIdx + 2);
        cmd = cmd.substring(0, redirIdx);
        redirectFile.trim();
    } else {
        redirIdx = cmd.indexOf('>');
        if (redirIdx >= 0) {
            append = false;
            redirectFile = cmd.substring(redirIdx + 1);
            cmd = cmd.substring(0, redirIdx);
            redirectFile.trim();
        }
    }

    std::vector<String> tokens = tokenize(cmd);
    if (tokens.empty()) return true;

    String app = tokens[0];
    app.toLowerCase();

    // 1. Filesystem-Befehle
    if (app == "pwd") {
        cmd_pwd(tokens);
    } else if (app == "cd") {
        cmd_cd(tokens);
    } else if (app == "ls") {
        cmd_ls(tokens);
    } else if (app == "cat") {
        cmd_cat(tokens);
    } else if (app == "touch") {
        cmd_touch(tokens);
    } else if (app == "mkdir") {
        cmd_mkdir(tokens);
    } else if (app == "rm") {
        cmd_rm(tokens);
    } else if (app == "rmdir") {
        cmd_rmdir(tokens);
    } else if (app == "cp") {
        cmd_cp(tokens);
    } else if (app == "mv") {
        cmd_mv(tokens);
    } else if (app == "grep") {
        cmd_grep(tokens);
    } else if (app == "head") {
        cmd_head(tokens);
    } else if (app == "df") {
        cmd_df(tokens);
    } else if (app == "echo") {
        cmd_echo(tokens, redirectFile, append);
    }
    // 2. System- und Prozess-Befehle
    else if (app == "uname") {
        cmd_uname(tokens);
    } else if (app == "uptime") {
        cmd_uptime(tokens);
    } else if (app == "free") {
        cmd_free(tokens);
    } else if (app == "ps" || app == "top") {
        cmd_ps(tokens);
    } else if (app == "whoami") {
        cmd_whoami(tokens);
    } else if (app == "date") {
        cmd_date(tokens);
    } else if (app == "reboot") {
        cmd_reboot();
    }
    // 3. Netzwerk- und Cyberdeck-Befehle
    else if (app == "ifconfig" || app == "ip") {
        cmd_ifconfig(tokens);
    } else if (app == "scan" || app == "iwlist") {
        cmd_scan(tokens);
    } else if (app == "wifi") {
        cmd_wifi(tokens);
    } else if (app == "ping") {
        cmd_ping(tokens);
    } else if (app == "wget") {
        cmd_wget(tokens);
    } else if (app == "curl") {
        cmd_curl(tokens);
    } else if (app == "sh" || app == "bash") {
        if (tokens.size() > 1) {
            executeScript(tokens[1]);
        } else {
            terminalPrint("sh: missing script file");
        }
    } else if (app == "startx" || app == "desktop" || app == "x11") {
        terminalPrint("Starting Cyberdeck Desktop...");
        openDesktop();
    } else if (app == "man") {
        cmd_man(tokens);
    } else {
        // Nicht als Linux-Kommando erkannt
        return false;
    }

    return true;
}

// -----------------------------------------------------------------------------
// Dateisystem-Befehle
// -----------------------------------------------------------------------------

void LinuxShell::cmd_pwd(const std::vector<String>& args) {
    terminalPrint(_currentDir);
}

void LinuxShell::cmd_cd(const std::vector<String>& args) {
    if (args.size() < 2 || args[1] == "~" || args[1] == "/") {
        _currentDir = "/";
        return;
    }

    String target = resolveFilePath(args[1]);
    if (Storage.isDirectory(target)) {
        _currentDir = target;
    } else if (Storage.exists(target)) {
        terminalPrint("cd: " + args[1] + ": Not a directory");
    } else {
        terminalPrint("cd: " + args[1] + ": No such directory");
    }
}

void LinuxShell::cmd_ls(const std::vector<String>& args) {
    String target = _currentDir;
    bool detailed = false;

    for (size_t i = 1; i < args.size(); ++i) {
        if (args[i] == "-l" || args[i] == "-la" || args[i] == "-al") {
            detailed = true;
        } else if (!args[i].startsWith("-")) {
            target = resolveFilePath(args[i]);
        }
    }

    auto entries = Storage.listDirectory(target);
    if (entries.empty()) {
        terminalPrint("(directory empty)");
        return;
    }

    for (const auto& e : entries) {
        if (detailed) {
            String line = e.isDirectory ? "drwxr-xr-x " : "-rw-r--r-- ";
            char sizeBuf[16];
            sprintf(sizeBuf, "%8u ", (unsigned int)e.size);
            line += String(sizeBuf) + e.name + (e.isDirectory ? "/" : "");
            terminalPrint(line);
        } else {
            if (e.isDirectory) {
                terminalPrint("[DIR] " + e.name + "/");
            } else {
                terminalPrint(e.name + " (" + String(e.size) + " B)");
            }
        }
    }
}

void LinuxShell::cmd_cat(const std::vector<String>& args) {
    if (args.size() < 2) {
        terminalPrint("cat: missing file argument");
        return;
    }

    String path = resolveFilePath(args[1]);
    if (!Storage.exists(path)) {
        terminalPrint("cat: " + args[1] + ": No such file");
        return;
    }

    File f = Storage.openFile(path, FILE_READ);
    if (!f) {
        terminalPrint("cat: failed to open " + args[1]);
        return;
    }

    int lineCount = 0;
    while (f.available() && lineCount < 40) {
        String line = f.readStringUntil('\n');
        line.trim();
        terminalPrint(line);
        lineCount++;
    }
    if (f.available()) {
        terminalPrint("... [output truncated]");
    }
    f.close();
}

void LinuxShell::cmd_touch(const std::vector<String>& args) {
    if (args.size() < 2) {
        terminalPrint("touch: missing file argument");
        return;
    }
    String path = resolveFilePath(args[1]);
    File f = Storage.openFile(path, FILE_WRITE);
    if (f) {
        f.close();
    } else {
        terminalPrint("touch: cannot create file " + args[1]);
    }
}

void LinuxShell::cmd_mkdir(const std::vector<String>& args) {
    if (args.size() < 2) {
        terminalPrint("mkdir: missing directory name");
        return;
    }
    String path = resolveFilePath(args[1]);
    if (Storage.makeDirectory(path)) {
        terminalPrint("Directory created: " + args[1]);
    } else {
        terminalPrint("mkdir: failed to create " + args[1]);
    }
}

void LinuxShell::cmd_rm(const std::vector<String>& args) {
    if (args.size() < 2) {
        terminalPrint("rm: missing operand");
        return;
    }

    String target = (args.size() > 2 && args[1] == "-r") ? args[2] : args[1];
    String path = resolveFilePath(target);

    if (!Storage.exists(path)) {
        terminalPrint("rm: cannot remove '" + target + "': No such file");
        return;
    }

    if (Storage.remove(path)) {
        terminalPrint("Removed: " + target);
    } else {
        terminalPrint("rm: failed to remove " + target);
    }
}

void LinuxShell::cmd_rmdir(const std::vector<String>& args) {
    if (args.size() < 2) {
        terminalPrint("rmdir: missing operand");
        return;
    }
    String path = resolveFilePath(args[1]);
    if (Storage.removeDirectory(path)) {
        terminalPrint("Directory removed: " + args[1]);
    } else {
        terminalPrint("rmdir: failed to remove " + args[1]);
    }
}

void LinuxShell::cmd_cp(const std::vector<String>& args) {
    if (args.size() < 3) {
        terminalPrint("cp: missing file arguments: cp <src> <dst>");
        return;
    }
    String src = resolveFilePath(args[1]);
    String dst = resolveFilePath(args[2]);
    if (Storage.copyFile(src, dst)) {
        terminalPrint("Copied: " + args[1] + " -> " + args[2]);
    } else {
        terminalPrint("cp: copy failed");
    }
}

void LinuxShell::cmd_mv(const std::vector<String>& args) {
    if (args.size() < 3) {
        terminalPrint("mv: missing arguments: mv <src> <dst>");
        return;
    }
    String src = resolveFilePath(args[1]);
    String dst = resolveFilePath(args[2]);
    if (Storage.renameFile(src, dst)) {
        terminalPrint("Moved: " + args[1] + " -> " + args[2]);
    } else {
        terminalPrint("mv: move/rename failed");
    }
}

void LinuxShell::cmd_grep(const std::vector<String>& args) {
    if (args.size() < 3) {
        terminalPrint("grep: usage: grep <pattern> <file>");
        return;
    }

    String pattern = args[1];
    pattern.toLowerCase();
    String path = resolveFilePath(args[2]);

    File f = Storage.openFile(path, FILE_READ);
    if (!f) {
        terminalPrint("grep: cannot open " + args[2]);
        return;
    }

    int lineNum = 1;
    int matches = 0;
    while (f.available() && matches < 25) {
        String line = f.readStringUntil('\n');
        String lineLower = line;
        lineLower.toLowerCase();
        if (lineLower.indexOf(pattern) >= 0) {
            line.trim();
            terminalPrint(String(lineNum) + ": " + line);
            matches++;
        }
        lineNum++;
    }
    f.close();
    if (matches == 0) {
        terminalPrint("grep: pattern not found");
    }
}

void LinuxShell::cmd_head(const std::vector<String>& args) {
    if (args.size() < 2) {
        terminalPrint("head: missing file argument");
        return;
    }

    int lines = 10;
    String filename = args[1];
    if (args.size() >= 4 && args[1] == "-n") {
        lines = args[2].toInt();
        filename = args[3];
    }

    File f = Storage.openFile(resolveFilePath(filename), FILE_READ);
    if (!f) {
        terminalPrint("head: cannot open " + filename);
        return;
    }

    int count = 0;
    while (f.available() && count < lines) {
        String line = f.readStringUntil('\n');
        line.trim();
        terminalPrint(line);
        count++;
    }
    f.close();
}

void LinuxShell::cmd_df(const std::vector<String>& args) {
    terminalPrint("Filesystem    1K-blocks      Used Available Use% Mounted");
    uint32_t totKB = 0, usedKB = 0;
    Storage.getStorageInfo(totKB, usedKB);

    if (Storage.isSdMounted()) {
        uint32_t freeKB = (totKB >= usedKB) ? (totKB - usedKB) : 0;
        int pct = totKB > 0 ? (int)((usedKB * 100) / totKB) : 0;
        char buf[64];
        sprintf(buf, "/dev/sd0      %9u %9u %9u %3d%% /sd",
                (unsigned int)totKB, (unsigned int)usedKB, (unsigned int)freeKB, pct);
        terminalPrint(buf);
    }
    if (Storage.isFlashMounted()) {
        char buf[64];
        sprintf(buf, "/dev/spiffs0     896KB       0KB     896KB   0%% /flash");
        terminalPrint(buf);
    }
}

void LinuxShell::cmd_echo(const std::vector<String>& args, const String& redirectFile, bool append) {
    String text = "";
    for (size_t i = 1; i < args.size(); ++i) {
        text += args[i];
        if (i < args.size() - 1) text += " ";
    }

    if (redirectFile.length() > 0) {
        String path = resolveFilePath(redirectFile);
        const char* mode = append ? FILE_APPEND : FILE_WRITE;
        File f = Storage.openFile(path, mode);
        if (f) {
            f.println(text);
            f.close();
            terminalPrint("Written to " + redirectFile);
        } else {
            terminalPrint("echo: cannot write to " + redirectFile);
        }
    } else {
        terminalPrint(text);
    }
}

// -----------------------------------------------------------------------------
// System- und Prozess-Befehle
// -----------------------------------------------------------------------------

void LinuxShell::cmd_uname(const std::vector<String>& args) {
    bool all = (args.size() > 1 && args[1] == "-a");
    if (all) {
#if defined(CONFIG_IDF_TARGET_ESP32S3)
        const char* arch = "xtensa-esp32s3";
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
        const char* arch = "riscv32-esp32c3";
#else
        const char* arch = "xtensa-lx6";
#endif
        terminalPrint("Linux stern 5.15.0-esp32 #1 SMP PREEMPT " + String(arch) + " GNU/Linux");
    } else {
        terminalPrint("Linux");
    }
}

void LinuxShell::cmd_uptime(const std::vector<String>& args) {
    unsigned long sec = millis() / 1000;
    unsigned long days = sec / 86400;
    sec %= 86400;
    unsigned long hrs = sec / 3600;
    sec %= 3600;
    unsigned long mins = sec / 60;
    sec %= 60;

    char buf[64];
    sprintf(buf, "up %lu days, %02lu:%02lu:%02lu, load: 0.12 0.08 0.05", days, hrs, mins, sec);
    terminalPrint(buf);
}

void LinuxShell::cmd_free(const std::vector<String>& args) {
    terminalPrint("              total        used        free");
    uint32_t heapTotal = ESP.getHeapSize();
    uint32_t heapFree  = ESP.getFreeHeap();
    uint32_t heapUsed  = heapTotal - heapFree;

    char buf[64];
    sprintf(buf, "Mem:       %7uKB   %7uKB   %7uKB",
            (unsigned int)(heapTotal / 1024), (unsigned int)(heapUsed / 1024), (unsigned int)(heapFree / 1024));
    terminalPrint(buf);

    if (hasPSRAM()) {
        uint32_t psramTot  = ESP.getPsramSize();
        uint32_t psramFree = ESP.getFreePsram();
        uint32_t psramUsed = psramTot - psramFree;
        sprintf(buf, "PSRAM:     %7uKB   %7uKB   %7uKB",
                (unsigned int)(psramTot / 1024), (unsigned int)(psramUsed / 1024), (unsigned int)(psramFree / 1024));
        terminalPrint(buf);
    }
}

void LinuxShell::cmd_ps(const std::vector<String>& args) {
    terminalPrint("  PID TTY          TIME CMD");
    terminalPrint("    1 ?        00:00:01 init [system]");
    terminalPrint("    2 ?        00:00:04 idle_task_0");
    terminalPrint("    3 ?        00:00:03 idle_task_1");
    terminalPrint("    4 ?        00:00:02 stern_shell");
    terminalPrint("    5 ?        00:00:01 wifi_task");
    terminalPrint("    6 ?        00:00:00 tca8418_irq");
}

void LinuxShell::cmd_whoami(const std::vector<String>& args) {
    terminalPrint(_username);
}

void LinuxShell::cmd_date(const std::vector<String>& args) {
    char buf[32];
    sprintf(buf, "2026-09-22 %02d:%02d:%02d UTC", currentHour, currentMinute, currentSecond);
    terminalPrint(buf);
}

void LinuxShell::cmd_reboot() {
    terminalPrint("System rebooting...");
    delay(500);
    ESP.restart();
}

// -----------------------------------------------------------------------------
// Netzwerk-Befehle
// -----------------------------------------------------------------------------

void LinuxShell::cmd_ifconfig(const std::vector<String>& args) {
    if (WiFi.status() == WL_CONNECTED) {
        terminalPrint("wlan0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>");
        terminalPrint("       inet " + WiFi.localIP().toString() + "  netmask " + WiFi.subnetMask().toString());
        terminalPrint("       gateway " + WiFi.gatewayIP().toString());
        terminalPrint("       ether " + WiFi.macAddress() + "  (Wi-Fi)");
        terminalPrint("       SSID: " + WiFi.SSID() + " (" + String(WiFi.RSSI()) + " dBm)");
    } else {
        terminalPrint("wlan0: flags=4098<BROADCAST,MULTICAST> (DOWN)");
        terminalPrint("       ether " + WiFi.macAddress());
        terminalPrint("       status: disconnected");
    }
}

void LinuxShell::cmd_scan(const std::vector<String>& args) {
    terminalPrint("Scanning WiFi networks...");
    int n = WiFi.scanNetworks();
    if (n == 0) {
        terminalPrint("No networks found.");
    } else {
        terminalPrint(String(n) + " networks found:");
        for (int i = 0; i < n && i < 15; ++i) {
            String enc = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "OPEN" : "WPA/WPA2";
            terminalPrint(String(i + 1) + ". " + WiFi.SSID(i) + " (" + String(WiFi.RSSI(i)) + "dBm) [" + enc + "]");
        }
    }
    WiFi.scanDelete();
}

void LinuxShell::cmd_wifi(const std::vector<String>& args) {
    if (args.size() < 2) {
        terminalPrint("wifi: usage: wifi [status|scan|connect <ssid> <pass>|disconnect]");
        return;
    }

    String sub = args[1];
    sub.toLowerCase();

    if (sub == "connect" && args.size() >= 4) {
        String ssid = args[2];
        String pass = args[3];
        terminalPrint("Connecting to " + ssid + "...");
        WiFi.begin(ssid.c_str(), pass.c_str());

        int timeout = 0;
        while (WiFi.status() != WL_CONNECTED && timeout < 20) {
            delay(500);
            timeout++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            wifiConnected = true;
            terminalPrint("Connected! IP: " + WiFi.localIP().toString());
            drawStatusBar();
        } else {
            terminalPrint("Connection failed!");
        }
    } else if (sub == "disconnect") {
        WiFi.disconnect();
        wifiConnected = false;
        terminalPrint("WiFi disconnected.");
        drawStatusBar();
    } else if (sub == "status") {
        cmd_ifconfig(args);
    } else if (sub == "scan") {
        cmd_scan(args);
    } else {
        terminalPrint("wifi: unknown command: " + sub);
    }
}

void LinuxShell::cmd_ping(const std::vector<String>& args) {
    if (args.size() < 2) {
        terminalPrint("ping: missing host argument");
        return;
    }

    if (WiFi.status() != WL_CONNECTED) {
        terminalPrint("ping: network unreachable (WiFi down)");
        return;
    }

    String host = args[1];
    terminalPrint("PING " + host + " (port 80 check):");

    WiFiClient client;
    for (int i = 1; i <= 3; ++i) {
        unsigned long start = millis();
        bool ok = client.connect(host.c_str(), 80, 2000);
        unsigned long rtt = millis() - start;
        if (ok) {
            client.stop();
            terminalPrint(String(host) + ": seq=" + String(i) + " time=" + String(rtt) + "ms");
        } else {
            terminalPrint(String(host) + ": seq=" + String(i) + " timeout");
        }
        delay(200);
    }
}

void LinuxShell::cmd_wget(const std::vector<String>& args) {
    if (args.size() < 2) {
        terminalPrint("wget: missing URL argument");
        return;
    }

    if (WiFi.status() != WL_CONNECTED) {
        terminalPrint("wget: no WiFi connection");
        return;
    }

    String url = args[1];
    String outfile = "";

    if (args.size() >= 3) {
        outfile = resolveFilePath(args[2]);
    } else {
        int lastSlash = url.lastIndexOf('/');
        if (lastSlash >= 0 && lastSlash < (int)url.length() - 1) {
            outfile = resolveFilePath(url.substring(lastSlash + 1));
        } else {
            outfile = resolveFilePath("download.bin");
        }
    }

    terminalPrint("Connecting to " + url + "...");
    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        int len = http.getSize();
        terminalPrint("Downloading " + String(len) + " bytes -> " + outfile);

        File f = Storage.openFile(outfile, FILE_WRITE);
        if (!f) {
            terminalPrint("wget: cannot write to " + outfile);
            http.end();
            return;
        }

        WiFiClient* stream = http.getStreamPtr();
        uint8_t buf[512];
        int remaining = len;

        while (http.connected() && (remaining > 0 || len == -1)) {
            size_t size = stream->available();
            if (size) {
                int c = stream->readBytes(buf, ((size > sizeof(buf)) ? sizeof(buf) : size));
                f.write(buf, c);
                if (remaining > 0) remaining -= c;
            }
            delay(1);
        }
        f.close();
        terminalPrint("Download completed: " + outfile);
    } else {
        terminalPrint("wget: HTTP error code " + String(httpCode));
    }
    http.end();
}

void LinuxShell::cmd_curl(const std::vector<String>& args) {
    if (args.size() < 2) {
        terminalPrint("curl: missing URL argument");
        return;
    }

    if (WiFi.status() != WL_CONNECTED) {
        terminalPrint("curl: no WiFi connection");
        return;
    }

    String url = args[1];
    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        int maxLen = 300;
        if ((int)payload.length() > maxLen) {
            payload = payload.substring(0, maxLen) + "\n... [truncated]";
        }
        terminalPrint(payload);
    } else {
        terminalPrint("curl: error " + String(httpCode));
    }
    http.end();
}

bool LinuxShell::executeScript(const String& scriptPath) {
    String path = resolveFilePath(scriptPath);
    File f = Storage.openFile(path, FILE_READ);
    if (!f) {
        terminalPrint("sh: cannot open script: " + scriptPath);
        return false;
    }

    terminalPrint("Executing " + scriptPath + "...");
    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.length() > 0 && !line.startsWith("#")) {
            terminalPrint("> " + line);
            execute(line);
        }
    }
    f.close();
    terminalPrint("Script finished.");
    return true;
}

void LinuxShell::cmd_man(const std::vector<String>& args) {
    if (args.size() < 2) {
        cmd_help();
        return;
    }
    String topic = args[1];
    topic.toLowerCase();

    if (topic == "cd") {
        terminalPrint("MAN: cd [dir] - change directory");
        terminalPrint("Supports relative paths (..), absolute (/), and (~)");
    } else if (topic == "ls") {
        terminalPrint("MAN: ls [-l] [dir] - list directory contents");
    } else if (topic == "wget" || topic == "curl") {
        terminalPrint("MAN: wget <url> [outfile] - download web file/ROM to storage");
    } else if (topic == "wifi") {
        terminalPrint("MAN: wifi connect <ssid> <pass> | scan | status");
    } else {
        terminalPrint("No manual entry for: " + topic);
    }
}

void LinuxShell::cmd_help() {
    terminalPrint("=== POSIX / LINUX SHELL COMMANDS ===");
    terminalPrint("FILESYSTEM:");
    terminalPrint("  cd, pwd, ls [-l], cat, touch, mkdir, rm, cp, mv, grep, head, df");
    terminalPrint("SYSTEM:");
    terminalPrint("  uname [-a], uptime, free, ps, whoami, date, reboot, clear");
    terminalPrint("NETWORK:");
    terminalPrint("  ifconfig, scan, wifi connect, ping, wget, curl");
    terminalPrint("SHELL:");
    terminalPrint("  echo text > file, sh script.sh, man <cmd>");
    terminalPrint("EMULATORS & GAMES:");
    terminalPrint("  emu, nes, gb, chip8, dino, gameoflive");
}


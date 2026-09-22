#ifndef LINUX_SHELL_H
#define LINUX_SHELL_H

#include <Arduino.h>
#include <vector>

// =============================================================================
// LinuxShell: Native Linux / POSIX Shell Engine für STERN OS
// Bietet Standard-Unix-Befehle (cd, ls, cat, touch, rm, cp, mv, grep, uname,
// free, ps, df, ifconfig, scan, ping, wget, sh) und Output-Redirection (>).
// =============================================================================

class LinuxShell {
public:
    LinuxShell();

    void begin();

    // Führt einen Shell-Befehl aus
    bool execute(const String& commandLine);

    // Aktuelles Arbeitsverzeichnis & Prompt
    String getCurrentDirectory() const { return _currentDir; }
    String getPrompt() const;

    // Führt ein Shell-Skript (.sh) Zeile für Zeile aus
    bool executeScript(const String& scriptPath);

private:
    String _currentDir;
    String _hostname;
    String _username;

    // Befehls-Handler
    void cmd_pwd(const std::vector<String>& args);
    void cmd_cd(const std::vector<String>& args);
    void cmd_ls(const std::vector<String>& args);
    void cmd_cat(const std::vector<String>& args);
    void cmd_touch(const std::vector<String>& args);
    void cmd_mkdir(const std::vector<String>& args);
    void cmd_rm(const std::vector<String>& args);
    void cmd_rmdir(const std::vector<String>& args);
    void cmd_cp(const std::vector<String>& args);
    void cmd_mv(const std::vector<String>& args);
    void cmd_grep(const std::vector<String>& args);
    void cmd_head(const std::vector<String>& args);
    void cmd_df(const std::vector<String>& args);
    void cmd_echo(const std::vector<String>& args, const String& redirectFile = "", bool append = false);

    void cmd_uname(const std::vector<String>& args);
    void cmd_uptime(const std::vector<String>& args);
    void cmd_free(const std::vector<String>& args);
    void cmd_ps(const std::vector<String>& args);
    void cmd_whoami(const std::vector<String>& args);
    void cmd_date(const std::vector<String>& args);
    void cmd_reboot();

    void cmd_ifconfig(const std::vector<String>& args);
    void cmd_scan(const std::vector<String>& args);
    void cmd_wifi(const std::vector<String>& args);
    void cmd_ping(const std::vector<String>& args);
    void cmd_wget(const std::vector<String>& args);
    void cmd_curl(const std::vector<String>& args);

    void cmd_help();
    void cmd_man(const std::vector<String>& args);

    // Helper
    String resolveFilePath(const String& path);
    std::vector<String> tokenize(const String& line);
};

extern LinuxShell Shell;

#endif // LINUX_SHELL_H


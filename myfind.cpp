// myfind: one fork per filename, -R recursive, -i ignore case
// Output: "<pid>: <filename>: <absolute-path>"

#include <filesystem>
#include <vector>
#include <string>
#include <cstdio>
#include <cctype>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/file.h>

namespace fs = std::filesystem;

// show how to use the program
static void usage(const char* progname) {
    std::fprintf(stderr,
        "Usage: %s [-R] [-i] searchpath file1 [file2] ...\n"
        "  -R  search recursive\n"
        "  -i  ignore case\n", progname);
}

// compare two names
static bool namesMatch(std::string s1, std::string s2, bool ignore) {
    if (!ignore) return s1 == s2;
    if (s1.size() != s2.size()) return false;
    for (size_t i = 0; i < s1.size(); i++) {
        if (std::tolower((unsigned char)s1[i]) != std::tolower((unsigned char)s2[i]))
            return false;
    }
    return true;
}

// print result line (safe with flock)
static void printResult(pid_t p, std::string wanted, fs::path found) {
    std::string absPath = fs::absolute(found).string();
    flock(STDOUT_FILENO, LOCK_EX);
    dprintf(STDOUT_FILENO, "%d: %s: %s\n", (int)p, wanted.c_str(), absPath.c_str());
    flock(STDOUT_FILENO, LOCK_UN);
}

// child search
static void doSearch(std::string start, std::string wanted, bool rec, bool ignore) {
    try {
        if (!fs::exists(start) || !fs::is_directory(start)) _exit(0);

        if (rec) {
            for (auto &entry : fs::recursive_directory_iterator(
                     start, fs::directory_options::skip_permission_denied)) {
                if (!entry.is_regular_file()) continue;
                if (namesMatch(entry.path().filename().string(), wanted, ignore)) {
                    printResult(getpid(), wanted, entry.path());
                }
            }
        } else {
            for (auto &entry : fs::directory_iterator(
                     start, fs::directory_options::skip_permission_denied)) {
                if (!entry.is_regular_file()) continue;
                if (namesMatch(entry.path().filename().string(), wanted, ignore)) {
                    printResult(getpid(), wanted, entry.path());
                }
            }
        }
    } catch (...) {
        // ignore errors in child
    }
    _exit(0);
}

int main(int argc, char* argv[]) {
    bool rec = false;
    bool ignore = false;

    int c;
    while ((c = getopt(argc, argv, "Ri")) != -1) {
        if (c == 'R') rec = true;
        else if (c == 'i') ignore = true;
        else { usage(argv[0]); return 1; }
    }

    if (argc - optind < 2) { usage(argv[0]); return 1; }

    std::string searchDir = argv[optind++];
    std::vector<std::string> fileNames;
    while (optind < argc) {
        fileNames.push_back(argv[optind++]);
    }

    try {
        if (!fs::exists(searchDir) || !fs::is_directory(searchDir)) {
            std::fprintf(stderr, "Error: '%s' is not a directory.\n", searchDir.c_str());
            return 1;
        }
    } catch (...) {
        std::fprintf(stderr, "Error while checking '%s'.\n", searchDir.c_str());
        return 1;
    }

    std::vector<pid_t> kids;
    for (auto &fn : fileNames) {
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); break; }
        if (pid == 0) {
            doSearch(searchDir, fn, rec, ignore);
        } else {
            kids.push_back(pid);
        }
    }

    for (pid_t p : kids) {
        int st;
        waitpid(p, &st, 0);
    }

    return 0;
}

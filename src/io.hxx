#pragma once

#include <string>
#include <cstring>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <map>
#include <regex>
#include <algorithm>
#include <iterator>
#include <dirent.h>
#include <fcntl.h>

#include <curl/curl.h>

#include <archive.h>
#include <archive_entry.h>

#include "pkg.hxx"

using namespace std;
using namespace filesystem;

#define DEFAULT_CONFIG "/etc/nya.conf"

#define print(x) cout << x << endl;
#define strpos(x, y) x.find(y) != string::npos

#define contains(x, y) count(x.begin(), x.end(), y)

#define makedir(x) mkdir(x.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)
#define changedir(x) chdir(x.c_str())
#define perms(x) permissions(x, perms::owner_all | perms::group_all, perm_options::add);

bool init(string cfg);
void mvfile(string file, string to);
void rmfile(string file);
bool is_added(string name);
void add_package(string name);
vector<string> read_section(vector<string> data, string section, bool checks = true);
vector<string> read_file(string filename, bool checks = false);
bool file_exists(string path);
bool dir_exists(string path);
string get_pkg_file(string name);
string read_variable(vector<string> data, string variable);
map<string, string> read_variables(vector<string> data);
string get_value(map<string, string> data, string var);
string get_val(string var);
bool is_yes(string var);
bool is_no(string var);
map<string, string> *get_config_data();
bool package_exists(string name);
string erase(string mainStr, string toErase);
string get_dl();
string get_build();
string get_built();
string get_stuff();
string get_rootfs();
string get_db();
string get_cwd();
bool is_archive(string filename);
vector<string> get_contents(string path);
void create_archive(string archivename, string dest, vector<string> files);
bool extract_archive(string filename, string dest);
int copy_data(struct archive *ar, struct archive *aw);

string readsymlink(string path);

void maindir();

vector<string> get_options(string option);
string get_choose(string option);

bool action(string name, bool emerge = true);

bool add_placeholder(string placeholder, string value);
bool placeholder_exists(string placeholder);
string apply_placeholders(string line);

string strip_extension(string filename);

CURL *get_curl();

vector<string> get_pkg_names();
Package* get_pkg(string name);

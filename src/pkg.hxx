#pragma once

#include <string>
#include <fstream>
#include <vector>
#include <map>

using namespace std;

class Package {
  public:
    Package(string name);

    string getName();

    string val(string var);
    vector<string> sect(string sect, bool checks = true);

    bool read(string file);

    bool get_sources(bool silent = true);
    void get_archives();
    bool extract_archives();
    vector<string> get_depends();
    string get_dl_path();
    string get_build_path();
    string get_files_path();
    string get_db_path();

    string placeholders_var(string line);
    vector<string> placeholders_sect(vector<string> lines);
    string placeholders(string line);

    bool create_script();
    bool package();

    bool build(bool silent = true);
    bool install();
    bool remove();
  private:
    string name, desc, ver;
    string display_name;
    string fullname;

    string dest;
    string files;

    map<string, string> variables, sources, archives;
    map<string, string>::iterator itr;

    vector<string> data, deps;
};

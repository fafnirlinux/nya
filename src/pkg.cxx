#include "pkg.hxx"
#include "io.hxx"

#include <string.h>
#include <filesystem>

#define msg(x) print(name + ": " + x)
#define err(x) msg(x); return false

Package::Package(string n) {
	name = n;
}

string Package::getName() {
	return name;
}

string Package::val(string var) {
	return placeholders_var(get_value(variables, var));
}

vector<string> Package::sect(string sect, bool checks) {
	return read_section(data, sect, checks);
}

bool Package::_is_yes(string var) {
	return val(var) == "yes";
}

bool Package::_is_no(string var) {
	return val(var) == "no";
}

bool Package::read(string file) {
	if (is_read) return true;
	if (!file_exists(file)) return false;

	data = read_file(file);
	variables = read_variables(data);

	display_name = val("name");
    if (display_name.empty())
        display_name = name;
    desc = val("desc");
	ver = val("ver");

	deps = placeholders_sect(sect("deps"));

	fullname = name + " v" + ver;

	is_read = true;

	return true;
}

string Package::get_dl_path() {
	return get_dl() + "/" + name;
}

string Package::get_build_path() {
	return get_build() + "/" + name;
}

string Package::get_files_path() {
	return get_stuff() + "/" + name;
}

string Package::get_db_path() {
	return get_db() + "/" + name;
}

vector<string> Package::get_depends() {
	vector<string> result;

	for (auto dep: deps) {
		if (!contains(result, dep))
			result.push_back(dep);
	}

	return result;
}

bool Package::check_hostdeps() {
	vector<string> hostdeps = sect("hostdeps");

	if (!hostdeps.empty()) {
		for (auto hostdep: hostdeps) {
			if (!system(string("which " + hostdep + " > /dev/null 2>&1").c_str())) {
				err("missing " + hostdep);
			}
		}
	}

	return true;
}

bool Package::get_sources(bool silent) {
    for (auto src: placeholders_sect(sect("srcs"))) {
        string filename;

        if (strpos(src, "::")) {
            filename = src.substr(src.find("::") + 2);
            src = erase(src, "::" + filename);
        } else {
            filename = basename(src.c_str());
        }

        if (is_archive(src)) {
            archives.insert({basename(src.c_str()), strip_extension(filename)});
		}

        sources.insert({src, filename});
    }

    char buff[100];
    int i = 1;

    for (itr = sources.begin(); itr != sources.end(); ++itr) {
        string source = itr->first;
        string filename, target;

        bool exists;

        bool git = !is_archive(source) && strpos(source, "git");

        if (!git) {
            filename = basename(source.c_str());
            target = get_dl_path() + "/" + filename;
            exists = file_exists(target);
        } else {
            filename = itr->second;
            target = get_build_path() + "/" + filename;
            exists = dir_exists(target);
        }

        snprintf(buff, sizeof(buff), " [%i/%i]", i, sources.size());

		if (exists) {
            msg(filename + " already exists" + string(buff));
            ++i;
			continue;
        }

		msg((!git ? "downloading " : "cloning ") + filename + string(buff));

        if (!git) {
            makedir(get_dl_path());

            CURL *curl = get_curl();

            curl_easy_setopt(curl, CURLOPT_URL, source.c_str());

		    FILE *file = fopen(target.c_str(), "w");
		    if (file) {
			    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);

			    if (!silent || !is_yes("silent"))
			    	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

			    curl_easy_perform(curl);

			    fclose(file);
		    }

		    if (!file_exists(target) || file_size(target) == 0) {
			    err("couldn't get " + filename);
                rmfile(target);
		    }
        } else {
            changedir(get_build_path());
            if (silent && !is_no("silent"))
            	system(string("git clone " + source + " &>/dev/null").c_str());
            else
            	system(string("git clone " + source).c_str());
            maindir();

            if (!dir_exists(target)) {
                err("couldn't clone " + filename);
            }
        }

        ++i;
	}

	return true;
}

bool Package::extract_archives() {
    char buff[100];
    int i = 1;

    for (itr = archives.begin(); itr != archives.end(); ++itr) {
        snprintf(buff, sizeof(buff), " [%i/%i]", i, archives.size());

		msg("extracting " + itr->first + string(buff));
		extract_archive(get_dl_path() + "/" + itr->first, get_build_path());

		++i;
	}

	for (itr = archives.begin(); itr != archives.end(); ++itr) {
		if (!dir_exists(get_build_path() + "/" + itr->second)) {
			err("failed to extract " + itr->first);
		}
	}

	return true;
}

#define replace(x, y) line = regex_replace(line, regex(x), y)

string Package::placeholders_var(string line) {
  	replace("%name", name);

  	for (itr = variables.begin(); itr != variables.end(); ++itr) {
		replace("%" + itr->first, itr->second);
	}

    string str = line.substr(line.find("%") + 1);
    str = str.substr(0, str.find("%"));

    if (strpos(str, ":")) {
        string var = str.substr(str.find(":") + 1);
        string package = erase(str, ":" + var);

        Package *pkg = get_pkg(package);

        if (pkg != NULL && pkg->read(get_pkg_file(package))) {
            string val = pkg->val(var);

            if (!val.empty()) {
                replace("%" + package + ":" + var + "%", val);
            }
        }
    }

    return line;
}

vector<string> Package::placeholders_sect(vector<string> lines, bool is_build) {
    vector<string> result;

    for (auto line: lines) {
        line = placeholders(line);

        if (line.at(0) == '&') {
            line = line.erase(0, 1);

            if (strpos(line, ":")) {
                string sect = line.substr(line.find(":") + 1);
                string package = erase(line, ":" + sect);

                Package *pkg = get_pkg(package);

                if (pkg != NULL && pkg->read(get_pkg_file(package))) {
                	if (is_build) {
                		vector<string> list = get_contents(pkg->get_files_path() + "/patches");

	    				for (auto patch: list) {
		    				string filename(basename(patch.c_str()));

		    				if (strpos(filename, ".patch") || strpos(filename, ".diff")) {
                				result.push_back("apply_patch " + patch);
            				}
	    				}
	    			}

                    for (auto line: pkg->sect(sect)) {
                        result.push_back(placeholders(line));
                    }
                }
            } else {
            	string choose = get_choose(line.erase(0, 1));

            	if (!choose.empty()) {
            		result.push_back(choose);
            	}
            }
        } else {
            result.push_back(line);
        }
    }

    return result;
}

string Package::placeholders(string line) {
    replace("%dest", dest);

  	replace("%conf", "./configure --prefix=%prefix $xconfflags");
	replace("%make", "make -j%threads");
	replace("%inst", "inst");

	replace("%dl", get_dl_path());
	replace("%src", get_build_path());
	replace("%files", get_files_path());

    return apply_placeholders(placeholders_var(line));
}

#define line(x) script << x << endl

bool Package::create_script() {
	vector<string> lines = sect("build");

	if (lines.empty() && sources.size() != 1) {
		return false;
	}

	rmfile(get_build_path() + "/build.sh");
	ofstream script(get_build_path() + "/build.sh");

  	line("#!/bin/sh");
    line("source common.sh");

  	if (sources.size() == 1)
  		line("cd " + strip_extension(sources.begin()->second));
  	else if (!val("workdir").empty())
  		line("cd " + val("workdir"));

    if (archives.size() == 1 || _is_yes("force-patch")) {
        vector<string> list = get_contents(get_files_path() + "/patches");

	    for (auto patch: list) {
		    string filename(basename(patch.c_str()));

		    if (strpos(filename, ".patch") || strpos(filename, ".diff")) {
                line("apply_patch " + patch);
            }
	    }
    }

    for (auto line: placeholders_sect(lines, true)) {
        line(line);
    }

  	script.close();

    rmfile(get_build_path() + "/common.sh");
    ofstream common(get_build_path() + "/common.sh");
	vector<string> file = read_file(get_stuff() + "/common.sh");

	for (auto line: file) {
		common << placeholders(line) << endl;
	}

	common.close();

  	return true;
}

#define add_line(x) pkgdata << x << endl

bool Package::package() {
	vector<string> list = get_contents(dest);

    if (list.size() == 0) {
        err("package is not built");
    }

	ofstream filelist(dest + "/files");

	for (auto file: list) {
		if (string(basename(file.c_str())).compare("files") == 0) continue;
		filelist << erase(file, dest) << endl;
	}

	filelist.close();

    ofstream pkgdata(dest + "/pkgdata");

    if (!display_name.empty())
	    add_line("name=" + display_name);
    if (!desc.empty())
        add_line("desc=" + desc);
    if (!ver.empty())
	    add_line("ver=" + ver);

	vector<string> depends = get_depends();

	if (!depends.empty()) {
		add_line("[deps]");

		for (auto dep: depends) {
			add_line(dep);
		}
	}

	pkgdata.close();

	vector<string> install = sect("install");

	if (!install.empty()) {
		ofstream install_script(dest + "/install.sh");

		install_script << "#!/bin/sh" << endl;

		for (auto line: placeholders_sect(install)) {
			install_script << line << endl;
		}

		install_script.close();
	}

	vector<string> uninstall = sect("remove");

	if (!uninstall.empty()) {
		ofstream uninstall_script(dest + "/uninstall.sh");

		uninstall_script << "#!/bin/sh" << endl;

		for (auto line: placeholders_sect(uninstall)) {
			uninstall_script << line << endl;
		}

		uninstall_script.close();
	}

	create_archive(get_built() + "/" + name + ".cpio.gz", dest, get_contents(dest));

	return true;
}

bool Package::build(bool silent) {
    if (!is_yes("no-package") && file_exists(get_built() + "/" + name + ".cpio.gz")) {
		msg("already built");
		return true;
	}

	string pkgfile = get_pkg_file(name);

	if (!check_hostdeps()) return false;

	if (dir_exists(get_build_path()))
		system(string("rm -rf " + get_build_path()).c_str());

	makedir(get_build_path());

	if (!get_sources(silent)) { err("couldn't get sources"); }
	if (!extract_archives()) { err("couldn't extract archives"); }

	bool do_package = !is_yes("no-package") && !sources.empty();

	if (do_package) {
		dest = get_build_path() + "/" + name + "-dest";
  		makedir(dest);
  	} else {
  		dest = get_rootfs();
  	}

	if (!create_script()) {
		if (read_section(read_file(pkgfile), "deps").empty()) {
			err("empty package");
		}

		for (auto dep: get_depends()) {
			if (!action(dep)) return false;
		}

		return true;
	}

	msg("building");

	changedir(get_build_path());
    if (silent && !is_no("silent"))
        system("bash build.sh &>/dev/null");
    else
        system("bash build.sh");
    maindir();

	if (do_package && !package()) return false;

	return true;
}

bool Package::install() {
	if (file_exists(get_db_path() + "/pkgdata")) {
		msg("already installed");
		return true;
	}

	string archive(get_built() + "/" + name + ".cpio.gz");

	if (!file_exists(archive)) {
		print("no built package");
		return false;
	}

	extract_archive(archive, get_rootfs());

	makedir(get_db_path());

	mvfile(get_rootfs() + "/pkgdata", get_db_path() + "/pkgdata");
	mvfile(get_rootfs() + "/files", get_db_path() + "/files");

    mvfile(get_rootfs() + "/uninstall.sh", get_db_path() + "/uninstall.sh");

	if (file_exists(get_rootfs() + "/install.sh")) {
		changedir(get_rootfs());
        perms("install.sh");
		system("bash install.sh");
		rmfile("install.sh");
		maindir();
	}

	msg("installed");

	return true;
}

bool Package::remove() {
	if (!file_exists(get_db_path() + "/pkgdata")) {
		err("not installed");
	}

	if (!read(get_db_path() + "/pkgdata")) {
		err("not installed correctly");
	}

	rmfile(get_db_path() + "/pkgdata");

	ofstream remove_script(get_rootfs() + "/remove_files.sh");

	vector<string> file = read_file(get_stuff() + "/remove_files.sh");

	for (auto line: file) {
		replace("%list", get_db_path() + "/files");
		replace("%rootfs", get_rootfs());

		remove_script << line << endl;
	}

	remove_script.close();

    changedir(get_rootfs());

	perms("remove_files.sh");
	system("bash remove_files.sh");
	rmfile("remove_files.sh");

	if (file_exists("uninstall.sh")) {
        perms("uninstall.sh");
		system("bash uninstall.sh");
		rmfile("uninstall.sh");
	}

	maindir();

	system(string("rm -rf " + get_db_path()).c_str());

	msg("removed");

	return true;
}

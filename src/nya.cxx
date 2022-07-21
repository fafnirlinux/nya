#include <memory>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>

#include <stdio.h>

#include <string.h>

#include "io.hxx"
#include "pkg.hxx"

void usage(char *arg0) {
	printf("%s", arg0);
	printf(" emerge|build|install|remove <pkg>\n");
}

vector<string> dependencies;
vector<string> marked;

void dependency(string pkgname) {
	Package* pkg = get_pkg(pkgname);
	if(pkg == NULL) return;
	pkg->read(get_pkg_file(pkgname));

	vector<string> deps = pkg->get_depends();

	for (auto dep: deps) {
		if (!count(marked.begin(), marked.end(), dep)) {
			marked.push_back(dep);
			dependency(dep);
		}
	}

	if (!count(dependencies.begin(), dependencies.end(), pkgname))
		dependencies.push_back(pkgname);
}

void emerge(vector<string> pkgs) {
	if (is_yes("no-deps")) {
		for (auto pkgname: pkgs) {
			Package* pkg = get_pkg(pkgname);

			if (pkg->build(false)) {
            	if (!pkg->install()) {
                	break;
            	}
            }
		}

		return;
	}

	for (auto pkgname: pkgs) {
		dependency(pkgname);

		bool error;

		for (auto dep: dependencies) {
			Package* pkg = get_pkg(dep);

			if (pkg->build(false)) {
            	if (!pkg->install()) {
            		error = true;
                	break;
            	}
			} else {
				error = true;
				break;
			}
		}

		if (error) break;
	}
}

void build(vector<string> pkgs) {
	if (is_yes("no-deps")) {
		for (auto pkgname: pkgs) {
			Package* pkg = get_pkg(pkgname);

			if (!pkg->build(false)) {
                break;
            }
		}

		return;
	}

	for (auto pkgname: pkgs) {
		dependency(pkgname);

		bool error;

		for (auto dep: dependencies) {
			Package* pkg = get_pkg(dep);

			if (!pkg->build(false)) {
            	error = true;
                break;
            }
		}

		if (error) break;
	}
}

int main(int argc, char *argv[]) {
	--argc;
	if (argc < 1) { usage(argv[0]); return 1; }
	vector<string> args(argv + 1, argv + (argc + 1));
	string action;
	vector<string> others;
	for (auto str: args) {
		if (action.empty()) { action = str; continue; }
		others.push_back(str);
	}
	if (others.empty()) { usage(argv[0]); return 1; }
	string config;
	vector<string> pkgs;
	int i;
	for (auto str: others) {
		if (str == "--config") {
			if (i+1 < others.size())
				config = others[i+1];
		} else if (str != config) {
			pkgs.push_back(str);
		}
		++i;
	}

	if (!init(config))
		return 1;

	if (action == "emerge") {
		emerge(pkgs);
	} else if (action == "build") {
		build(pkgs);
	} else if (action == "install") {
		for (auto pkg: pkgs) {
			if (!get_pkg(pkg)->install()) break;
		}
	} else if (action == "remove") {
		for (auto pkg: pkgs) {
			if (!get_pkg(pkg)->remove()) break;
		}
	} else {
		print("invalid action");
		return 1;
	}

	curl_global_cleanup();

	return 0;
}

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
	Package *pkg = get_pkg(pkgname);
	if (pkg == NULL) return;
	if (!pkg->read(get_pkg_file(pkgname))) return;

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

int emerge(vector<string> pkgs) {
	if (is_yes("no-package")) {
		return 1;
	}

	int ret = 0;

	if (is_yes("no-deps")) {
		for (auto pkgname: pkgs) {
			if (!action(pkgname)) {
				ret = 1;
				break;
			}
		}

		return ret;
	}

	for (auto pkgname: pkgs) {
		dependency(pkgname);

		for (auto dep: dependencies) {
			if (!action(dep)) {
				ret = 1;
                break;
			}
		}

		if (ret == 1) break;
	}

	return ret;
}

int build(vector<string> pkgs) {
	int ret = 0;

	if (is_yes("no-deps")) {
		for (auto pkgname: pkgs) {
			if (!action(pkgname, false)) {
				ret = 1;
				break;
			}
		}

		return ret;
	}

	for (auto pkgname: pkgs) {
		dependency(pkgname);

		for (auto dep: dependencies) {
			if (!action(dep, false)) {
            	ret = 1;
                break;
            }
		}

		if (ret == 1) break;
	}

	return ret;
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

	if (getuid() && !is_yes("no-root")) { print("run as root"); return 1; }

	int ret = 0;

	if (action == "emerge") {
		ret = emerge(pkgs);
	} else if (action == "build") {
		ret = build(pkgs);
	} else if (action == "install") {
		if (is_yes("no-package")) {
			return 1;
		}

		for (auto pkg: pkgs) {
			if (!get_pkg(pkg)->install()) {
				ret = 1;
				break;
			}
		}
	} else if (action == "remove") {
		for (auto pkg: pkgs) {
			if (!get_pkg(pkg)->remove()) {
				ret = 1;
				break;
			}
		}
	} else {
		print("invalid action");
		ret = 1;
	}

	curl_global_cleanup();

	return ret;
}

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
	pkg->read(get_pkg_file(pkgname));
	if(pkg == NULL) return;
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

int main(int argc, char *argv[]) {
	if (argc < 2 || !argv[1] || !argv[2]) {
		usage(argv[0]);
		return 1;
	}

	if (!init())
		return 1;

	string action(argv[1]);
	string pkgname(argv[2]);

	if (strpos(action, "emerge")) {
		dependency(pkgname);

		for(auto dep: dependencies){
			Package* pkg = get_pkg(dep);
			if (pkg->build(false)) {
                if (!pkg->install()) {
                    break;
                }
            }
		}
	} else if (strpos(action, "build")) {
		dependency(pkgname);

		for(auto dep: dependencies){
			Package* pkg = get_pkg(dep);
			pkg->build(false);
		}
	} else if (strpos(action, "install")) {
		Package* pkg = get_pkg(pkgname);
		pkg->install();
	} else if (strpos(action, "remove")) {
		Package* pkg = get_pkg(pkgname);
		pkg->remove();
	} else {
		print("invalid action");
		return 1;
	}
	curl_global_cleanup();
	return 0;
}

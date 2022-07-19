#include "io.hxx"

string cwd, rootfs, db;

string src, build, dl, built;
string pkgdir, stuff;

CURL *curl;
CURLcode res;

map<string, Package> pool;
map<string, Package>::iterator itr;

vector<string> config, archive_extensions;

map<string, string> config_data, placeholders;
map<string, string>::iterator _itr;

bool add_placeholder(string placeholder, string value) {
	if (placeholder_exists(placeholder))
		return false;

	placeholders.insert({placeholder, value});

	return true;
}

bool placeholder_exists(string placeholder) {
	for (_itr = placeholders.begin(); _itr != placeholders.end(); ++_itr) {
		if (placeholder == _itr->first) {
			return true;
		}
	}

	return false;
}

vector<string> get_pkg_names() {
	vector<string> names;

	for (itr = pool.begin(); itr != pool.end(); ++itr) {
		names.push_back(itr->first);
	}

	return names;
}

void add_package(string name) {
	if (is_added(name))
		return;

	Package pkg(name);

	pool.insert({name, pkg});
}

Package* get_pkg(string name) {
	if (!is_added(name))
		add_package(name);

	for (itr = pool.begin(); itr != pool.end(); ++itr) {
		if (name == itr->first) {
			return &itr->second;
		}
	}

	return NULL;
}

string erase(string mainStr, string toErase) {
    string str = mainStr;
    size_t pos = string::npos;
    while ((pos = str.find(toErase) )!= string::npos) {
        str.erase(pos, toErase.length());
    }
    return str;
}

#define conf(x) regex_replace(get_val(x), regex("%pwd"), cwd)
#define add_ext(x) archive_extensions.push_back(x)

bool init(string cfg){
	char cwdd[50];

	if (getcwd(cwdd, sizeof(cwdd)) == NULL) {
		perror("getcwd() error");
		return false;
	}

	cwd = string(cwdd);

	if (cfg.empty()) cfg = cwd + "/config";

	curl_global_init(CURL_GLOBAL_ALL);

	curl = curl_easy_init();
	if (!curl)
		return false;

    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    if (file_exists(DEFAULT_CONFIG)) {
    	config = read_file(DEFAULT_CONFIG, true);
    } else if (file_exists(cfg)) {
    	config = read_file(cfg, true);
    } else {
    	return false;
   	}

   	config_data = read_variables(config);

   	map<string, string> data;

	for (_itr = config_data.begin(); _itr != config_data.end(); ++_itr) {
		string var = _itr->first;
		string val = _itr->second;

		for (_itr = config_data.begin(); _itr != config_data.end(); ++_itr) {
			val = regex_replace(val, regex("%" + _itr->first), _itr->second);
		}

		val = regex_replace(val, regex("%pwd"), get_cwd());

		data.insert({var, val});
	}

	config_data = data;

   	if (!conf("rootfs").empty()) rootfs = conf("rootfs"); else rootfs = "/";

	if (!conf("src").empty()) src = conf("src"); else src = rootfs + "/src";

	if (!conf("db").empty()) db = conf("db"); else db = src + "/db";
	if (!conf("build_dir").empty()) build = conf("build_dir"); else build = src + "/build";
	if (!conf("dl_dir").empty()) dl = conf("dl_dir"); else dl = src + "/dl";
	if (!conf("built_dir").empty()) built = conf("built_dir"); else built = src + "/built";
	if (!conf("pkgdir").empty()) pkgdir = conf("pkgdir"); else pkgdir = src + "/pkg";
	if (!conf("stuff_dir").empty()) stuff = conf("stuff_dir"); else stuff = src + "/stuff";

	makedir(rootfs);
	makedir(src);

	makedir(build);
	makedir(dl);
	makedir(pkgdir);
	makedir(stuff);

	if (!is_yes("no-package")) {
		makedir(db);
		makedir(built);
	}

	add_ext("tar.gz");
	add_ext("tar.xz");
    add_ext("tar.bz2");

	string cpu_count = to_string(sysconf(_SC_NPROCESSORS_ONLN));

	string prefix;

	add_placeholder("%rootfs", rootfs);
	add_placeholder("%prefix", prefix);
	add_placeholder("%threads", cpu_count);
    add_placeholder("%arch", "x86_64");

	add_placeholder("%conf", "./configure --prefix=" + prefix);
	add_placeholder("%make", "make -j" + cpu_count);

	add_placeholder("%stuff", stuff);

	maindir();

	return true;
}

string get_dl() {
	return dl;
}

string get_build() {
	return build;
}

string get_built() {
	return built;
}

string get_stuff() {
	return stuff;
}

string get_rootfs() {
	return rootfs;
}

string get_db() {
	return db;
}

CURL *get_curl() {
	return curl;
}

void mvfile(string file, string to) {
	if (file_exists(file)) fs::rename(file, to);
}

void rmfile(string file) {
	if (file_exists(file)) fs::remove(file);
}

bool is_added(string name) {
	for (itr = pool.begin(); itr != pool.end(); ++itr) {
		if (name == itr->first) {
			return true;
		}
	}
	return false;
}

vector<string> read_section(vector<string> data, string section, bool checks) {
	bool in_section;

	vector<string> output;

	for (auto line: data) {
        if (line.empty()) {
            if (!checks) output.push_back("");
            continue;
        }
        if (checks && in_section && line.at(0) == '#') continue;
		if (in_section && line.at(0) == '[' && line.at(line.size()-1) == ']') break;
		if (line.compare("["+section+"]") == 0) {
			in_section = true;
		} else if (in_section) {
			output.push_back(line);
		}
	}

	return output;
}

vector<string> read_file(string filename, bool checks) {
	string line;

	ifstream file(filename);

	vector<string> result;

	while (getline(file, line)) {
        if (line.empty()) {
            if (!checks) result.push_back("");
            continue;
        }
        if (checks && line.at(0) == '#') continue;
		result.push_back(line);
	}

	file.close();

	return result;
}

bool is_archive(string filename) {
	for (auto ext: archive_extensions) {
		if (strpos(filename, ext)) {
			return true;
		}
	}

	return false;
}

vector<string> get_contents(string path) {
	vector<string> list;

    if (file_exists(path)) {
	    for (const auto &file: fs::recursive_directory_iterator(path))
		    list.push_back(file.path());
    }

	return list;
}

string readsymlink(string path) {
	char buf[1024];
	ssize_t len;
	if ((len = readlink(path.c_str(), buf, sizeof(buf)-1)) != -1) {
		buf[len] = '\0';
		return string(buf);
	}
	return "";
}

void create_archive(string archivename, string dest, vector<string> files) {
	struct archive *a;
	struct archive_entry *entry;
	struct stat st;
	char buff[8192];
	int len;
	int fd;

	rmfile(archivename);

	a = archive_write_new();
    archive_write_add_filter_gzip(a);
	archive_write_set_format_cpio(a);
    archive_write_set_format_by_name(a, "newc");
	archive_write_open_filename(a, archivename.c_str());

	for (string file: files) {
		string filename(erase(file, dest));
		lstat(file.c_str(), &st);
		entry = archive_entry_new();
		archive_entry_set_pathname(entry, filename.c_str());
		archive_entry_set_size(entry, st.st_size);

		string type, link;
		switch (st.st_mode & S_IFMT) {
			case S_IFBLK:
				type = "block device";
				archive_entry_set_filetype(entry, AE_IFBLK);
				break;
			case S_IFCHR:
				type = "character device";
				archive_entry_set_filetype(entry, AE_IFCHR);
				break;
			case S_IFDIR:
				type = "directory";
				archive_entry_set_filetype(entry, AE_IFDIR);
				break;
			case S_IFIFO:
				type = "FIFO/pipe";
				archive_entry_set_filetype(entry, AE_IFIFO);
				break;
			case S_IFLNK:
				link = readsymlink(file);
				type = "symlink";
				archive_entry_set_filetype(entry, AE_IFLNK);
				archive_entry_set_symlink(entry, link.c_str());
				break;
			case S_IFREG:
				type = "regular file";
				archive_entry_set_filetype(entry, AE_IFREG);
				break;
			case S_IFSOCK:
				type = "socket";
				archive_entry_set_filetype(entry, AE_IFSOCK);
				break;
			default:
				type = "unknown";
				archive_entry_set_filetype(entry, AE_IFREG);
				break;
		}

		if (strcmp(link.c_str(),"") != 0)
			link = " -> \'" + link + "\'";

		archive_entry_set_perm(entry, st.st_mode & 07777);

		archive_write_header(a, entry);
		if (archive_entry_size(entry) > 0) {
			fd = open(file.c_str(), O_RDONLY);
			len = read(fd, buff, sizeof(buff));
			while ( len > 0 ) {
				archive_write_data(a, buff, len);
				len = read(fd, buff, sizeof(buff));
			}
			close(fd);
		}
		archive_entry_free(entry);
	}

	archive_write_close(a);
	archive_write_free(a);
}

bool extract_archive(string filename, string dest) {
  struct archive *a;
  struct archive *ext;
  struct archive_entry *entry;
  int flags;
  int r;

  flags = ARCHIVE_EXTRACT_TIME;
  flags |= ARCHIVE_EXTRACT_PERM;
  flags |= ARCHIVE_EXTRACT_ACL;
  flags |= ARCHIVE_EXTRACT_FFLAGS;

  a = archive_read_new();
  archive_read_support_format_all(a);
  archive_read_support_filter_all(a);
  ext = archive_write_disk_new();
  archive_write_disk_set_options(ext, flags);
  archive_write_disk_set_standard_lookup(ext);
  if ((r = archive_read_open_filename(a, filename.c_str(), 10240)))
    return false;
  for (;;) {
    r = archive_read_next_header(a, &entry);
    if (r == ARCHIVE_EOF)
      break;
    if (r < ARCHIVE_OK)
      fprintf(stderr, "%s\n", archive_error_string(a));
    if (r < ARCHIVE_WARN)
      return false;
    const char* currentFile = archive_entry_pathname(entry);
    string fullOutputPath = dest + "/" + string(currentFile);
    archive_entry_set_pathname(entry, fullOutputPath.c_str());
    r = archive_write_header(ext, entry);
    if (r < ARCHIVE_OK)
      fprintf(stderr, "%s\n", archive_error_string(ext));
    else if (archive_entry_size(entry) > 0) {
      r = copy_data(a, ext);
      if (r < ARCHIVE_OK)
        fprintf(stderr, "%s\n", archive_error_string(ext));
      if (r < ARCHIVE_WARN)
        return false;
    }
    r = archive_write_finish_entry(ext);
    if (r < ARCHIVE_OK)
      fprintf(stderr, "%s\n", archive_error_string(ext));
    if (r < ARCHIVE_WARN)
      return false;
  }
  archive_read_close(a);
  archive_read_free(a);
  archive_write_close(ext);
  archive_write_free(ext);
  return true;
}

int copy_data(struct archive *ar, struct archive *aw) {
  int r;
  const void *buff;
  size_t size;
  la_int64_t offset;

  for (;;) {
    r = archive_read_data_block(ar, &buff, &size, &offset);
    if (r == ARCHIVE_EOF)
      return (ARCHIVE_OK);
    if (r < ARCHIVE_OK)
      return (r);
    r = archive_write_data_block(aw, buff, size, offset);
    if (r < ARCHIVE_OK) {
      fprintf(stderr, "%s\n", archive_error_string(aw));
      return (r);
    }
  }
}

string strip_extension(string filename) {
    bool erased;

	for (auto ext: archive_extensions) {
        if (strpos(filename, ext)) {
		    filename = erase(filename, ext);
            erased = true;
        }
	}

    if (erased)
	    filename.pop_back();

	return filename;
}

void maindir() {
	changedir(src);
}

string apply_placeholders(string line) {
	string result(line);

	for (_itr = placeholders.begin(); _itr != placeholders.end(); ++_itr) {
		result = regex_replace(result, regex(_itr->first), _itr->second);
	}

	return result;
}

bool file_exists(const string& name) {
	struct stat buffer;
	return !stat(name.c_str(), &buffer);
}

string get_pkg_file(string name) {
    DIR *dir = opendir(pkgdir.c_str());

    struct dirent *entry = readdir(dir);

    string pkgfile;

    while (entry != NULL) {
        if (entry->d_type == DT_DIR) {
            if (file_exists(pkgdir + "/" + string(entry->d_name) + "/" + name)) {
                pkgfile = pkgdir + "/" + string(entry->d_name) + "/" + name;
                break;
            }
        }

        entry = readdir(dir);
    }

    closedir(dir);

	return pkgfile;
}

string read_variable(vector<string> data, string variable) {
	if (variable.empty()) return "";

	for (auto var: data) {
		if (strpos(var, variable + "=")) {
			return erase(var, variable + "=");
		}
	}

	return "";
}

map<string, string> read_variables(vector<string> data) {
	map<string, string> variables;

	for (auto line: data) {
		if (line.empty()) continue;
		if (line.at(0) == '[' && line.at(line.size()-1) == ']') break;
		if (strpos(line, "=")) {
			string val = line.substr(line.find("=") + 1);
            string var = erase(line, "=" + val);

            variables.insert({var, val});
		}
	}

	return variables;
}

string get_value(map<string, string> data, string var) {
	for (_itr = data.begin(); _itr != data.end(); ++_itr) {
		return _itr->second;
	}

	return "";
}

string get_val(string var) {
	return get_value(config_data, var);
}

bool is_yes(string var) {
	return get_val(var) == "yes";
}

bool is_no(string var) {
	return get_val(var) == "no";
}

map<string, string> get_config_data() {
	return config_data;
}

string get_cwd() {
	return cwd;
}

bool package_exists(string name){
	return file_exists(get_pkg_file(name));
}

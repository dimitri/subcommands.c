/*
 * filepaths.h, copyright Dimitri Fontaine <dim@tapoueh.org>
 * License: ISC
 *
 * The lib is meant to be easy to use and very convenient, at the expense of
 * doing too many things by default (calls to access(2) and stat(2) and
 * allocating more memory that you anticipated or would find
 * necessary/tasteful/sensible.
 *
 * It's quite convenient though.
 *
 */

#ifndef __FILEPATHS_H__
#define __FILEPATHS_H__

#include <libgen.h>
#include <fts.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAXPATHLEN 1024			/* TODO: find proper header */

typedef struct
{
	int nb_dirs;				/* we normalize at creation */
	char **directories;			/* walk the tree */
	char *name;					/* name of the file */
	char *extension;			/* extension of the file */
	char *filename;				/* original given filename */
	char *realpath;				/* realpath() of original filename */
	bool exists;				/* does the file exists? */
	struct stat *st;			/* when file exists, its stats */
} Path;

typedef struct
{
	int size;
	Path **list;
} PathList;

Path *filepath_new(const char *filename);
void filepath_refresh_stats(Path *path);
void filepath_free(Path *path);

char *filepath_get_filename(Path *path);
int filepath_sprintf(char *dest, Path *path);
void filepath_fprintf(FILE *stream, Path *path);

Path *filepath_cwd();
Path *filepath_join(Path *path, const char *filename);
Path *filepath_merge(Path *specs, Path *defaults);
Path *filepath_with_extension(Path *path, const char *extension);

const char *filepath_absolute_filename(Path *path);
const char *filepath_relative_filename(Path *path, Path *maybe_root);

bool filepath_is_absolute(Path *path);
bool filepath_is_relative(Path *path);
bool filepath_file_exists(Path *path);
bool filepath_directory_exists(Path *path);
bool filepath_ensure_directories_exist(Path *path, mode_t mode);
bool filepath_remove_directory(Path *path);

PathList *filepath_list_new(const char *list);
void filepath_list_free(PathList *plist);
PathList *filepath_list_find(PathList *path, const char *filename);

static void filepath_normalize_directory(Path *path);


/*
 * Main entry point API, creating a Path object from a filename.
 *
 * In this library we choose to do the hard work at Path creation time, so that
 * other operations are then easy to implement. You're not expected to just
 * instanciate a Path object then never use it, after all, as a C-string would
 * be perfect to use if you don't process it.
 */
Path *
filepath_new(const char *filename)
{
	Path *path = (Path *) malloc(sizeof(Path));
	char *resolved = (char *) malloc(MAXPATHLEN * sizeof(char));
	char *ptr;
	bool is_dir;

	path->filename = (char *)filename;
	path->realpath = realpath(filename, resolved);

	/* we fix that as part of the normalization process, see below */
	path->name = NULL;
	path->extension = NULL;
	path->directories = NULL;

	/* update file stats cache */
	filepath_refresh_stats(path);

	/*
	 * Normalize our filename into a realpath, directories, name and
	 * extension.
	 */
	filepath_normalize_directory(path);

	return path;
}

/*
 * Helper function to normalize given filename into a proper path, part of the
 * initialization routine in filepath_new().
 */
static void
filepath_normalize_directory(Path *path)
{
	char *ptr, *realpath, *previous;
	bool is_dir;
	int i;

	if (path->filename == NULL)
	{
		return;
	}

	if (path->realpath != NULL && path->st != NULL)
	{
		is_dir = S_ISDIR(path->st->st_mode);
	}
	else
	{
		int len = strlen(path->filename);

		is_dir = len >= 2 && path->filename[len-2] == '/';
	}

	/*
	 * Now, normalize the directory path.
	 */
	path->nb_dirs = 0;

	/*
	 * When the path itself is a directory, append the last part to our
	 * directories list.
	 */
	if (is_dir)
	{
		int bytes = strlen(path->realpath) + 2;
		realpath = (char *) malloc(bytes * sizeof(char));

		sprintf(realpath, "%s/", path->realpath);
	}
	else if (path->realpath != NULL)
	{
		realpath = strdup(path->realpath);
	}
	else
	{
		/*
		 * The file doesn't exists, so it's not been normalized by calling into
		 * realpath(3). We don't need to solve the ../.. and such in the
		 * filename, as after all it points to a non-existing location and we
		 * know that already, but double-slashes are going to be a problem
		 * going forward(-slash).
		 */
		int f_i, r_i;
		int len = strlen(path->filename);
		char previous = '\0';

		realpath = (char *) malloc((len+1) * sizeof(char));
		realpath[len] = '\0';

		for (f_i = 0, r_i = 0; f_i < len; previous = path->filename[f_i++])
		{
			char current = path->filename[f_i];

			if (current == '/' && previous == current)
			{
				/* just skip it */
				continue;
			}
			else
			{
				/* we keep it */
				realpath[r_i++] = current;
			}
		}
		path->realpath = strdup(realpath);
	}

	/* we compute nb_dirs on the non-modified realpath */
	if (path->realpath != NULL)
	{
		for (ptr = path->realpath; *ptr != '\0'; ptr++)
		{
			if (*ptr == '/')
			{
				path->nb_dirs++;
			}
		}
		path->nb_dirs += is_dir ? 1 :0;

		if (path->nb_dirs == 0)
		{
			path->directories = NULL;
		}
		else
		{
			path->directories =
				(char **) malloc(path->nb_dirs * sizeof(char *));
		}
	}

	/*
	 * Loop over the directory entries in realpath and split them on reading
	 * the '/' separator, without allocating more memory.
	 */
	for (i = 0, ptr = realpath, previous = ptr;
		 (ptr = strchr(ptr, '/')) != NULL;
		 previous = ++ptr)
	{
		if (ptr == realpath && *ptr == '/')
		{
			/*
			 * That's the first / character, we keep it because it's kind of
			 * both the directory name of the root directory on the file system
			 * and a directory separator.
			 */
			path->directories[i] = (char *) malloc(2*sizeof(char));
			sprintf(path->directories[i++], "/");
		}
		else
		{
			path->directories[i++] = previous;
			*ptr = '\0';
		}
	}

	/*
	 * And now the name and extension, still using the realpath allocated
	 * memory.
	 */
	if (!is_dir)
	{
		char *first_char_after_last_slash = previous;
		char *last_dot = strrchr(previous, '.');

		if (last_dot != NULL && last_dot == first_char_after_last_slash)
		{
			/* the file is .foo, which is a name without extension */
			path->name = first_char_after_last_slash;
			path->extension = NULL;
		}
		else if (last_dot == NULL)
		{
			path->name = first_char_after_last_slash;
			path->extension = NULL;
		}
		else if (last_dot != NULL)
		{
			path->name = first_char_after_last_slash;
			path->extension = last_dot;
			*path->extension++ = '\0';
		}
	}

	return;
}


/*
 * This helper builds a filename from already split pieces. It's then possible
 * to hack e.g. the path->extension then call filepath_rebuild_pathname() to
 * have the magic happen.
 *
 * Note that in principle it could be possible to do that with
 * filepath_merge(), except that a file named '.h' is taken to mean a dotfile,
 * not a filename with only an extension.
 */
Path *
filepath_new_from_pieces(Path *path)
{
	char *filename = (char *) malloc(MAXPATHLEN * sizeof(char));
	sprintf(filename, "");

	/* starting at second directory and accumulating /dirname */
	for (int i = 1; i < path->nb_dirs; i++)
	{
		sprintf(filename, "%s/%s", filename, path->directories[i]);
	}

	/*
	 * Add the name.extension to our filename. Remember that we made sure that
	 * dotfiles don't have an extension, they have a name that begins with dot.
	 */
	if (path->name != NULL)
	{
		sprintf(filename, "%s/%s", filename, path->name);

		if (path->extension != NULL)
		{
			sprintf(filename, "%s.%s", filename, path->extension);
		}
	}

	return filepath_new(filename);
}

/*
 * Free allocated memory for a Path representation.
 */
void
filepath_free(Path *path)
{
	if (path == NULL)
	{
		return;
	}

	if (path->st != NULL)
	{
		free(path->st);
	}

	if (path->realpath != NULL)
	{
		free(path->realpath);
	}

	/*
	 * The directories array is allocated all at once, and the name and
	 * extension live in the same memory area too.
	 */
	if (path->directories != NULL)
	{
		free(*(path->directories));
	}

	free(path);
	return;
}


/*
 * Return a string representation of a filepath
 */
int
filepath_sprintf(char *dest, Path *path)
{
	int len = 0;

	if (path->realpath != NULL)
	{
		len += 1;
		sprintf(dest, "");

		for (int i = 1; i < path->nb_dirs; i++)
		{
			char *dir = path->directories[i];

			len += strlen(dir) + 1;
			sprintf(dest, "%s/%s", dest, dir);
		}

		if (path->name != NULL)
		{
			len += strlen(path->name) + 1;
			sprintf(dest, "%s/%s", dest, path->name);

			if (path->extension != NULL)
			{
				len += strlen(path->extension) + 1;
				sprintf(dest, "%s.%s", dest, path->extension);
			}
		}
	}
	else if (path->filename != NULL)
	{
		len = strlen(path->filename);
		sprintf(dest, "%s", path->filename);
	}
	return len;
}


char *
filepath_get_filename(Path *path)
{
	char *filename = (char *) malloc(MAXPATHLEN * sizeof(char));

	filepath_sprintf(filename, path);

	return filename;
}

/*
 * Print a filepath to stream.
 */
void
filepath_fprintf(FILE *stream, Path *path)
{
	if (path->realpath)
	{
		for (int i = 0; i < path->nb_dirs; i++)
		{
			char *fmt = i == 0 ? "%s" : "%s/";
			fprintf(stream, fmt, path->directories[i]);
		}

		if (path->name != NULL)
		{
			fprintf(stream, "%s", path->name);

			if (path->extension != NULL)
			{
				fprintf(stream, ".%s", path->extension);
			}
		}
	}
	else if (path->filename)
	{
		fprintf(stream, "%s", path->filename);
	}
	return;
}


/*
 * Refresh stats about the given path: we only
 */
void
filepath_refresh_stats(Path *path)
{
	struct stat *st = (struct stat *) malloc(sizeof(struct stat));

	if (path->realpath == NULL)
	{
		path->st = NULL;
		return;
	}

	path->exists = (access(path->realpath, F_OK) == 0);

	/*
	 * Attempt to get stats on the filename.
	 */
	if (path->exists && stat(path->realpath, st) == 0)
	{
		path->st = st;
	}
	else
	{
		path->st = NULL;
	}
	return;
}


/*
 * Return Current Working Directory (as in getcwd()) as a Path.
 */
Path *filepath_cwd()
{
	Path *path;
	char *cwd = (char *) malloc(MAXPATHLEN * sizeof(char));

	cwd = getcwd(cwd, MAXPATHLEN);

	path = filepath_new(cwd);

	return path;
}


/*
 * Build a new Path given an existing path (supposedly a directory) and a
 * relative filename to this directory, which may or may not contain
 * directories too.
 */
Path *
filepath_join(Path *path, const char *filename)
{
	Path *rhs = filepath_new(filename);
	Path *result = NULL;

	if (filepath_is_absolute(rhs))
	{
		return rhs;
	}
	else
	{
		/* size the new specs, adding a / separator and a \0 */
		char *specs = (char *) malloc(strlen(path->filename) +
									  strlen(filename) + 2);

		sprintf(specs, "%s/%s", path->filename, filename);

		result = filepath_new(specs);
	}
	return result;
}


/*
 * Merge filepath specifications into defaults. Any parts that are left empty
 * in the specs are taken from the defaults instead.
 *
 * Typical use case is when building a path from specifications that don't
 * relate to existing entries in the file system yet.
 */
Path *
filepath_merge(Path *specs, Path *defaults)
{
	Path *merge;
	int nb_dirs;
	char **dirs;

	if (specs->nb_dirs > 0)
	{
		merge->nb_dirs = specs->nb_dirs;
		merge->directories = specs->directories;
	}
	else
	{
		merge->nb_dirs = defaults->nb_dirs;
		merge->directories = defaults->directories;
	}

	if (specs->name != NULL || defaults->name != NULL)
	{
		merge->name = specs->name == NULL ? defaults->name : specs->name;

		if (specs->extension != NULL || defaults->extension != NULL)
		{
			merge->extension =
				specs->extension == NULL ? defaults->extension : specs->extension;
		}
	}

	/* filepath_rebuild is going to make copies of the memory */
	return filepath_new_from_pieces(merge);
}


/*
 * Like a merge but for the extension only.
 */
Path *
filepath_with_extension(Path *path, const char *extension)
{
	path->extension = (char *) extension;
	return filepath_new_from_pieces(path);
}


/*
 * Getting the absolute filename of a Path structure is basically a free
 * operation, as all the work is done by calling the realpath(3) system call,
 * which has already been done in filepath_new().
 */
const char *
filepath_absolute_filename(Path *path)
{
	return path->realpath;
}


/*
 * Get the relative path to a file given a reference point, which might or
 * might not be containing the target file.
 */
const char *
filepath_relative_filename(Path *path, Path *maybe_root)
{
	int common_subdirs_count = 0;
	char *relpath;

	/*
	 * Ok this only works with Path that exist and that we know the full
	 * absolute name of, which we call the realpath.
	 */
	if (path->realpath == NULL || maybe_root->realpath == NULL)
	{
		return NULL;
	}

	/*
	 * Compute how many directories are common in both path and maybe_root.
	 */
	for (int i = 0; i < maybe_root->nb_dirs; i++)
	{
		if (strcmp(maybe_root->directories[i], path->directories[i]) == 0)
		{
			common_subdirs_count++;
		}
		else
		{
			/* we're done */
			break;
		}
	}

	/*
	 * Time to allocate our answer.
	 */
	relpath = (char *) malloc(MAXPATHLEN * sizeof(char));

	/*
	 * If path is contained within maybe_root, that's easy, produce:
	 *    ./some/path/to/name.ext.
	 */
	if (common_subdirs_count == maybe_root->nb_dirs)
	{
		sprintf(relpath, ".");

		/* skip the maybe_root directories */
		for(int i = maybe_root->nb_dirs; i < path->nb_dirs; i++)
		{
			sprintf(relpath, "%s/%s", relpath, path->directories[i]);
		}
	}
	else
	{
		/*
		 * Ok, so path isn't contain within maybe_root. Issue as many ../ as
		 * necessary to go back to a common ancestor.
		 */
		int stepback = maybe_root->nb_dirs - common_subdirs_count - 1;

		sprintf(relpath, "..");

		for (int i = 0; i < stepback ; i++)
		{
			sprintf(relpath, "../%s", relpath);
		}

		/*
		 * Now get down to our target, starting at the common place.
		 */
		for (int i = common_subdirs_count; i < path->nb_dirs; i++)
		{
			sprintf(relpath, "%s/%s", relpath, path->directories[i]);
		}
	}

	/*
	 * So relpath is set with directory access, add the name and extension.
	 */
	if (path->name != NULL)
	{
		sprintf(relpath, "%s/%s", relpath, path->name);

		if (path->extension != NULL)
		{
			sprintf(relpath, "%s.%s", relpath, path->extension);
		}
	}
	return relpath;
}


/*
 * The two following functions (filepath_is_absolute and filepath_is_relative)
 * consider the specification given when building the Path structure, which we
 * conveniently still have around.
 */
bool
filename_is_absolute(const char *filename)
{
	return filename && strlen(filename) > 0 && filename[0] == '/';
}


bool
filepath_is_absolute(Path *path)
{
	return filename_is_absolute(path->filename);
}


bool filepath_is_relative(Path *path)
{
	return !filepath_is_absolute(path);
}


/*
 * We cache the fact that a file exists when building our cache.
 */
bool
filepath_file_exists(Path *path)
{
	return path->exists;
}


/*
 * A directory exists when the file exists and is a directory, right?
 */
bool
filepath_directory_exists(Path *path)
{
	return path->exists && path->st != NULL && S_ISDIR(path->st->st_mode);
}


/*
 * Ensure a directory exists at given location. It's like mkdir -p.
 */
bool
filepath_ensure_directories_exist(Path *path, mode_t mode)
{
	if (path->exists)
	{
		return path->st != NULL && S_ISDIR(path->st->st_mode);
	}
	else
	{
		/*
		 * Ok so the path doesn't exists yet.
		 *
		 * The path that's been created might have been normalized as a file,
		 * because maybe a / was not appended to the end of the filename. Make
		 * sure we normalize the filename as a directory here, as a convenience
		 * for our users.
		 */
		int len = strlen(path->filename);
		bool is_dir = len >= 2 && path->filename[len-2] == '/';
		char *currdir = (char *) malloc(MAXPATHLEN * sizeof(char));

		if (!is_dir)
		{
			char *dirname = (char *) malloc(MAXPATHLEN * sizeof(char));

			sprintf(dirname, "%s/", path->filename);
			path->realpath = dirname;
			filepath_normalize_directory(path);
		}

		/* ok, now back to our business of creating directories */
		sprintf(currdir, "");

		for (int i = 1; i < path->nb_dirs; i++)
		{
			bool exists;

			sprintf(currdir, "%s/%s", currdir, path->directories[i]);

			if (access(currdir, F_OK) != 0)
			{
				if (mkdir(currdir, mode) == -1)
				{
					return false;
				}
			}
		}
	}
	return true;
}


/*
 * rm -rf /path/to/dir
 */
bool
filepath_remove_directory(Path *path)
{
	/* of course that only works on files that do exist. */
	if (path->realpath != NULL && path->exists)
	{
		char *topdirs[] = {path->realpath, NULL};
		FTSENT *entry;
		FTS *tree =
			fts_open(topdirs, FTS_PHYSICAL|FTS_NOCHDIR|FTS_NOSTAT, NULL);

		while ((entry = fts_read(tree)) != NULL)
		{
			switch (entry->fts_info)
			{
				/*
				 * We use FTS_DP to remove a directory only once its contents
				 * have been taken care of, and we're careful to ignore FTS_D
				 * here.
				 */
				case FTS_DP:
				case FTS_F:
				case FTS_NS:
				case FTS_NSOK:
				case FTS_SL:
				case FTS_SLNONE:
					if (remove(entry->fts_accpath) == -1)
					{
						return false;
					}
					break;

				case FTS_D:
				default:
					/* ignore other cases */
					break;
			}
		}
		return true;
	}
	return false;
}

/*
 * PathList API, to manipulate an array of Path, such as found in the PATH for
 * instance.
 */
PathList *
filepath_list_new(const char *list)
{
	int i;
	char *previous = (char *)list, *ptr;
	PathList *plist = (PathList *) malloc(sizeof(PathList));

	plist->size = 0;
	plist->list = NULL;

	if (list == NULL)
	{
		return plist;
	}

	/* count PATH separators */
	for (ptr = (char *)list; *ptr != '\0'; ptr++)
	{
		if (*ptr == ':')
		{
			plist->size++;
		}
	}
	plist->list = (Path **) malloc(plist->size * sizeof(Path *));

	i = 0;
    while ((ptr = strchr(previous, ':')) != NULL)
	{
        size_t size = ptr - previous;
		char *entry = (char *) malloc((size+1) * sizeof(char));

		strncpy(entry, previous, size);
		entry[size] = '\0';

		plist->list[i++] = filepath_new(entry);

		previous = ++ptr;
	}

	return plist;
}


void
filepath_list_free(PathList *plist)
{
	for (int i = 0; i < plist->size; i++)
	{
		free(plist->list[i]);
	}
	free(plist);
	return;
}


/*
 * Find filename in path and return all places where we find a matching
 * filename.
 */
PathList *
filepath_list_find(PathList *path, const char *filename)
{
	PathList *result = filepath_list_new(NULL);

	if (filename == NULL)
	{
		return result;
	}
	result->list = (Path **) malloc(path->size * sizeof(Path *));

	for (int i = 0; i < path->size; i++)
	{
		Path *item = path->list[i];

		if (filepath_directory_exists(item))
		{
			Path *candidate = filepath_join(item, filename);

			if (candidate->exists)
			{
				result->list[result->size++] = candidate;
			}
		}
	}
	return result;
}


#endif  /* __FILEPATHS_H__ */

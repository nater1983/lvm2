/*
 * Copyright (C) 2001 Sistina Software (UK) Limited.
 *
 * This file is released under the LGPL.
 *
 */

#include "lib.h"
#include "toolcontext.h"
#include "pool.h"
#include "metadata.h"
#include "defaults.h"
#include "lvm-string.h"
#include "activate.h"
#include "filter.h"
#include "filter-composite.h"
#include "filter-persistent.h"
#include "filter-regex.h"
#include "label.h"
#include "lvm-file.h"
#include "format-text.h"
#include "display.h"

#ifdef HAVE_LIBDL
#include "sharedlib.h"
#endif

#ifdef LVM1_INTERNAL
#include "format1.h"
#endif

#include <locale.h>
#include <sys/stat.h>
#include <syslog.h>
#include <time.h>

static FILE *_log;

static int _get_env_vars(struct cmd_context *cmd)
{
	const char *e;

	/* Set to "" to avoid using any system directory */
	if ((e = getenv("LVM_SYSTEM_DIR"))) {
		if (lvm_snprintf(cmd->sys_dir, sizeof(cmd->sys_dir),
				 "%s", e) < 0) {
			log_error("LVM_SYSTEM_DIR environment variable "
				  "is too long.");
			return 0;
		}
	}

	return 1;
}

static void _init_logging(struct cmd_context *cmd)
{
	const char *open_mode = "a";
	time_t t;

	const char *log_file;

	/* Syslog */
	cmd->default_settings.syslog =
	    find_config_int(cmd->cf->root, "log/syslog", '/', DEFAULT_SYSLOG);
	if (cmd->default_settings.syslog != 1)
		fin_syslog();

	if (cmd->default_settings.syslog > 1)
		init_syslog(cmd->default_settings.syslog);

	/* Debug level for log file output */
	cmd->default_settings.debug =
	    find_config_int(cmd->cf->root, "log/level", '/', DEFAULT_LOGLEVEL);
	init_debug(cmd->default_settings.debug);

	/* Verbose level for tty output */
	cmd->default_settings.verbose =
	    find_config_int(cmd->cf->root, "log/verbose", '/', DEFAULT_VERBOSE);
	init_verbose(cmd->default_settings.verbose);

	/* Log message formatting */
	init_indent(find_config_int(cmd->cf->root, "log/indent", '/',
				    DEFAULT_INDENT));

	cmd->default_settings.msg_prefix = find_config_str(cmd->cf->root,
							   "log/prefix", '/',
							   DEFAULT_MSG_PREFIX);
	init_msg_prefix(cmd->default_settings.msg_prefix);

	cmd->default_settings.cmd_name = find_config_int(cmd->cf->root,
							 "log/command_names",
							 '/', DEFAULT_CMD_NAME);
	init_cmd_name(cmd->default_settings.cmd_name);

	/* Test mode */
	cmd->default_settings.test =
	    find_config_int(cmd->cf->root, "global/test", '/', 0);

	/* Settings for logging to file */
	if (find_config_int(cmd->cf->root, "log/overwrite", '/',
			    DEFAULT_OVERWRITE))
		open_mode = "w";

	log_file = find_config_str(cmd->cf->root, "log/file", '/', 0);
	if (log_file) {
		/* set up the logging */
		if (!(_log = fopen(log_file, open_mode)))
			log_error("Couldn't open log file %s", log_file);
		else
			init_log(_log);
	}

	t = time(NULL);
	log_verbose("Logging initialised at %s", ctime(&t));

	/* Tell device-mapper about our logging */
#ifdef DEVMAPPER_SUPPORT
	dm_log_init(print_log);
#endif
}

static int _process_config(struct cmd_context *cmd)
{
	mode_t old_umask;

	/* umask */
	cmd->default_settings.umask = find_config_int(cmd->cf->root,
						      "global/umask", '/',
						      DEFAULT_UMASK);

	if ((old_umask = umask((mode_t) cmd->default_settings.umask)) !=
	    (mode_t) cmd->default_settings.umask)
		log_verbose("Set umask to %04o", cmd->default_settings.umask);

	/* dev dir */
	if (lvm_snprintf(cmd->dev_dir, sizeof(cmd->dev_dir), "%s/",
			 find_config_str(cmd->cf->root, "devices/dir",
					 '/', DEFAULT_DEV_DIR)) < 0) {
		log_error("Device directory given in config file too long");
		return 0;
	}
#ifdef DEVMAPPER_SUPPORT
	dm_set_dev_dir(cmd->dev_dir);
#endif

	/* proc dir */
	if (lvm_snprintf(cmd->proc_dir, sizeof(cmd->proc_dir), "%s",
			 find_config_str(cmd->cf->root, "global/proc",
					 '/', DEFAULT_PROC_DIR)) < 0) {
		log_error("Device directory given in config file too long");
		return 0;
	}

	/* activation? */
	cmd->default_settings.activation = find_config_int(cmd->cf->root,
							   "global/activation",
							   '/',
							   DEFAULT_ACTIVATION);
	set_activation(cmd->default_settings.activation);

	cmd->default_settings.suffix = find_config_int(cmd->cf->root,
						       "global/suffix",
						       '/', DEFAULT_SUFFIX);

	if (!(cmd->default_settings.unit_factor =
	      units_to_bytes(find_config_str(cmd->cf->root,
					     "global/units",
					     '/',
					     DEFAULT_UNITS),
			     &cmd->default_settings.unit_type))) {
		log_error("Invalid units specification");
		return 0;
	}

	return 1;
}

/* Find and read config file */
static int _init_config(struct cmd_context *cmd)
{
	struct stat info;
	char config_file[PATH_MAX] = "";

	if (!(cmd->cf = create_config_tree())) {
		stack;
		return 0;
	}

	/* No config file if LVM_SYSTEM_DIR is empty */
	if (!*cmd->sys_dir)
		return 1;

	if (lvm_snprintf(config_file, sizeof(config_file),
			 "%s/lvm.conf", cmd->sys_dir) < 0) {
		log_error("LVM_SYSTEM_DIR was too long");
		destroy_config_tree(cmd->cf);
		return 0;
	}

	/* Is there a config file? */
	if (stat(config_file, &info) == -1) {
		if (errno == ENOENT)
			return 1;
		log_sys_error("stat", config_file);
		destroy_config_tree(cmd->cf);
		return 0;
	}

	if (!read_config_file(cmd->cf, config_file)) {
		log_error("Failed to load config file %s", config_file);
		destroy_config_tree(cmd->cf);
		return 0;
	}

	return 1;
}

static int _init_dev_cache(struct cmd_context *cmd)
{
	struct config_node *cn;
	struct config_value *cv;

	if (!dev_cache_init()) {
		stack;
		return 0;
	}

	if (!(cn = find_config_node(cmd->cf->root, "devices/scan", '/'))) {
		if (!dev_cache_add_dir("/dev")) {
			log_error("Failed to add /dev to internal "
				  "device cache");
			return 0;
		}
		log_verbose("device/scan not in config file: "
			    "Defaulting to /dev");
		return 1;
	}

	for (cv = cn->v; cv; cv = cv->next) {
		if (cv->type != CFG_STRING) {
			log_error("Invalid string in config file: "
				  "devices/scan");
			return 0;
		}

		if (!dev_cache_add_dir(cv->v.str)) {
			log_error("Failed to add %s to internal device cache",
				  cv->v.str);
			return 0;
		}
	}

	return 1;
}

static struct dev_filter *_init_filter_components(struct cmd_context *cmd)
{
	struct config_node *cn;
	struct dev_filter *f1, *f2, *f3;

	cn = find_config_node(cmd->cf->root, "devices/types", '/');

	if (!(f2 = lvm_type_filter_create(cmd->proc_dir, cn)))
		return NULL;

	if (!(cn = find_config_node(cmd->cf->root, "devices/filter", '/'))) {
		log_debug("devices/filter not found in config file: no regex "
			  "filter installed");
		return f2;
	}

	if (!(f1 = regex_filter_create(cn->v))) {
		log_error("Failed to create regex device filter");
		return NULL;
	}

	if (!(f3 = composite_filter_create(2, f1, f2))) {
		log_error("Failed to create composite device filter");
		return NULL;
	}

	return f3;
}

static int _init_filters(struct cmd_context *cmd)
{
	const char *lvm_cache;
	struct dev_filter *f3, *f4;
	struct stat st;
	char cache_file[PATH_MAX];

	cmd->dump_filter = 0;

	if (!(f3 = _init_filter_components(cmd)))
		return 0;

	if (lvm_snprintf(cache_file, sizeof(cache_file),
			 "%s/.cache", cmd->sys_dir) < 0) {
		log_error("Persistent cache filename too long ('%s/.cache').",
			  cmd->sys_dir);
		return 0;
	}

	lvm_cache =
	    find_config_str(cmd->cf->root, "devices/cache", '/', cache_file);
	if (!(f4 = persistent_filter_create(f3, lvm_cache))) {
		log_error("Failed to create persistent device filter");
		return 0;
	}

	/* Should we ever dump persistent filter state? */
	if (find_config_int(cmd->cf->root, "devices/write_cache_state", '/', 1))
		cmd->dump_filter = 1;

	if (!*cmd->sys_dir)
		cmd->dump_filter = 0;

	if (!stat(lvm_cache, &st) &&
	    (st.st_mtime > config_file_timestamp(cmd->cf)) &&
	    !persistent_filter_load(f4))
		log_verbose("Failed to load existing device cache from %s",
			    lvm_cache);

	cmd->filter = f4;

	return 1;
}

static int _init_formats(struct cmd_context *cmd)
{
	const char *format;

	struct format_type *fmt;
	struct list *fmth;

#ifdef HAVE_LIBDL
	struct config_node *cn;
#endif

	label_init();

#ifdef LVM1_INTERNAL
	if (!(fmt = init_lvm1_format(cmd)))
		return 0;
	fmt->library = NULL;
	list_add(&cmd->formats, &fmt->list);
#endif

#ifdef HAVE_LIBDL
	/* Load any formats in shared libs */
	if ((cn = find_config_node(cmd->cf->root, "global/format_libraries",
				   '/'))) {

		struct config_value *cv;
		struct format_type *(*init_format_fn) (struct cmd_context *);
		void *lib;

		for (cv = cn->v; cv; cv = cv->next) {
			if (cv->type != CFG_STRING) {
				log_error("Invalid string in config file: "
					  "global/format_libraries");
				return 0;
			}
			if (!(lib = load_shared_library(cmd->cf, cv->v.str,
							"format"))) {
				stack;
				return 0;
			}

			if (!(init_format_fn = dlsym(lib, "init_format"))) {
				log_error("Shared library %s does not contain "
					  "format functions", cv->v.str);
				dlclose(lib);
				return 0;
			}

			if (!(fmt = init_format_fn(cmd)))
				return 0;
			fmt->library = lib;
			list_add(&cmd->formats, &fmt->list);
		}
	}
#endif

	if (!(fmt = create_text_format(cmd)))
		return 0;
	fmt->library = NULL;
	list_add(&cmd->formats, &fmt->list);

	cmd->fmt_backup = fmt;

	format = find_config_str(cmd->cf->root, "global/format", '/',
				 DEFAULT_FORMAT);

	list_iterate(fmth, &cmd->formats) {
		fmt = list_item(fmth, struct format_type);
		if (!strcasecmp(fmt->name, format) ||
		    (fmt->alias && !strcasecmp(fmt->alias, format))) {
			cmd->default_settings.fmt = fmt;
			return 1;
		}
	}

	log_error("_init_formats: Default format (%s) not found", format);
	return 0;
}

/* Entry point */
struct cmd_context *create_toolcontext(struct arg *the_args)
{
	struct cmd_context *cmd;

	if (!setlocale(LC_ALL, ""))
		log_error("setlocale failed");

	init_syslog(DEFAULT_LOG_FACILITY);

	if (!(cmd = dbg_malloc(sizeof(*cmd)))) {
		log_error("Failed to allocate command context");
		return NULL;
	}
	memset(cmd, 0, sizeof(*cmd));
	cmd->args = the_args;
	list_init(&cmd->formats);

	strcpy(cmd->sys_dir, DEFAULT_SYS_DIR);

	if (!_get_env_vars(cmd))
		goto error;

	/* Create system directory if it doesn't already exist */
	if (*cmd->sys_dir && !create_dir(cmd->sys_dir))
		goto error;

	if (!_init_config(cmd))
		goto error;

	_init_logging(cmd);

	if (!_process_config(cmd))
		goto error;

	if (!_init_dev_cache(cmd))
		goto error;

	if (!_init_filters(cmd))
		goto error;

	if (!(cmd->mem = pool_create(4 * 1024))) {
		log_error("Command memory pool creation failed");
		return 0;
	}

	if (!_init_formats(cmd))
		goto error;

	cmd->current_settings = cmd->default_settings;

	return cmd;

      error:
	dbg_free(cmd);
	return NULL;
}

static void _destroy_formats(struct list *formats)
{
	struct list *fmtl, *tmp;
	struct format_type *fmt;
	void *lib;

	list_iterate_safe(fmtl, tmp, formats) {
		fmt = list_item(fmtl, struct format_type);
		list_del(&fmt->list);
		lib = fmt->library;
		fmt->ops->destroy(fmt);
#ifdef HAVE_LIBDL
		if (lib)
			dlclose(lib);
#endif
	}
}

void destroy_toolcontext(struct cmd_context *cmd)
{
	if (cmd->dump_filter)
		persistent_filter_dump(cmd->filter);

	cache_destroy();
	label_exit();
	_destroy_formats(&cmd->formats);
	cmd->filter->destroy(cmd->filter);
	pool_destroy(cmd->mem);
	dev_cache_exit();
	destroy_config_tree(cmd->cf);
	dbg_free(cmd);

	dump_memory();
	fin_log();
	fin_syslog();

	if (_log)
		fclose(_log);

}

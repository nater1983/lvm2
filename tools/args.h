/*
 * Copyright (C) 2001 Sistina Software (UK) Limited.
 *
 * This file is released under the LGPL.
 */

/*
 * Put all long args that don't have a
 * corresponding short option first ...
 */
arg(version_ARG, '\0', "version", NULL)
arg(quiet_ARG, '\0', "quiet", NULL)
arg(physicalvolumesize_ARG, '\0', "setphysicalvolumesize", size_arg)
arg(ignorelockingfailure_ARG, '\0', "ignorelockingfailure", NULL)
arg(metadatacopies_ARG, '\0', "metadatacopies", int_arg)
arg(metadatasize_ARG, '\0', "metadatasize", size_arg)
arg(restorefile_ARG, '\0', "restorefile", string_arg)
arg(labelsector_ARG, '\0', "labelsector", int_arg)
arg(driverloaded_ARG, '\0', "driverloaded", yes_no_arg)

/* Allow some variations */
arg(resizable_ARG, '\0', "resizable", yes_no_arg)
arg(allocation_ARG, '\0', "allocation", yes_no_arg)

/*
 * ... and now the short args.
 */
arg(available_ARG, 'a', "available", yes_no_arg)
arg(all_ARG, 'a', "all", NULL)
arg(autobackup_ARG, 'A', "autobackup", yes_no_arg)
arg(activevolumegroups_ARG, 'A', "activevolumegroups", NULL)
arg(blockdevice_ARG, 'b', "blockdevice", NULL)
arg(chunksize_ARG, 'c', "chunksize", size_arg)
arg(colon_ARG, 'c', "colon", NULL)
arg(contiguous_ARG, 'C', "contiguous", yes_no_arg)
arg(debug_ARG, 'd', "debug", NULL)
arg(disk_ARG, 'D', "disk", NULL)
arg(exported_ARG, 'e', "exported", NULL)
arg(physicalextent_ARG, 'E', "physicalextent", NULL)
arg(file_ARG, 'f', "file", string_arg)
arg(force_ARG, 'f', "force", NULL)
arg(full_ARG, 'f', "full", NULL)
arg(help_ARG, 'h', "help", NULL)
arg(help2_ARG, '?', "", NULL)
arg(stripesize_ARG, 'I', "stripesize", size_arg)
arg(stripes_ARG, 'i', "stripes", int_arg)
arg(iop_version_ARG, 'i', "iop_version", NULL)
arg(logicalvolume_ARG, 'l', "logicalvolume", int_arg)
arg(maxlogicalvolumes_ARG, 'l', "maxlogicalvolumes", int_arg)
arg(extents_ARG, 'l', "extents", int_arg_with_sign)
arg(lvmpartition_ARG, 'l', "lvmpartition", NULL)
arg(list_ARG, 'l', "list", NULL)
arg(size_ARG, 'L', "size", size_arg)
arg(logicalextent_ARG, 'L', "logicalextent", int_arg_with_sign)
arg(persistent_ARG, 'M', "persistent", yes_no_arg)
arg(metadatatype_ARG, 'M', "metadatatype", metadatatype_arg)
arg(minor_ARG, 'm', "minor", minor_arg)
arg(maps_ARG, 'm', "maps", NULL)
arg(name_ARG, 'n', "name", string_arg)
arg(oldpath_ARG, 'n', "oldpath", NULL)
arg(nofsck_ARG, 'n', "nofsck", NULL)
arg(novolumegroup_ARG, 'n', "novolumegroup", NULL)
arg(permission_ARG, 'p', "permission", permission_arg)
arg(maxphysicalvolumes_ARG, 'p', "maxphysicalvolumes", int_arg)
arg(partial_ARG, 'P', "partial", NULL)
arg(physicalvolume_ARG, 'P', "physicalvolume", NULL)
arg(readahead_ARG, 'r', "readahead", int_arg)
arg(reset_ARG, 'R', "reset", NULL)
arg(physicalextentsize_ARG, 's', "physicalextentsize", size_arg)
arg(stdin_ARG, 's', "stdin", NULL)
arg(snapshot_ARG, 's', "snapshot", NULL)
arg(short_ARG, 's', "short", NULL)
arg(test_ARG, 't', "test", NULL)
arg(uuid_ARG, 'u', "uuid", NULL)
arg(uuidstr_ARG, 'u', "uuid", string_arg)
arg(uuidlist_ARG, 'U', "uuidlist", NULL)
arg(verbose_ARG, 'v', "verbose", NULL)
arg(volumegroup_ARG, 'V', "volumegroup", NULL)
arg(allocatable_ARG, 'x', "allocatable", yes_no_arg)
arg(resizeable_ARG, 'x', "resizeable", yes_no_arg)
arg(yes_ARG, 'y', "yes", NULL)
arg(zero_ARG, 'Z', "zero", yes_no_arg)

/* this should always be last */
arg(ARG_COUNT, '-', "", NULL)

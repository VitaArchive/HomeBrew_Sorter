/*
	Adrenaline VSH Control
	Copyright (C) 2025, GrayJack
	Copyright (C) 2021, PRO CFW

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <string.h>

#include <psperror.h>
#include <pspiofilemgr.h>
#include <pspsysmem_kernel.h>
#include <psputilsforkernel.h>

#include <vshctrl.h>
#include <cfwmacros.h>
#include <systemctrl.h>
#include <systemctrl_se.h>

#include <adrenaline_log.h>

#include "isoreader.h"
#include "iso_common.h"

#define MAX_RETRIES 10
#define MAX_DIR_LEVEL 8
#define ISO_STANDARD_ID "CD001"

typedef unsigned int uint;

static Iso9660DirectoryRecord g_root_record;

static inline u32 isoPos2LBA(u32 pos) {
	return pos / SECTOR_SIZE;
}

static inline u32 isoLBA2Pos(u32 lba, int offset) {
	return lba * SECTOR_SIZE + offset;
}

static inline u32 isoPos2OffsetInSector(u32 pos) {
	return pos & (SECTOR_SIZE - 1);
}

static inline u32 isoPos2RestSize(u32 pos) {
	return SECTOR_SIZE - isoPos2OffsetInSector(pos);
}

static int readSector(u32 sector, u8* buf) {
	IoReadArg read_arg = {
		.offset = sector*ISO_SECTOR_SIZE,
		.address = buf,
		.size = ISO_SECTOR_SIZE,
	};
	return iso_read(&read_arg);
}

static void normalizeName(char *filename) {
	char *p;

	p = strstr(filename, ";1");

	if (p) {
		*p = '\0';
	}
}

static int findFile(const char * file, u32 lba, u32 dir_size, u32 is_dir, Iso9660DirectoryRecord *result_record) {
	char name[32];
	int ret;

	u32 pos = isoLBA2Pos(lba, 0);
	int re = lba = 0;

	while (re < dir_size) {
		if (isoPos2LBA(pos) != lba) {
			lba = isoPos2LBA(pos);
			ret = readSector(lba, (u8*)g_sector_buffer);

			if (ret != SECTOR_SIZE) {
				return SCE_ENOENT;
			}
		}

		Iso9660DirectoryRecord *rec = (Iso9660DirectoryRecord*)&g_sector_buffer[isoPos2OffsetInSector(pos)];

		if (rec->len_dr == 0) {
			u32 remaining;

			remaining = isoPos2RestSize(pos);
			pos += remaining;
			re += remaining;
			continue;
		}

		#ifdef DEBUG
		if (rec->len_dr < rec->len_fi + sizeof(*rec)) {
			logmsg("%s: Corrupted directory record found in %s, LBA %d\n", __func__, g_iso_fn, (int)lba);
		}
		#endif

		if (rec->len_fi > 32) {
			return SCE_EINVAL;
		}

		if (rec->len_fi == 1 && rec->fi == 0) {
			if (0 == strcmp(file, ".")) {
				memcpy(result_record, rec, sizeof(*result_record));

				return 0;
			}
		} else if (rec->len_fi == 1 && rec->fi == 1) {
			if (0 == strcmp(file, "..")) {
				// didn't support ..
				return SCE_ERR_NOT_SUPP;
			}
		} else {
			memset(name, 0, sizeof(name));
			memcpy(name, &rec->fi, rec->len_fi);
			normalizeName(name);

			if (0 == strcmp(name, file)) {
				if (is_dir) {
					if(!(rec->fileFlags & ISO9660_FILEFLAGS_DIR)) {
						return SCE_ENOTDIR;
					}
				}

				memcpy(result_record, rec, sizeof(*result_record));

				return 0;
			}
		}

		pos += rec->len_dr;
		re += rec->len_dr;
	}

	return SCE_ENOENT;
}

static int findPath(const char *path, Iso9660DirectoryRecord *result_record) {
	int level = 0;
	char cur_dir[32];

	if (result_record == NULL) {
		return SCE_EINVAL;
	}

	memset(result_record, 0, sizeof(*result_record));
	u32 lba = g_root_record.lsbStart;
	u32 dir_size = g_root_record.lsbDataLength;

	const char *cur_path = path;

	while (*cur_path == '/') {
		cur_path++;
	}

	const char *next = strchr(cur_path, '/');

	int ret;
	while (next != NULL) {
		if (next-cur_path >= sizeof(cur_dir)) {
			return -15;
		}

		memset(cur_dir, 0, sizeof(cur_dir));
		strncpy(cur_dir, cur_path, next-cur_path);
		cur_dir[next-cur_path] = '\0';

		if (0 == strcmp(cur_dir, ".")) {
		} else if (0 == strcmp(cur_dir, "..")) {
			level--;
		} else {
			level++;
		}

		if (level > MAX_DIR_LEVEL) {
			return SCE_EINVAL;
		}

		ret = findFile(cur_dir, lba, dir_size, 1, result_record);

		if (ret < 0) {
			return ret;
		}

		lba = result_record->lsbStart;
		dir_size = result_record->lsbDataLength;

		cur_path=next+1;

		// skip unwant path separator
		while (*cur_path == '/') {
			cur_path++;
		}

		next = strchr(cur_path, '/');
	}

	ret = findFile(cur_path, lba, dir_size, 0, result_record);

	return ret;
}

int isoOpen(const char *path) {
	int ret = -1;

	if (path == NULL) {
		return SCE_EINVAL;
	}

	if (strnlen(path, 256) > 255) {
		return SCE_ENAMETOOLONG;
	}

	int k1 = pspSdkSetK1(0);

	if (g_iso_fd >= 0) {
		isoClose();
	}

	strncpy(g_iso_fn, path, 255);

	if (g_iso_fn[0] == '\0' || iso_re_open() < 0) {
		logmsg("%s: open failed %s -> 0x%08X\n", __func__, g_iso_fn, g_iso_fd);
		ret = SCE_EIO;
		goto error;
	}

	iso_alloc(DAX_COMP_BUF);
	iso_type_check(g_iso_fd);

	ret = readSector(16, (u8*)g_sector_buffer);

	if (ret != SECTOR_SIZE) {
		ret = SCE_EIO;
		goto error;
	}

	if (memcmp(&g_sector_buffer[1], ISO_STANDARD_ID, sizeof(ISO_STANDARD_ID)-1)) {
		logmsg("%s: [ERROR]: vol descriptor not found\n", __func__);
		ret = SCE_EINVAL;

		goto error;
	}

	memcpy(&g_root_record, &g_sector_buffer[0x9C], sizeof(g_root_record));

	pspSdkSetK1(k1);
	return 0;

error:
	if (g_iso_fd >= 0) {
		isoClose();
	}
	pspSdkSetK1(k1);
	return ret;
}

int isoGetTotalSectorSize(void) {
	return g_total_sectors;
}

void isoClose(void) {
	int k1 = pspSdkSetK1(0);
	iso_close();
	pspSdkSetK1(k1);
}

int isoGetFileInfo(const char* path, u32 *filesize, u32 *lba) {
	int ret = 0;
	Iso9660DirectoryRecord rec;
	int k1 = pspSdkSetK1(0);

	ret = findPath(path, &rec);

	if (ret >= 0) {
		if (lba) {
		   *lba = rec.lsbStart;
		}
		if (filesize) {
			*filesize = rec.lsbDataLength;
		}
	}

	pspSdkSetK1(k1);
	return ret;
}

int isoRead(void *buffer, u32 lba, int offset, u32 size) {
	int k1 = pspSdkSetK1(0);
	u32 pos = isoLBA2Pos(lba, offset);
	IoReadArg read_arg = {
		.offset = pos,
		.address = buffer,
		.size = size,
	};
	int res = iso_read(&read_arg);
	pspSdkSetK1(k1);
	return res;
}

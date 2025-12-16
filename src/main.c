#include <time.h>

#include <pspsdk.h>
#include <pspkernel.h>

#include <oslib/oslib.h>

#define _ADRENALINE_LOG_IMPL_
#include <adrenaline_log.h>

#include "media.h"
#include "fileOperation.h"
#include "isoreader.h"

PSP_MODULE_INFO("TitleSorter", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(12*1024);

#define ANALOG_SENS 40

enum HbSorterModes {
	MODE_TITLELIST = 0,
	MODE_CATEGORIES = 1,
};

/* Globals: */
static Title g_titles_list[MAX_HB];
static int g_titles_count = 0;
static Categories g_cat_list[MAX_CAT];
static Categories g_cat_list_norep[MAX_CAT];
static int g_cat_count = 0;
static int g_cat_count_norep = 0;
static int g_is_browser_mode = 0;

static OSL_IMAGE *g_bkg;
static OSL_IMAGE *g_startb;
static OSL_IMAGE *g_cross;
static OSL_IMAGE *g_circle;
static OSL_IMAGE *g_square;
static OSL_IMAGE *g_triangle;
static OSL_IMAGE *g_folder;
static OSL_IMAGE *g_iso;
static OSL_IMAGE *g_icon0;
static OSL_IMAGE *g_R;
static OSL_IMAGE *g_L;
static OSL_FONT *g_pgf_font;

static char g_temp_name[262];

/* Draw toolbars: */
static char g_toolbars[100];
static char g_titlesfound[100];

static void drawToolbars(int mode) {
	oslDrawFillRect(0,0,480,15,RGBA(0,0,0,170));
	oslDrawString(5,0,"Title Sorter");
	if (!mode) {
		sprintf(g_titlesfound,"Titles found: %i", g_titles_count);
	} else {
		sprintf(g_titlesfound,"                       ");
	}
	oslDrawString(195,0,g_titlesfound);
	//Current time:
	struct tm * ptm;
	time_t mytime;
	time(&mytime);
	ptm = localtime(&mytime);
	sprintf(g_toolbars,"%2.2d/%2.2d/%4.4d %2.2d:%2.2d",ptm->tm_mday, ptm->tm_mon + 1, ptm->tm_year + 1900, ptm->tm_hour,ptm->tm_min);
	oslDrawString(360,0,g_toolbars);
}

static void getIcon0(char* filename) {
	//unsigned char _header[40];
	int icon0Offset, icon1Offset;
	char file[256];
	sprintf(file,"%s/eboot.pbp",filename);
	SceUID fd = sceIoOpen(file, 0x0001/*O_RDONLY*/, 0777);

	if (fd < 0) {
		g_icon0 = oslLoadImageFilePNG("ram:/Media/icon0.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
		return;
	}

	sceIoLseek(fd, 12, SEEK_SET);
	sceIoRead(fd, &icon0Offset, 4);
	sceIoRead(fd, &icon1Offset, 4);
	int icon0_size = icon1Offset - icon0Offset;
	sceIoLseek(fd, icon0Offset, SEEK_SET);
	unsigned char icon[icon0_size];

	if (icon0_size) {
		sceIoRead(fd, icon, icon0_size);
		oslSetTempFileData(icon, icon0_size, &VF_MEMORY);
		g_icon0 = oslLoadImageFilePNG(oslGetTempFileName(), OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
	} else {
		g_icon0 = oslLoadImageFilePNG("ram:/Media/icon0.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
	}

	sceIoClose(fd);
}

static void getIcon0_fromfile(char* filename) {
	char file[256];
	SceOff icon0_size;

	sprintf(file,"%s/icon0.png",filename);
	SceUID fd = sceIoOpen(file, PSP_O_RDONLY, 0777);

	if (fd < 0) {
		g_icon0 = oslLoadImageFilePNG("ram:/Media/icon0.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
		return;
	}

	icon0_size = sceIoLseek(fd, 0, SEEK_END);
	sceIoLseek(fd, 0, SEEK_SET);
	unsigned char icon[icon0_size];
	if (icon0_size) {
		sceIoRead(fd, icon, icon0_size);
		oslSetTempFileData(icon, icon0_size, &VF_MEMORY);
		g_icon0 = oslLoadImageFilePNG(oslGetTempFileName(), OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
	} else {
		g_icon0 = oslLoadImageFilePNG("ram:/Media/icon0.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
	}
	sceIoClose(fd);
}

static void getIcon0_fromiso(char* filename) {
	int res = isoOpen(filename);

	if (res < 0) {
		g_icon0 = oslLoadImageFilePNG("ram:/Media/icon0.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
		return;
	}

	u32 icon0_size = 0;
	u32 lba = 0;
	res = isoGetFileInfo("/PSP_GAME/ICON0.PNG", &icon0_size, &lba);

	if (res < 0) {
		g_icon0 = oslLoadImageFilePNG("ram:/Media/icon0.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
		isoClose();
		return;
	}

	unsigned char icon[icon0_size];
	if (icon0_size > 0) {
		isoRead(icon, lba, 0 , icon0_size);
		oslSetTempFileData(icon, icon0_size, &VF_MEMORY);
		g_icon0 = oslLoadImageFilePNG(oslGetTempFileName(), OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
	} else {
		g_icon0 = oslLoadImageFilePNG("ram:/Media/icon0.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
	}

	isoClose();
}

/*  Main menu: */
static int mainMenu() {
	int mode = MODE_TITLELIST;
	int skip = 0;
	int start = 27;
	int first = 0, catFirst =0, hbFirst =0;
	int total = g_titles_count;
	int visible = 13;
	int selected = 0;
	int catSelected=0;
	int hbSelected=0;
	int oldSelected=-1;
	int i = 0;
	int flag=0;
	int enable = 1;

	while (!osl_quit) {
		if (!skip) {
			oslStartDrawing();
			oslDrawImageXY(g_bkg, 0, 0);
			drawToolbars(mode);
			oslDrawFillRect(5,22,285,248,RGBA(0,0,0,170));
			if (g_cat_count != 0) {
				oslDrawFillRect(290,22,475,113,RGBA(0,0,0,170));
			} else {
				oslDrawFillRect(290,22,475,93,RGBA(0,0,0,170));
			}
			oslDrawImageXY(g_cross,305,30);
			oslDrawString(335,30,"Select/Release");
			oslDrawImageXY(g_circle,305,50);

			if (enable) {
				oslIntraFontSetStyle(g_pgf_font, 0.5, RGBA(255,255,255,100), RGBA(0,0,0,0), 0.0, INTRAFONT_ALIGN_LEFT);
				oslDrawString(335,50,"Hide icon0");
				oslIntraFontSetStyle(g_pgf_font, 0.5, RGBA(255,255,255,255), RGBA(0,0,0,0), 0.0, INTRAFONT_ALIGN_LEFT);
			} else {
				oslDrawString(335,50,"Show icon0");
			}

			oslDrawImageXY(g_startb,295,70);
			oslDrawString(335,70,"Save List");

			if (g_cat_count != 0) {
				oslDrawImageXY(g_triangle,305,90);
				oslDrawString(335,90,"Return");
			}

			oslDrawFillRect(290,118,475,155,RGBA(0,0,0,170));
			oslDrawImageXY(g_L,295,122);
			oslDrawString(350, 122, "Change view");

			if (mode == MODE_TITLELIST) {
				oslDrawString(355, 135, "Title List");
			} else if (mode ==1) {
				oslDrawString(355, 135, "Categories");
			}

			oslDrawImageXY(g_R,452,122);

			//Draw menu:
			for (i=first; i<=first+visible; i++) {
				if (i == selected) {
					oslIntraFontSetStyle(g_pgf_font, 0.5, RGBA(20,20,20,255), RGBA(255,255,255,200), 0.0, INTRAFONT_ALIGN_LEFT);
					oslSetFont(g_pgf_font);
					if (enable && !mode) {
						if (oldSelected != selected) {
							if (g_icon0!=NULL) {
								oslDeleteImage(g_icon0);
							}
							oldSelected = selected;
							if (g_titles_list[i].type == TITLE_EBOOT) {
								getIcon0(g_titles_list[i].path);
							} else if (g_titles_list[i].type == TITLE_ISO) {
								getIcon0_fromiso(g_titles_list[i].path);
							}
						}

						if (g_icon0!=NULL) {
							//oslDrawImageXY(icon0, 315,150);
							oslDrawImageXY(g_icon0, 312,168);
						}

					} else if (enable && mode) {
						if (oldSelected != selected) {
							if (g_icon0!=NULL) {
								oslDeleteImage(g_icon0);
							}
							oldSelected = selected;
							getIcon0_fromfile(g_cat_list[i].path);
						}

						if (g_icon0!=NULL) {
							oslDrawImageXY(g_icon0, 312,168);
						}
					}
				} else {
					oslIntraFontSetStyle(g_pgf_font, 0.5, RGBA(255,255,255,255), RGBA(0,0,0,0), 0.0, INTRAFONT_ALIGN_LEFT);
					oslSetFont(g_pgf_font);
				}

				if (i < total) {
					if (mode == MODE_TITLELIST) {
						if (g_titles_list[i].type == TITLE_EBOOT) {
							oslDrawImageXY(g_folder,12,start +(i - first)*oslGetImageHeight(g_folder));
						} else {
							oslDrawImageXY(g_iso,12,start +(i - first)*oslGetImageHeight(g_folder));
						}

						if (strcmp(g_titles_list[i].category, "Uncategorized") != 0) {
							strcpy(g_temp_name, g_titles_list[i].category);
							strcat(g_temp_name, ": ");
						}
						strcat(g_temp_name, g_titles_list[i].name);
						oslDrawString(15+oslGetImageWidth(g_folder),start +(i - first)*oslGetImageHeight(g_folder), g_temp_name);//HBlist[i].name);

					} else if (mode == MODE_CATEGORIES) {
						oslDrawImageXY(g_folder,12,start +(i - first)*oslGetImageHeight(g_folder));
						oslIntraFontSetStyle(g_pgf_font, 0.5, RGBA(0,0,0,255), RGBA(0,0,0,0), 0.0, INTRAFONT_ALIGN_LEFT);
						oslSetFont(g_pgf_font);
						oslDrawString(10+oslGetImageWidth(g_folder)/4,start +(i - first)*oslGetImageHeight(g_folder)+2, "C");

						if (i == selected) {
							oslIntraFontSetStyle(g_pgf_font, 0.5, RGBA(20,20,20,255), RGBA(255,255,255,200), 0.0, INTRAFONT_ALIGN_LEFT);
							oslSetFont(g_pgf_font);
						} else {
							oslIntraFontSetStyle(g_pgf_font, 0.5, RGBA(255,255,255,255), RGBA(0,0,0,0), 0.0, INTRAFONT_ALIGN_LEFT);
							oslSetFont(g_pgf_font);
						}

						if (g_cat_list[i].repeated) {
							oslDrawString(15+oslGetImageWidth(g_folder),start +(i - first)*oslGetImageHeight(g_folder), g_cat_list[i].path);
						} else {
							oslDrawString(15+oslGetImageWidth(g_folder),start +(i - first)*oslGetImageHeight(g_folder), g_cat_list[i].name+4);
						}
					}
					g_temp_name[0]='\0';
				}
			}
			oslIntraFontSetStyle(g_pgf_font, 0.5, RGBA(255,255,255,255), RGBA(0,0,0,0), 0.0, INTRAFONT_ALIGN_LEFT);
			oslSetFont(g_pgf_font);

			oslEndDrawing();
		}
		oslEndFrame();
		skip = oslSyncFrame();

		oslReadKeys();
		if (osl_keys->pressed.down) {
			if (flag == 0 && selected < total - 1 ) {
				if (++selected > first + visible) {
					first++;
				}

			} else if (mode == MODE_TITLELIST && selected < g_titles_count - 1) {
				moveHBdown(selected, g_titles_list);
				if (++selected > first + visible) {
					first++;
				}

			} else if (mode == MODE_CATEGORIES && selected < g_cat_count - 1) {
				moveCATdown(selected, g_cat_list);
				if (++selected > first + visible) {
					first++;
				}
			}

		} else if (osl_keys->pressed.up) {
			if (flag == 0 && selected > 0) {
				if (--selected < first) {
					first--;
				}

			} else if (mode == MODE_TITLELIST && selected > 0) {
				moveHBup(selected, g_titles_list);
				if (--selected < first) {
					first--;
				}

			} else if (mode == MODE_CATEGORIES && selected > 0) {
				moveCATup(selected, g_cat_list);
				if (--selected < first) {
					first--;
				}
			}

		} else if (osl_keys->released.cross) {
			flag ^= 1;

		} else if (osl_keys->released.circle) {
			enable ^= 1;

		} else if (osl_keys->released.L || osl_keys->released.R) {
			if (mode == MODE_TITLELIST) {
				mode = MODE_CATEGORIES;
				hbSelected = selected;
				selected = catSelected;
				total = g_cat_count;
				hbFirst = first;
				first = catFirst;
			} else if (mode == MODE_CATEGORIES) {
				mode = MODE_TITLELIST;
				catSelected = selected;
				selected = hbSelected;
				total = g_titles_count;
				catFirst = first;
				first = hbFirst;
			}
			oldSelected = -1;

		} else if (osl_keys->released.start) {
			if (mode == MODE_TITLELIST) {
				saveTitlesList(g_titles_list, g_titles_count);
				if (g_is_browser_mode) {
					saveTitlesListBM(g_titles_list, g_titles_count);
				}

			} else if (mode == MODE_CATEGORIES) {
				saveCATlist(g_cat_list, g_cat_count);
			}

		} else if (osl_keys->released.triangle) {
			if (g_cat_count != 0) {
				return 1;
			}
		}
	}
	return 0;
}

/*  Prior menu showing categories */
static int priorMenu() {
	int skip = 0;
	int start = 27;
	int first = 0;
	int total = g_cat_count_norep+1;
	int visible = 13;
	int selected = 0;
	int oldSelected = -1;
	int i = 0;
	int enable = 1;

	while (!osl_quit) {
		if (!skip) {
			oslStartDrawing();
			oslDrawImageXY(g_bkg, 0, 0);
			drawToolbars(1);
			oslDrawFillRect(5,22,285,248,RGBA(0,0,0,170));
			oslDrawFillRect(290,22,475,60+40+13,RGBA(0,0,0,170));
			oslDrawImageXY(g_cross,305,30);
			oslDrawString(335,30,"Enter Category");
			oslDrawImageXY(g_circle,305,50);
			if (enable) {
				oslIntraFontSetStyle(g_pgf_font, 0.5, RGBA(255,255,255,100), RGBA(0,0,0,0), 0.0, INTRAFONT_ALIGN_LEFT);
				oslDrawString(335,50,"Hide icon0");
				oslIntraFontSetStyle(g_pgf_font, 0.5, RGBA(255,255,255,255), RGBA(0,0,0,0), 0.0, INTRAFONT_ALIGN_LEFT);
			} else {
				oslDrawString(335,50,"Show icon0");
			}
			oslDrawImageXY(g_square,305,70);
			if (g_is_browser_mode) {
				oslIntraFontSetStyle(g_pgf_font, 0.5, RGBA(255,255,255,100), RGBA(0,0,0,0), 0.0, INTRAFONT_ALIGN_LEFT);
				oslDrawString(335,70,"Disable browser mode");
				oslIntraFontSetStyle(g_pgf_font, 0.5, RGBA(255,255,255,255), RGBA(0,0,0,0), 0.0, INTRAFONT_ALIGN_LEFT);
			} else {
				oslDrawString(335,70,"Enable browser mode");
			}
			oslDrawImageXY(g_triangle,305,90);
			oslDrawString(335,90,"View All");

			//Draw menu:
			for (i = first; i <= first+visible; i++) {
				if (i == selected && i != 0) {
					oslIntraFontSetStyle(g_pgf_font, 0.5, RGBA(20,20,20,255), RGBA(255,255,255,200), 0.0, INTRAFONT_ALIGN_LEFT);
					oslSetFont(g_pgf_font);
					if (enable) {
						if (oldSelected != selected) {
							if (g_icon0!=NULL) {
								oslDeleteImage(g_icon0);
							}
							oldSelected = selected;
							getIcon0_fromfile(g_cat_list_norep[i-1].path);
						}

						if (g_icon0!=NULL) {
							oslDrawImageXY(g_icon0, 312,168);
						}
					}
				} else {
					oslIntraFontSetStyle(g_pgf_font, 0.5, RGBA(255,255,255,255), RGBA(0,0,0,0), 0.0, INTRAFONT_ALIGN_LEFT);
					oslSetFont(g_pgf_font);
				}

				//CAT
				if (i < total) {
					oslDrawImageXY(g_folder,12,start +(i - first)*oslGetImageHeight(g_folder));
					oslIntraFontSetStyle(g_pgf_font, 0.5, RGBA(0,0,0,255), RGBA(0,0,0,0), 0.0, INTRAFONT_ALIGN_LEFT);
					oslSetFont(g_pgf_font);
					oslDrawString(10+oslGetImageWidth(g_folder)/4,start +(i - first)*oslGetImageHeight(g_folder)+2, "C");
					if (i == selected) {
						oslIntraFontSetStyle(g_pgf_font, 0.5, RGBA(20,20,20,255), RGBA(255,255,255,200), 0.0, INTRAFONT_ALIGN_LEFT);
						oslSetFont(g_pgf_font);
					} else {
						oslIntraFontSetStyle(g_pgf_font, 0.5, RGBA(255,255,255,255), RGBA(0,0,0,0), 0.0, INTRAFONT_ALIGN_LEFT);
						oslSetFont(g_pgf_font);
					}
					if (i == 0) {
						oslDrawString(15+oslGetImageWidth(g_folder),start +(i - first)*oslGetImageHeight(g_folder), "Uncategorized");
					} else {
						oslDrawString(15+oslGetImageWidth(g_folder),start +(i - first)*oslGetImageHeight(g_folder), g_cat_list_norep[i-1].name+4);
					}

				}
			}
			oslIntraFontSetStyle(g_pgf_font, 0.5, RGBA(255,255,255,255), RGBA(0,0,0,0), 0.0, INTRAFONT_ALIGN_LEFT);
			oslSetFont(g_pgf_font);

			oslEndDrawing();
		}
		oslEndFrame();
		skip = oslSyncFrame();

		oslReadKeys();
		if (osl_keys->pressed.down && selected < total - 1) {
			if (++selected > first + visible) {
				first++;
			}

		} else if (osl_keys->pressed.up && selected > 0) {
			if (--selected < first) {
				first--;
			}

		} else if (osl_keys->released.cross) {
			//If selected Uncategorized. Flag should only be 1 here
			if (selected == 0) {
				g_titles_count = getTitlesList(g_titles_list, "All", 1);
			} else {
				g_titles_count = getTitlesList(g_titles_list, g_cat_list_norep[selected-1].name, 0);
			}
			mainMenu();
			g_titles_count = 0;
			oldSelected = -1;

		} else if (osl_keys->released.circle) {
			enable ^= 1;

		} else if (osl_keys->released.square) {
			g_is_browser_mode ^= 1;

		} else if (osl_keys->released.triangle) {
			g_titles_count = getTitlesList(g_titles_list, "All", 0);
			mainMenu();
			g_titles_count = 0;

		} else if (osl_keys->released.start) {

		} else if (osl_keys->released.L || osl_keys->released.R) {

		}
	}
	return 0;
}

static const OSL_VIRTUALFILENAME __image_ram_files[] = {
	{"ram:/Media/bkg.png", (void*)bkg_png, size_bkg_png, &VF_MEMORY},
	{"ram:/Media/start.png", (void*)start_png, size_start_png, &VF_MEMORY},
	{"ram:/Media/cross.png", (void*)cross_png, size_cross_png, &VF_MEMORY},
	{"ram:/Media/circle.png", (void*)circle_png, size_circle_png, &VF_MEMORY},
	{"ram:/Media/square.png", (void*)square_png, size_square_png, &VF_MEMORY},
	{"ram:/Media/triangle.png", (void*)triangle_png, size_triangle_png, &VF_MEMORY},
	{"ram:/Media/folder.png", (void*)folder_png, size_folder_png, &VF_MEMORY},
	{"ram:/Media/iso.png", (void*)iso_png, size_iso_png, &VF_MEMORY},
	{"ram:/Media/icon0.png", (void*)icon0_png, size_icon0_png, &VF_MEMORY},
	{"ram:/Media/R.png", (void*)R_png, size_R_png, &VF_MEMORY},
	{"ram:/Media/L.png", (void*)L_png, size_L_png, &VF_MEMORY}
};

static int initOSLib() {
	oslInit(0);
	oslInitGfx(OSL_PF_8888, 1);
	oslInitAudio();
	oslSetQuitOnLoadFailure(1);
	oslAddVirtualFileList((OSL_VIRTUALFILENAME*)__image_ram_files, oslNumberof(__image_ram_files));
	g_bkg = oslLoadImageFilePNG("ram:/Media/bkg.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
	g_startb = oslLoadImageFilePNG("ram:/Media/start.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
	g_cross = oslLoadImageFilePNG("ram:/Media/cross.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
	g_circle = oslLoadImageFilePNG("ram:/Media/circle.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
	g_square = oslLoadImageFilePNG("ram:/Media/square.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
	g_triangle = oslLoadImageFilePNG("ram:/Media/triangle.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
	g_folder = oslLoadImageFilePNG("ram:/Media/folder.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
	g_iso = oslLoadImageFilePNG("ram:/Media/iso.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
	g_R = oslLoadImageFilePNG("ram:/Media/R.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
	g_L = oslLoadImageFilePNG("ram:/Media/L.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);

	oslSetKeyAutorepeatInit(40);
	oslSetKeyAutorepeatInterval(10);
	oslIntraFontInit(INTRAFONT_CACHE_MED);
	g_pgf_font = oslLoadFontFile("flash0:/font/ltn0.pgf");
	oslIntraFontSetStyle(g_pgf_font, 0.5, RGBA(255,255,255,255), RGBA(0,0,0,0), 0.0, INTRAFONT_ALIGN_LEFT);
	oslSetFont(g_pgf_font);
	oslSetKeyAnalogToDPad(ANALOG_SENS);

	return 0;
}


int main() {
	logInit("ms0:/PSP/GAME/TITLESORTER/log.txt");
	logmsg("Title Sorter started...\n");

	initOSLib();
	tzset();
	g_titles_count = getTitlesList(g_titles_list, "All", 0);
	g_cat_count = getCATList(g_cat_list);
	g_cat_count_norep = checkCATList(g_cat_list, g_cat_list_norep);

	if (g_cat_count == 0) {
		mainMenu();
	} else {
		g_titles_count = 0;
		priorMenu();
	}

	oslEndGfx();
	oslQuit();
	return 0;
}

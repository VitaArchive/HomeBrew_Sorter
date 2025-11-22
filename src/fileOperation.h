#define MAX_HB 500
#define MAX_CAT 150

// Categories lite says that a category name must be of 30 character
#define MAX_CAT_PATH 32
// Max path size for full named thing
#define MAX_PATH 256

enum HomebrewType {
	HB_EBOOT = 0,
	HB_ISO = 1,
};

typedef struct homebrew {
    char name[MAX_PATH];
    char path[MAX_PATH];
	char category[MAX_CAT_PATH];
    struct ScePspDateTime dateModify;
    char dateForSort[21];
	short int type;
} Homebrew;

typedef struct categories {
	char name[MAX_CAT_PATH];
	char path[MAX_PATH];
	struct ScePspDateTime dateModify;
	char dateForSort[21];
	int repeated;//this stores if a category is repeated for merging it.
} Categories;

int getHBList(Homebrew *HBlist, char *category, int flag);
int moveHBup(int index, Homebrew *HBlist);
int moveHBdown(int index, Homebrew *HBlist);
int saveHBlist(Homebrew *HBlist, int HBcount);
int saveHBlistBM(Homebrew *HBlist, int HBcount);//For browser mode, modifies eboot.pbp

int getCATList(Categories *CAT);
int checkCATList(Categories *CAT, Categories *CAT_norep);
int moveCATup(int index, Categories *CATlist);
int moveCATdown(int index, Categories *CATlist);
int saveCATlist(Categories *CATlist, int CATcount);

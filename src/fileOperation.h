#define MAX_HB 500
#define MAX_CAT 500

typedef struct homebrew {
    char name[262];
    char path[262];
	char category[262];
    struct ScePspDateTime dateModify;
    char dateForSort[21];
	short int type;
} Homebrew;

typedef struct categories {
	char name[262];
	char path[262];
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

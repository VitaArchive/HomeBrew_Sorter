#define MAX_HB 500
#define MAX_CAT 150

// Categories lite says that a category name must be of 30 character
#define MAX_CAT_PATH 32
// Max path size for full named thing
#define MAX_PATH 256

enum TitleType {
	TITLE_EBOOT = 0,
	TITLE_ISO = 1,
};

typedef struct homebrew {
    char name[MAX_PATH];
    char path[MAX_PATH];
	char category[MAX_CAT_PATH];
    struct ScePspDateTime dateModify;
    char dateForSort[21];
	short int type;
} Title;

typedef struct categories {
	char name[MAX_CAT_PATH];
	char path[MAX_PATH];
	struct ScePspDateTime dateModify;
	char dateForSort[21];
	int repeated;//this stores if a category is repeated for merging it.
} Categories;

int getTitlesList(Title *HBlist, char *category, int flag);
int moveHBup(int index, Title *HBlist);
int moveHBdown(int index, Title *HBlist);
int saveTitlesList(Title *HBlist, int HBcount);
int saveTitlesListBM(Title *HBlist, int HBcount);//For browser mode, modifies eboot.pbp

int getCATList(Categories *CAT);
int checkCATList(Categories *CAT, Categories *CAT_norep);
int moveCATup(int index, Categories *CATlist);
int moveCATdown(int index, Categories *CATlist);
int saveCATlist(Categories *CATlist, int CATcount);

#include "file-table.h"
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

int get_files_in_dir(char *dir, FILE_TABLE **files) {
  DIR *dp;
  struct dirent *ep;
  FILE_TABLE *_files;
  dp = opendir(dir);
  if (dp == NULL)
    return -1;

  _files = NULL;
  while (ep = readdir(dp)) {
    if (ep->d_type == DT_REG) {
      FILE_TABLE *new = malloc(sizeof(FILE_TABLE));
      new->file = calloc(strlen(ep->d_name) + 1, sizeof(char));
      strcpy(new->file, ep->d_name);
      new->next = _files;
      _files = new;
    }
  }
  *files = _files;
  (void)closedir(dp);
  return 0;
}

#ifndef FILE_TABLE_H
#define FILE_TABLE_H

typedef struct file_table {
  char *file;
  struct file_table *next;
} FILE_TABLE;

int get_files_in_dir(char *dir, FILE_TABLE **files);

#endif

#ifndef __FILE_SAVE_H__
#define  __FILE_SAVE_H__
int open_file(const char* file_name);

void close_file();

int write_file(const char* buf,int len);
#endif

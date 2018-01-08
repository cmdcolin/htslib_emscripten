#include <map>

#include "hts_js.h"
#include "hfile_js.h"

typedef std::map<std::string, htsFile*> file_map;

extern file_map htsFiles;

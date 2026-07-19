#ifndef __UTILS_H__
#define __UTILS_H__

#include <cstdlib>
#include <sys/stat.h>
#include <string>
#include <climits>
#include <sstream>
#include <vector>
#include <filesystem>

inline bool fileExists(const char *filename) {
  struct stat buffer;
  return (stat(filename, &buffer) == 0);
}

inline std::vector<std::string> tokenizeString(std::string path, char delimiter = '/') {
  std::istringstream ss(path);
  std::string token;
  std::vector<std::string> parts;
  while(std::getline(ss, token, delimiter)) {
    parts.push_back(token);
  }
  return parts;
}

inline bool getRunNumber(const std::string& infile,int& run,int& subrun) {
  const std::string stem = std::filesystem::path(infile).stem().string();

  run = -1;
  subrun = -1;

  // Example: hist1102-00.root
  if(std::sscanf( stem.c_str(), "hist%d-%d", &run, &subrun) == 2) {
    return true;
  }

  // Example: hist1102.root
  if(std::sscanf( stem.c_str(),"hist%d",&run) == 1) {
    subrun = -1;
    return true;
  }

  return false;
}


#ifdef __LINUX__
#include <unistd.h>
inline std::string programPath(){
  char buff[PATH_MAX+1];
  size_t len = readlink("/proc/self/exe", buff, sizeof(buff)-1);
  buff[len] = '\0';

  std::string exe_path = buff;
  return exe_path.substr(0, exe_path.find_last_of('/'));
}
#endif

#ifdef __DARWIN__ 
#include <mach-o/dyld.h>
inline std::string programPath(){
  char buff[PATH_MAX];
  uint32_t len = PATH_MAX;
  _NSGetExecutablePath(buff,&len);
  std::string exe_path = buff;
  return exe_path.substr(0, exe_path.find_last_of('/'));
}
#endif

#endif
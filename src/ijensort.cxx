#include <cstdio>
#include <string>
#include <utils.h>
#include <TEnv.h>

#include <Ijen.h>


void loadEnv() {
  setenv("IJENSORTSYS", (programPath()+"/..").c_str(), 0);

  printf("IJENSORTSYS has been set to: %s\n",getenv("IJENSORTSYS"));

  // Load $GSORTSYS/.gsortrc
    std::string ijensortrc_path = Form("%s/.ijensortrc",getenv("IJENSORTSYS"));
    gEnv->ReadFile(ijensortrc_path.c_str(),kEnvChange);

    ijensortrc_path = Form("%s/.ijensortrc",getenv("HOME"));
    if(fileExists(ijensortrc_path.c_str())){
        gEnv->ReadFile(ijensortrc_path.c_str(),kEnvChange);
    }

    ijensortrc_path = Form("%s/.ijensortrc",getenv("PWD"));
    if(fileExists(ijensortrc_path.c_str())){
        gEnv->ReadFile(ijensortrc_path.c_str(),kEnvChange);
    }
}


int main (int argc, char **argv) {

//  ROOT::EnableThreadSafety();

  loadEnv();
  //gROOT->ProcessLine(".L include/Gtypes.h"); // for some reason the enums aren't being processed properly by the rootclang

 //  Gint::Get(argc,argv)->Run(true);
  Ijen::Get(argc,argv)->Run(true);
 
  return 0;
}
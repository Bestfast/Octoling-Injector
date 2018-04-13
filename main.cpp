#include <dirent.h>
#include <iostream>
#include <fstream>
#include <boost/crc.hpp>  
#include <switch.h>

#ifndef PRIVATE_BUFFER_SIZE
#define PRIVATE_BUFFER_SIZE  1024
#endif


std::streamsize const  buffer_size = PRIVATE_BUFFER_SIZE;
typedef unsigned char byte;

using namespace std;

int main(int argc, char **argv)
{
    Result rc=0;
    int ret=0;

    DIR* dir;

    FsFileSystem tmpfs;
    u128 userID=0;
    bool account_selected=0;
    u64 titleid=0x01003BC0000A0000; // TID of the selected game

    gfxInitDefault();
    consoleInit(NULL);

    rc = accountInitialize();
    if (R_FAILED(rc)) {
        printf("accountInitialize() failed: 0x%x\n", rc);
    }

    if (R_SUCCEEDED(rc)) {
        rc = accountGetActiveUser(&userID, &account_selected);
        accountExit();

        if (R_FAILED(rc)) {
            printf("accountGetActiveUser() failed: 0x%x\n", rc);
        }
        else if(!account_selected) {
            printf("No user is currently selected.\n");
            rc = -1;
        }
    }

    if (R_SUCCEEDED(rc)) {
        rc = fsMount_SaveData(&tmpfs, titleid, userID);//See also libnx fs.h.
        if (R_FAILED(rc)) {
            printf("fsMount_SaveData() failed: 0x%x\n", rc);
        }
    }

    if (R_SUCCEEDED(rc)) {
        ret = fsdevMountDevice("save", tmpfs);
        if (ret==-1) {
            printf("fsdevMountDevice() failed.\n");
            rc = ret;
        }
    }


    if (R_SUCCEEDED(rc)) {
        dir = opendir("save:/");
        if(dir==NULL)
        {
            printf("Failed to open dir.\n");
        }
        else
        {
            printf("Modifying the save game... ");
            FILE * pFile;
            pFile = fopen("save.dat", "rb+");
            byte octoling = 0x4;
            fseek (pFile , 0x10 , SEEK_SET);
            fwrite (&octoling , sizeof(char), sizeof(octoling), pFile);
            byte hair = 0x64;
            fseek (pFile , 0x12 , SEEK_SET);
            fwrite (&hair , sizeof(char), sizeof(hair), pFile);
            fclose (pFile);
            boost::crc_32_type  result;

            std::ifstream  ifs( "save.dat", std::ios_base::binary );
            if ( ifs )
            {
                do
                {
                    char  buffer[ buffer_size ];
                    ifs.seekg (0x10, ifs.beg);
                    ifs.read( buffer, buffer_size );
                    result.process_bytes( buffer, ifs.gcount() );
                } while ( ifs );
            }
            unsigned int buf[] = {result.checksum()};
            fseek ( pFile , 0x8 , SEEK_SET);
            fwrite (buf , sizeof(char), sizeof(buf), pFile);
            rc = fsdevCommitDevice("save");
            printf("Done!");
        }
        fsdevUnmountDevice("save");
    }

    while(appletMainLoop())
    {
        hidScanInput();

        u32 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS) break;

        gfxFlushBuffers();
        gfxSwapBuffers();
        gfxWaitForVsync();
    }

    gfxExit();
    return 0;
}


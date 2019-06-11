#ifndef H_PAK_IMPORTER
#define H_PAK_IMPORTER

#include <KAO2_PAK/ConsoleApp.h>


////////////////////////////////////////////////////////////////
// PAK IMPORTER CLASS
////////////////////////////////////////////////////////////////

enum ParseOptions
{
    GET_STREAM_NAME,
    GET_DIRECTORY,
    GET_NEXT_FILE
};


class PakImporter
{
    public:

    /*** Properties ***/

        std::string MediaDir;

        char StreamName[48];

        int32_t BlocksCount;
        char Languages[8][4];
        int32_t BlockSizes[9];
        int32_t ItemsCount[9];

        std::ofstream PakFile;
        std::ifstream ItemFile;
        uint8_t* Data;

        std::ifstream ListFile;
        std::string ListName;

    /*** Methods ***/

        void trimLine(std::string &line);

        bool parseList(std::string &result, int what);

        void closeBlock();

        bool readItem(int32_t &filesize, char* filename);

        bool createArchive();

        bool importData();

        PakImporter(char* log);

        ~PakImporter();
};

#endif

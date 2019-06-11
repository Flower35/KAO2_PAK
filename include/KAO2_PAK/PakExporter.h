#ifndef H_PAK_EXPORTER
#define H_PAK_EXPORTER

#include <KAO2_PAK/ConsoleApp.h>


////////////////////////////////////////////////////////////////
// PAK EXPORTER CLASS
////////////////////////////////////////////////////////////////

class PakExporter
{
    public:

    /*** Properties ***/

        std::string OutputDir;

        std::string PakName;
        char StreamName[48];

        int32_t ItemsCount;
        int32_t LanguagesCount;
        char Languages[8][4];
        int32_t BlockSizes[9];

        std::ifstream PakFile;
        std::ofstream ItemFile;
        uint8_t* Data;

        std::ofstream LogFile;
        bool SaveLog;

    /*** Methods ***/

        bool saveItem(int32_t filesize, char* filename);

        bool exportItemBlock(int32_t end_offset, int sector, int &id);

        bool exportArchive();

        bool checkStreamSize(int32_t size);

        bool openAndCheckArchive();

        bool checkPakFileExtension();

        void getPakFilenameFromPath();

        PakExporter(char* pak, char* directory, bool log);

        ~PakExporter();
};

#endif

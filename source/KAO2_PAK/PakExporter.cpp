#include <KAO2_PAK/PakExporter.h>


////////////////////////////////////////////////////////////////
// PAK EXPORTER - initialize
////////////////////////////////////////////////////////////////

PakExporter::PakExporter(char* pak, char* directory, bool log)
{
    /* Reset data */

    std::memset(StreamName, 0x00, sizeof(StreamName));
    ItemsCount = 0;
    LanguagesCount = 0;
    std::memset(Languages, 0x00, sizeof(Languages));
    std::memset(BlockSizes, 0x00, sizeof(BlockSizes));

    /* Save paths */

    OutputDir = directory;
    PakName = pak;

    SaveLog = log;
}


////////////////////////////////////////////////////////////////
// PAK EXPORTER - close
////////////////////////////////////////////////////////////////

PakExporter::~PakExporter()
{
    if (PakFile.is_open())
    {
        PakFile.close();
    }

    if (ItemFile.is_open())
    {
        ItemFile.close();
    }

    if (LogFile.is_open())
    {
        LogFile.close();
    }

    if (nullptr != Data)
    {
        delete[](Data);
    }
}

////////////////////////////////////////////////////////////////
// Check PAK archive file extension
////////////////////////////////////////////////////////////////

bool PakExporter::checkPakFileExtension()
{
    int l = PakName.length();

    /* At least one char, dot and PAK ext. */

    if (l >= (1 + 1 + 3))
    {
        return
        (
            ('.' == PakName.at(l - 4))
            && ('p' == tolower(PakName.at(l - 3)))
            && ('a' == tolower(PakName.at(l - 2)))
            && ('k' == tolower(PakName.at(l - 1)))
        );
    }

    return false;
}


////////////////////////////////////////////////////////////////
// Get PAK archive filename + check output directory
////////////////////////////////////////////////////////////////

void PakExporter::getPakFilenameFromPath()
{
    std::string temp = PakName;

    int c1 = PakName.rfind('/') + 1;
    int c2 = PakName.rfind('\\') + 1;
    PakName = PakName.substr(c1 > c2 ? c1 : c2);

    if (checkPakFileExtension())
    {
        PakName = PakName.substr(0, (PakName.length() - 4));
    }

    if (OutputDir.length() <= 0)
    {
        OutputDir = temp.substr(0, c1 > c2 ? c1 : c2) + PakName + '\\';

        _mkdir(OutputDir.c_str());
    }
}


////////////////////////////////////////////////////////////////
// Validate stream size
////////////////////////////////////////////////////////////////

bool PakExporter::checkStreamSize(int32_t size)
{
    int32_t test = 0;

    for (int i = 0; i < (LanguagesCount + 1); i++)
    {
        if (0 != (BlockSizes[i] % 65536))
        {
            return false;
        }

        test += BlockSizes[i];
    }

    return ((size == BlockSizes[0]) || (size == test));
}


////////////////////////////////////////////////////////////////
// Open and check PAK archive
////////////////////////////////////////////////////////////////

bool PakExporter::openAndCheckArchive()
{
    int32_t size;
    int32_t test;

    /* Try to open the PAK archive */

    PakFile.open(PakName, std::ios::in | std::ios::binary | std::ios::ate);

    if (!PakFile.is_open())
    {
        std::cout << "\n [ERROR] Cannot open this file:"
            << "\n * \"" << PakName << "\""
            << "\n";

        return false;
    }

    /* Read file size and reset file pointer */

    size = (int32_t)PakFile.tellg();
    PakFile.seekg(0);

    /* Check PAK header */

    PakFile.read((char*)&test, 0x04);

    if (*(int32_t*)"TATE" != test)
    {
        std::cout << "\n [ERROR] Incorrect PAK header! (expected: \"TATE\")"
            << "\n";

        return false;
    }

    /* Read basic archive info */

    PakFile.read((char*)&(BlockSizes[0]), 0x04);
    PakFile.read((char*)&ItemsCount, 0x04);
    PakFile.read((char*)&LanguagesCount, 0x04);

    if (LanguagesCount > 8)
    {
        std::cout << "\n [ERROR] Too many language blocks! (max 8)"
            << "\n";

        return false;
    }

    /* Read and check stream name */

    PakFile.read(StreamName, 48);

    getPakFilenameFromPath();

    if (0 != PakName.compare(StreamName))
    {
        std::cout << "\n [WARNING] \"StreamName\" does not match the file name!"
            << "\n";
    }

    /* Read language info */

    for (test = 0; test < LanguagesCount; test++)
    {
        PakFile.read(Languages[test], 0x04);
        PakFile.read((char*)&(BlockSizes[1 + test]), 0x04);
    }

    /* Validate stream size */

    if (!checkStreamSize(size))
    {
        std::cout << "\n [ERROR] Incorrect PAK file size"
            << "(or misaligned block size)!"
            << "\n";

        return false;
    }

    /* Write info to console window */

    std::cout << "\n --------------------------------"
        << "\n * " << StreamName
        << "\n * Output folder: \"" << OutputDir << "\""
        << "\n";

    /* Try to open log file */

    if (SaveLog)
    {
        std::string path = OutputDir + StreamName + ".log";

        LogFile.open(path.c_str(), std::ios::out | std::ios::trunc);

        if (!LogFile.is_open())
        {
            std::cout << "\n [WARNING] Cannot create this file:"
                << "\n * \"" << path << "\""
                << "\n";

            SaveLog = false;
        }
        else
        {
            std::cout << " * Log file: \"" << path << "\""
                << "\n";

            LogFile << "STREAM: " << StreamName << "\n"
                << "FOLDER: " << OutputDir << "\n";
        }
    }

    return true;
}


////////////////////////////////////////////////////////////////
// Export whole PAK archive
////////////////////////////////////////////////////////////////

bool PakExporter::exportArchive()
{
    int32_t current_item = 0;
    int32_t main_sector_last_item = 0;
    int32_t end_offset = BlockSizes[0];

    /* Switch to offset 0x80 and start working with the first sector */

    std::cout << "\n Extracting archive..."
        << "\n";

    if (SaveLog)
    {
        LogFile << "\n -tate"
            << "\n";
    }

    PakFile.seekg(0x80);

    if (!exportItemBlock(end_offset, 0, main_sector_last_item))
    {
        return false;
    }

    /* Continue exporting files from language blocks */

    for (int i = 0; i < LanguagesCount; i++)
    {
        std::cout << "\n -lang=" << Languages[i]
            << "\n";

        if (SaveLog)
        {
            LogFile << "\n -lang=" << Languages[i]
                << "\n";
        }

        PakFile.seekg(end_offset);
        end_offset += BlockSizes[i + 1];

        current_item = main_sector_last_item;

        if (!exportItemBlock(end_offset, (i + 1), current_item))
        {
            return false;
        }
    }

    /* The end :) */

    return true;
}


////////////////////////////////////////////////////////////////
// Collecting items from a block
////////////////////////////////////////////////////////////////

bool PakExporter::exportItemBlock(int32_t end_offset, int sector, int &id)
{
    int32_t test;
    int32_t offset;

    int32_t item_size;
    char item_name[0x70];

    while ((offset = (int32_t)PakFile.tellg()) < end_offset)
    {
        /* Align stream offset to a 0x80 byte block */

        if (test = (offset % 128))
        {
            PakFile.seekg((128 - test) + offset);
        }

        if (PakFile.tellg() >= end_offset)
        {
            return true;
        }

        /* Read item magic */

        PakFile.read((char*)&test, 0x04);

        if (*(int32_t*)"item" != test)
        {
            if (((id == ItemsCount) && ((LanguagesCount <= 0) || (sector > 0))) || (0 == test))
            {
                /* Empty bytes at the end of any block */

                /* If it is the first sector, then ID is smaller than ItemCount in multilanguage versions */

                return true;
            }
            else
            {
                std::cout << " [ERROR] Expected item magic at offset 0x"
                    << std::hex << ((int)PakFile.tellg() - 4)
                    << "\n";

                return false;
            }
        }

        /* Read item size */

        PakFile.read((char*)&item_size, 0x04);

        /* This check is not done in any Kao2/Kao3 engine, but I make it for clarity */

        PakFile.read((char*)&test, 0x04);

        if (id != test)
        {
            std::cout << "\n [ERROR] item ID is incorrect! (stream: " << test << ", expected: " << id << ")"
                << "\n";

            return false;
        }

        /* Read item name */

        PakFile.seekg(0x04, std::ios::cur);
        PakFile.read(item_name, 0x70);
        item_name[0x70 - 1] = 0x00;

        /* Export data */
        if (!saveItem(item_size, item_name))
        {
            return false;
        }

        /* Advance the ID counter */
        id++;
    }

    /* "Kurwa dosz³o tutaj i nic siê nie wybra³o. Co ty na to? Zobacz poni¿ej. Mo¿e to uczyni Ciê m¹drzejszym." */

    return true;
}


////////////////////////////////////////////////////////////////
// Save a single file to your hard drive or something
////////////////////////////////////////////////////////////////

bool PakExporter::saveItem(int32_t filesize, char* filename)
{
    std::string path = OutputDir + filename;
    char temp;

    /* Write info to console */

    std::cout << filename << "\n";

    if (SaveLog)
    {
        LogFile << filename << "\n";
    }

    /* Try to allocate memory for this item */

    if (nullptr != Data)
    {
        delete[](Data);
    }

    try
    {
        Data = new uint8_t [filesize];
    }
    catch (std::bad_alloc)
    {
        std::cout << "\n [ERROR] Could not allocate " << filesize << " bytes!"
            << "\n";

        return false;
    }

    /* Read item data */

    PakFile.read((char*)Data, filesize);

    /* Create output directories */

    for (char* p = (char*)path.c_str(); (*p); p++)
    {
        if (((*p) == '/') || ((*p) == '\\'))
        {
            temp = (*p);
            (*p) = 0x00;

            _mkdir(path.c_str());

            (*p) = temp;
        }
    }

    /* Open file for saving */

    if (ItemFile.is_open())
    {
        ItemFile.close();
    }

    ItemFile.open(path, std::ios::out | std::ios::binary | std::ios::trunc);

    if (!ItemFile.is_open())
    {
        std::cout << "\n [ERROR] Could not create file:"
            << "\n\"" << path << "\""
            << "\n";

        return false;
    }

    /* Save data and close item */

    ItemFile.write((char*)Data, filesize);

    ItemFile.close();

    delete[](Data);
    Data = nullptr;

    return true;
}

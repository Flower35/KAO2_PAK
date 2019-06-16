#include <KAO2_PAK/PakImporter.h>


////////////////////////////////////////////////////////////////
// PAK IMPORTER - initialize
////////////////////////////////////////////////////////////////

PakImporter::PakImporter(char* log)
{
    /* Reset data */

    std::memset(StreamName, 0x00, sizeof(StreamName));
    BlocksCount = (-1);
    std::memset(Languages, 0x00, sizeof(Languages));
    std::memset(BlockSizes, 0x00, sizeof(BlockSizes));
    std::memset(ItemsCount, 0x00, sizeof(ItemsCount));
    Data = nullptr;

    /* Save file path */

    ListName = log;
}


////////////////////////////////////////////////////////////////
// PAK IMPORTER - close
////////////////////////////////////////////////////////////////

PakImporter::~PakImporter()
{
    if (PakFile.is_open())
    {
        PakFile.close();
    }

    if (ItemFile.is_open())
    {
        ItemFile.close();
    }

    if (nullptr != Data)
    {
        delete[](Data);
    }
}


////////////////////////////////////////////////////////////////
// Trim line (delete special characters on both ends)
////////////////////////////////////////////////////////////////

void PakImporter::trimLine(std::string &line)
{
    int start = 0;
    int end = (line.length() - 1);

    if (end >= 0)
    {
        while ((start < end) && (line.at(start) <= 0x20))
        {
            start++;
        }

        while ((end > start) && (line.at(end) < 0x20))
        {
            end--;
        }

        line = line.substr(start, (end - start + 1));
    }
}


////////////////////////////////////////////////////////////////
// Parse files list
////////////////////////////////////////////////////////////////

bool PakImporter::parseList(std::string &result, int what)
{
    int32_t test;

    switch(what)
    {
        case ParseOptions::GET_STREAM_NAME:
        {
            ListFile.seekg(0);

            while (!ListFile.eof())
            {
                std::getline(ListFile, result);

                trimLine(result);

                if (0 == result.compare(0, 7, "STREAM:"))
                {
                    result = result.substr(7);
                    trimLine(result);

                    return true;
                }
            }

            return false;
        }

        case ParseOptions::GET_DIRECTORY:
        {
            ListFile.seekg(0);

            while (!ListFile.eof())
            {
                std::getline(ListFile, result);

                trimLine(result);

                if (0 == result.compare(0, 7, "FOLDER:"))
                {
                    result = result.substr(7);
                    trimLine(result);

                    return true;
                }
            }

            return false;
        }

        case ParseOptions::GET_NEXT_FILE:
        {
            if (BlocksCount < 0)
            {
                /* Search for the first file on the list */

                ListFile.seekg(0);

                while (!ListFile.eof())
                {
                    std::getline(ListFile, result);

                    trimLine(result);

                    if (0 == result.compare("-tate"))
                    {
                        BlocksCount = 0;

                        result = "";
                        return true;
                    }
                }

                return false;
            }
            else
            {
                /* Finish checking list if ending was reached */

                if (ListFile.eof())
                {
                    return false;
                }

                /* Check if it is a new language block or just a filename */

                std::getline(ListFile, result);

                trimLine(result);

                if (0 == result.compare(0, 6, "-lang="))
                {
                    if (BlocksCount >= 8)
                    {
                        std::cout << "\n [ERROR] Too many languages defined! Max 8."
                            << "\n";

                        return false;
                    }

                    result = result.substr(6);
                    trimLine(result);

                    test = result.length();
                    test = (test > 4) ? 4 : test;
                    std::memcpy(Languages[BlocksCount], result.c_str(), test);

                    for (int i = 0; i < BlocksCount; i++)
                    {
                        if (0 == std::memcmp(Languages[i], Languages[BlocksCount], 0x04))
                        {
                            std::cout << "\n [ERROR] Language \"" << result << "\" already defined!"
                                << "\n";

                            return false;
                        }
                    }

                    /* Close previous item block and advance block counter */

                    closeBlock();

                    std::cout << "\n -lang=" << Languages[BlocksCount]
                        << "\n";

                    BlocksCount++;
                    ItemsCount[BlocksCount] = ItemsCount[0];

                    result = "";
                    return true;
                }
                else
                {
                    return true;
                }
            }

            return false;
        }

        default:
        {
            std::cout << "\n [ERROR] Invalid parser argument."
                << "\n";

            return false;
        }
    }
}


////////////////////////////////////////////////////////////////
// Close item block
////////////////////////////////////////////////////////////////

void PakImporter::closeBlock()
{
    int32_t previous_size = 0;
    int32_t current_size;
    int32_t next_size;

    char empty[128];
    std::memset(empty, 0x00, 128);

    for (int32_t i = 0; i < BlocksCount; i++)
    {
        previous_size += BlockSizes[i];
    }

    /* Allign to 65536 bytes */

    current_size = (int32_t)PakFile.tellp();

    next_size = (current_size % 65536);

    if (0 != next_size)
    {
        next_size = current_size + 65536 - next_size;

        while (current_size < next_size)
        {
            PakFile.write(empty, 128);
            current_size += 128;
        }

        current_size = next_size - current_size;

        if (current_size > 0)
        {
            PakFile.write(empty, current_size);
        }
    }

    /* Save block size */

    BlockSizes[BlocksCount] = (next_size - previous_size);
}


////////////////////////////////////////////////////////////////
// Create a brand new PAK archive
////////////////////////////////////////////////////////////////

bool PakImporter::createArchive()
{
    int32_t test;
    std::string temp_str;

    char empty[64];
    std::memset(empty, 0x00, 64);

    /* Try to open file list */

    ListFile.open(ListName, std::ios::in);

    if (!ListFile.is_open())
    {
        std::cout << "\n [ERROR] Cannot open this file:"
            << "\n * \"" << ListName << "\""
            << "\n";

        return false;
    }

    /* Get stream name and input/output directory */

    if (!parseList(temp_str, ParseOptions::GET_STREAM_NAME))
    {
        std::cout << "\n [ERROR] Could not find \"STREAM:\" line in list file!"
            << "\n";

        return false;
    }

    test = temp_str.length();
    test = (test > 48) ? 48 : test;
    std::memcpy(StreamName, temp_str.c_str(), test);

    if (!parseList(MediaDir, ParseOptions::GET_DIRECTORY))
    {
        std::cout << "\n [ERROR] Could not find \"FOLDER:\" line in list file!"
            << "\n";

        return false;
    }

    if ((MediaDir.back() != '/') && (MediaDir.back() != '\\'))
    {
        MediaDir += '\\';
    }

    /* Try to open the PAK archive */

    temp_str = MediaDir + StreamName + ".pak";

    PakFile.open(temp_str, std::ios::out | std::ios::binary | std::ios::trunc);

    if (!PakFile.is_open())
    {
        std::cout << "\n [ERROR] Cannot create this file:"
            << "\n * \"" << temp_str << "\""
            << "\n";

        return false;
    }

    /* Write header */

    PakFile.write("TATE", 0x04);
    PakFile.write(empty, (3 * 0x04));
    PakFile.write(StreamName, 48);

    /* Write empty language info and fill 128 bytes */

    PakFile.write(empty, 64);

    if (0x80 != PakFile.tellp())
    {
        std::cout << "\n [ERROR] Incorrect PAK file pointer!"
            << "\n";

        return false;
    }

    /* Write info to console window */

    std::cout << "\n --------------------------------"
        << "\n * " << StreamName
        << "\n * Output folder: \"" << MediaDir << "\""
        << "\n";

    return true;
}


////////////////////////////////////////////////////////////////
// Import files into PAK archive
////////////////////////////////////////////////////////////////

bool PakImporter::importData()
{
    int32_t offset[2];
    int32_t test;

    std::string temp_str;

    char empty[128];
    std::memset(empty, 0x00, 128);

    std::cout << "\n Importing data..."
        << "\n";

    while (parseList(temp_str, ParseOptions::GET_NEXT_FILE))
    {
        if (temp_str.length() > 0)
        {
            /* Add new item */

            PakFile.write("item", 0x04);

            offset[0] = (int32_t)PakFile.tellp();
            PakFile.write(empty, 0x04);

            PakFile.write((char*)&(ItemsCount[BlocksCount]), 0x04);

            PakFile.write(empty, 0x04);

            test = temp_str.length();
            test = (test > 0x70) ? 0x70 : test;
            PakFile.write(temp_str.c_str(), test);

            test = 0x70 - test;
            if (test > 0)
            {
                PakFile.write(empty, test);
            }

            if (!readItem(test, (char*)temp_str.c_str()))
            {
                return false;
            }

            /* Save item size and align PAK pointer */

            offset[1] = (int32_t)PakFile.tellp();

            PakFile.seekp(offset[0]);
            PakFile.write((char*)&test, 0x04);

            PakFile.seekp(offset[1]);
            test = (test % 128);
            if (0 != test)
            {
                test = 128 - test;

                PakFile.write(empty, test);
            }

            /* Increase items counter */

            ItemsCount[BlocksCount]++;
        }
    }

    /* Close last block and check items counters */
    /* Note: `BlocksCount` shall now be equal to number of Language Blocks (so 0 for no languages) */

    closeBlock();

    for (test = 0; test < BlocksCount; test++)
    {
        ItemsCount[test] -= ItemsCount[0];
    }

    for (int i = 2; i <= BlocksCount; i++)
    {
        if (ItemsCount[1] != ItemsCount[i])
        {
            std::cout << "\n [ERROR] Items count mismatch:"
                << "\n -lang=" << Languages[0] << ": " << ItemsCount[1] << "files"
                << "\n -lang=" << Languages[i - 1] << ": " << ItemsCount[i] << "files"
                << "\n";

            return false;
        }
    }

    /* Complete archive header */

    test = ItemsCount[0] + ItemsCount[1];

    PakFile.seekp(0x04);
    PakFile.write((char*)&(BlockSizes[0]), 0x04);
    PakFile.write((char*)&test, 0x04);
    PakFile.write((char*)&BlocksCount, 0x04);

    PakFile.seekp(0x40);
    for (test = 0; test < BlocksCount; test++)
    {
        PakFile.write(Languages[test], 0x04);
        PakFile.write((char*)&(BlockSizes[1 + test]), 0x04);
    }

    /* The end :) */

    PakFile.close();

    return true;
}


////////////////////////////////////////////////////////////////
// Copy a single file content
////////////////////////////////////////////////////////////////

bool PakImporter::readItem(int32_t &filesize, char* filename)
{
    std::string path = MediaDir + filename;

    /* Write info to console */

    std::cout << filename << "\n";

    /* Try to open new file */

    if (ItemFile.is_open())
    {
        ItemFile.close();
    }

    ItemFile.open(path, std::ios::in | std::ios::binary | std::ios::ate);

    if (!ItemFile.is_open())
    {
        std::cout << "\n [ERROR] Cannot open this file:"
            << "\n * \"" << path << "\""
            << "\n";

        return false;
    }

    /* Get file size and reset file pointer */

    filesize = (int32_t)ItemFile.tellg();
    ItemFile.seekg(0);

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

    /* Read item data and close file */

    ItemFile.read((char*)Data, filesize);
    ItemFile.close();

    /* Save data to archive */

    PakFile.write((char*)Data, filesize);

    delete[](Data);
    Data = nullptr;

    return true;
}

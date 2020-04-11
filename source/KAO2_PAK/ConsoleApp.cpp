#include <KAO2_PAK/PakExporter.h>
#include <KAO2_PAK/PakImporter.h>

int main(int argc, char** argv)
{
    std::string common_path;
    std::string answer;
    bool test = false;
    bool no_errors = true;

    /* Welcome! */

    std::cout << "\n < KAO2_PAK >"
        << "\n Simple utility to extract files from"
        << "\n \"Kao the Kangaroo: Round 2\" PAK archives."
        << "\n";

    if (argc < 2)
    {
        std::cout << "\n Please close this window and drop"
            << "\n one or more of KAO2 PAK archives"
            << "\n onto this executable!"
            << "\n (press any key to close)"
            << "\n";

        std::cin.ignore();
        return (-1);
    }

    /* Get ready */

    std::cout << "\n Choose an option:"
        << "\n 1) passed PAK archives as arguments [UNPACK FILES]"
        << "\n 2) passed LOG lists of files as arguments [REPACK ARCHIVES]"
        << "\n >> ";

    std::getline(std::cin, common_path);

    switch (std::stoi(common_path))
    {
        case 1:
        {
            std::cout << "\n Please write a common path for all files"
                << "\n or leave empty for separate directories."
                << "\n >> ";

            std::getline(std::cin, common_path);

            if (common_path.length() > 0)
            {
                if ((common_path.back() != '/') && (common_path.back() != '\\'))
                {
                    common_path += '\\';
                }
            }

            /* Ask another question */

            std::cout << "\n Do you want to create LOG files"
                << "\n for repacking? [Y - yes]"
                << "\n >> ";

            std::getline(std::cin, answer);

            if (answer.length() > 0)
            {
                if ('y' == tolower(answer.at(0)))
                {
                    test = true;
                }
            }

            /* Iterate through every argument */

            SetConsoleTitle(TEXT("Kao-2 Unpacker - Unpacking..."));

            for (int i = 1; (i < argc) && no_errors; i++)
            {
                std::cout << "\n --------------------------------"
                    << "\n \"" << argv[i] << "\""
                    << "\n";

                PakExporter pak(argv[i], (char*)common_path.c_str(), test);

                if (pak.openAndCheckArchive())
                {
                    no_errors = pak.exportArchive();
                }
                else
                {
                    no_errors = false;
                }
            }

            break;
        }

        case 2:
        {

            /* Iterate through every argument */

            SetConsoleTitle(TEXT("Kao-2 Unpacker - Repacking..."));

            for (int i = 1; (i < argc) && no_errors; i++)
            {
                std::cout << "\n --------------------------------"
                    << "\n \"" << argv[i] << "\""
                    << "\n";

                PakImporter pak(argv[i]);

                if (pak.createArchive())
                {
                    no_errors = pak.importData();
                }
                else
                {
                    no_errors = false;
                }
            }

            break;
        }
    }

    /* The end :) */

    SetConsoleTitle(TEXT("Kao-2 Unpacker - Finished!"));

    std::cout << "\n --------------------------------"
        << "\n (press any key to close)"
        << "\n";

    std::cin.ignore();
    return 0;
}


#include <KAO2_PAK/PakExporter.h>


int main(int argc, char** argv)
{
    std::string common_path;

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

    /* Iterate through every argument */

    for (int i = 1; i < argc; i++)
    {

        std::cout << "\n --------------------------------"
            << "\n \"" << argv[1] << "\""
            << "\n";

        PakExporter pak(argv[i], (char*)common_path.c_str());

        if (pak.openAndCheckArchive())
        {
            pak.exportArchive();
        }
    }

    /* The end :) */

    std::cout << "\n --------------------------------"
        << "\n (press any key to close)"
        << "\n";

    std::cin.ignore();
    return 0;
}
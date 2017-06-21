#if defined(MSVS)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <iostream>

#include "MatFile.h"

/**
 * MatFile unit test
 */
class MatFileTest
{

public:

    /**
     * Constructor
     */
    MatFileTest()
    {
    }

    /**
     * Destructor
     */
    ~MatFileTest()
    {
    }

    /**
     * Run the unit tests. This should create 6 output files,
     * all which should be loadable in MatLab
     *
     * @return True if all tests passed
     */
    bool run(const std::string& path) const
    {
        return runTest1(path)
                && runTest2(path);
    }

private:

    bool runTest1(const std::string& path) const
    {
        MatFile matfile(MatFile::RealTime,
                        path);

        const int char_id   = matfile.create<char>("chars");
        const int int_id    = matfile.create<int>("ints");
        const int double_id = matfile.create<double>("doubles");

        if (char_id < 0 || int_id < 0 || double_id < 0)
            return false;

        char chars[] = {'a','b','c','d','e',
                        'f','g','h','i','j',
                        'k','l','m','n','o',
                        'p','q','r','s','t',
                        'u','v','w','x','y',
                        'z'};

        int ints[] = { -1, 0, 1, 2, 3, 4,
                        5, 6, 7, 8, 9, 10 };

        double doubles[] = { -0.1, 0.0,
                              0.1, 0.2,
                              0.3, 0.4,
                              0.5, 0.6,
                              0.7, 0.8 };

        for ( unsigned int i = 0; i < sizeof(chars); i++ )
        {
            if (!matfile.write(char_id,chars[i]))
                return false;
        }

        for ( unsigned int i = 0;
                i < sizeof(ints)/sizeof(int); i++)
        {
            if (!matfile.write(int_id, ints[i]))
                return false;
        }

        for ( unsigned int i = 0;
              i < sizeof(doubles)/sizeof(double); i++)
        {
            if (!matfile.write(double_id, doubles[i]))
                return false;
        }

        return true;
    }

    bool runTest2(const std::string& path) const
    {
        MatFile matfile(MatFile::RealTime,
                        path);

        const int char_id   = matfile.create<char>("chars2");
        const int int_id    = matfile.create<int>("ints2");
        const int double_id = matfile.create<double>("doubles2");

        if (char_id < 0 || int_id < 0 || double_id < 0)
            return false;

        char chars[] = {'a','b','c','d','e',
                        'f','g','h','i','j',
                        'k','l','m','n','o',
                        'p','q','r','s','t',
                        'u','v','w','x','y',
                        'z'};

        int ints[] = { -1, 0, 1, 2, 3, 4,
                        5, 6, 7, 8, 9, 10 };

        double doubles[] = { -0.1, 0.0,
                              0.1, 0.2,
                              0.3, 0.4,
                              0.5, 0.6,
                              0.7, 0.8 };

        for ( unsigned int i = 0; i < sizeof(chars); i++ )
        {
            if (!matfile.write("chars2",chars[i]))
                return false;
        }

        for ( unsigned int i = 0;
                i < sizeof(ints)/sizeof(int); i++)
        {
            if (!matfile.write("ints2", ints[i]))
                return false;
        }

        for ( unsigned int i = 0;
              i < sizeof(doubles)/sizeof(double); i++)
        {
            if (!matfile.write("doubles2",doubles[i]))
                return false;
        }

        return true;
    }
};

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cout << "usage: " << argv[0] << " <output dir>"
            << std::endl;
        return 0;
    }

    MatFileTest test;
    if (test.run(argv[1]))
        std::cout << "passed." << std::endl;
    else
        std::cout << "failed." << std::endl;

#if defined(MSVS)
	_CrtDumpMemoryLeaks();
#endif

    return 0;
}

#ifndef __MATFILE_H__
#define __MATFILE_H__

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <map>
#include <string>
#include <vector>

#include <sys/stat.h>

/*
 * MAT file data types:
 *
 * https://www.mathworks.com/help/pdf_doc/matlab/matfile_format.pdf
 */
#define miINT8    1
#define miUINT8   2
#define miINT16   3
#define miUINT16  4
#define miINT32   5
#define miUINT32  6
#define miSINGLE  7
#define miDOUBLE  9
#define miINT64  12
#define miUINT64 13
#define miMATRIX 14

/*
 * Sizes of matrix subelements:
 */
#define ARRAY_FLAGS_SIZE    16
#define DIM_ARRAY_SIZE      16
#define ARRAY_NAME_TAG_SIZE  8
#define RE_TAG_SIZE          8

/*
 * Mapping from MatLab data type to array type:
 */
static const int mi2mx[16] =
{  0,  8,  9, 10,
  11, 12, 13,  7,
   0,  6,  0,  0,
  14, 15,  0,  0 };

inline char separator()
{
#ifdef _WIN32
    return '\\';
#else
    return '/';
#endif
}

/*
 * The following template specializations are used to determine
 * the type of each variable. Since the MAT file binary format
 * requires us to assign a MatLab data type to each variable,
 * this avoids having to refer to the documentation to find out
 * what types are supported
 */

template <typename T>
struct is_double
{
    static const bool value = false;
};

template<>
struct is_double<double>
{
    static const bool value = true;
};

template <typename T>
struct is_float
{
    static const bool value = false;
};

template<>
struct is_float<float>
{
    static const bool value = true;
};

template <typename T>
struct is_int8
{
    static const bool value = false;
};

template<>
struct is_int8<char>
{
    static const bool value = (sizeof(char) == 1);
};

template <typename T>
struct is_int16
{
    static const bool value = false;
};

template<>
struct is_int16<int>
{
    static const bool value = (sizeof(int)  == 2);
};

template<>
struct is_int16<short>
{
    static const bool value =(sizeof(short) == 2);
};

template <typename T>
struct is_int32
{
    static const bool value = false;
};

template<>
struct is_int32<int>
{
    static const bool value = (sizeof(int)  == 4);
};

template<>
struct is_int32<long>
{
    static const bool value = (sizeof(long) == 4);
};

template <typename T>
struct is_int64
{
    static const bool value = false;
};

template<>
struct is_int64<int>
{
    static const bool value = (sizeof(int)  == 8);
};

template<>
struct is_int64<long>
{
    static const bool value = (sizeof(long) == 8);
};

template<>
struct is_int64<long long>
{
    static const bool value =
                         (sizeof(long long) == 8);
};

template <typename T>
struct is_uint8
{
    static const bool value = false;
};

template<>
struct is_uint8<unsigned char>
{
    static const bool value =
                    (sizeof(unsigned char)  == 1);
};

template <typename T>
struct is_uint16
{
    static const bool value = false;
};

template<>
struct is_uint16<unsigned int>
{
    static const bool value =
                    ( sizeof(unsigned int)  == 2);
};

template<>
struct is_uint16<unsigned short>
{
    static const bool value =
                    (sizeof(unsigned short) == 2);
};

template <typename T>
struct is_uint32
{
    static const bool value = false;
};

template<>
struct is_uint32<unsigned int>
{
    static const bool value =
                    ( sizeof(unsigned int)  == 4);
};

template<>
struct is_uint32<unsigned long>
{
    static const bool value =
                    (sizeof(unsigned long)  == 4);
};

template <typename T>
struct is_uint64
{
    static const bool value = false;
};

template<>
struct is_uint64<unsigned int>
{
    static const bool value =
                    ( sizeof(unsigned int)  == 8);
};

template<>
struct is_uint64<unsigned long>
{
    static const bool value =
                    (sizeof(unsigned long)  == 8);
};

template<>
struct is_uint64<unsigned long long>
{
    static const bool value =
                (sizeof(unsigned long long) == 8);
};

/**
 * A simple interface for outputting data that can be opened using
 * MatLab's load()
 */
class MatFile
{
    /*
     * Base class which allows us to polymorphically reference
     * different Variable types
     */
    class variable_base
    {
    public:
        virtual ~variable_base() {};
    };

    /*
     * Represents an output variable of type T, which can be an
     * int, float, etc.
     */
    template <class T>
    class Variable : public variable_base
    {
    public:

        Variable(const std::string& path, const std::string& name)
            : _count(0),
              _dim_tag_offset(0xA4), _mat_tag_offset(0x84),
              _name(name)
        {
            _fp = std::fopen((path +
                              separator() + name + ".mat").c_str(),
                             "wb");

            if (!write_header() || !write_meta_data())
            {
                std::fclose(_fp);
                _fp = NULL;
            }
        }

        Variable(const Variable& copy)
            : _dim_tag_offset(0xA4), _mat_tag_offset(0x84)
        {
            *this = copy;
        }

        Variable& operator=(const Variable& rhs)
        {
            if (this == &rhs)
                return *this;

            if (_fp)
                std::fclose(_fp);

            _count = rhs._count;
            _fp    = rhs._fp; rhs._fp = NULL;
            _name  = rhs._name;

            _mat_tag_size  =
                rhs._mat_tag_size;
            _re_tag_offset =
                    rhs._re_tag_offset;

            return *this;
        }

        ~Variable()
        {
            if (_fp)
                std::fclose(_fp);
        }

        bool write(const T& element)
        {
            if (_fp == NULL)
                return false;

            const size_t num =
                std::fwrite( &element, sizeof(T), 1, _fp );

            _count += num;
            return num == 1
                && update_counters();
        }

        size_t write(const T* data, size_t numel)
        {
            if (_fp == NULL)
                return 0;

            const size_t num =
                std::fwrite( data, sizeof(T), numel, _fp );

            _count += num;
            return num == numel
                && update_counters();
        }

        bool write_header()
        {
            const size_t HEADER_SIZE = 124;

            time_t raw; std::time(&raw);

            char header[HEADER_SIZE];

            std::memset(header, 0,
                            HEADER_SIZE);

            std::string header_s =
                        std::string("Name: ") + _name   +
                        "\nFormat: MATLAB 5.0 MAT file" +
                        "\nCreated: " + std::ctime(&raw);

            const size_t num_bytes =
                std::min( HEADER_SIZE, header_s.size() );

            std::memcpy(header, header_s.c_str(),
                        num_bytes);

            const short version = 0x0100;
            const short endian  =
                        (('M') << 8) | 'I';

            int numel = std::fwrite(header, sizeof(char),
                                    HEADER_SIZE, _fp);

            numel += std::fwrite(&version, sizeof(short),
                                 1, _fp);
            numel += std::fwrite(&endian , sizeof(short),
                                 1, _fp);

            return
                numel == 126;
        }

        bool write_meta_data()
        {
            /*
             * Write the matrix tag and array flags subelement:
             */
            int bytes = ARRAY_FLAGS_SIZE    +
                        DIM_ARRAY_SIZE      +
                        ARRAY_NAME_TAG_SIZE +
                        _name.size()        +
                        RE_TAG_SIZE;

            if (_name.size() % 8)
                bytes +=  8-(_name.size() % 8);

            _mat_tag_size = bytes;

            int dword[4] =
                {miMATRIX, bytes, miUINT32, 8};

            if (std::fwrite( dword, sizeof(int), 4, _fp ) != 4)
                return false;

            int miType;

            /*
             * Determine the array flags class element:
             */
            if (is_double<T>::value)
                miType = miDOUBLE;
            else if (is_float<T> ::value)
                miType = miSINGLE;
            else if (is_int64<T> ::value)
                miType = miINT64;
            else if (is_uint64<T>::value)
                miType = miUINT64;
            else if (is_int32<T> ::value)
                miType = miINT32;
            else if (is_uint32<T>::value)
                miType = miUINT32;
            else if (is_int16<T> ::value)
                miType = miINT16;
            else if (is_uint16<T>::value)
                miType = miUINT16;
            else if (is_int8<T>  ::value)
                miType = miINT8;
            else if (is_uint8<T> ::value)
                miType = miUINT8;
            else
                return false;

            const int arrayFlags[] =
                         { mi2mx[miType], 0, 0, 0, 0, 0, 0, 0 };

            if (std::fwrite(arrayFlags,sizeof(char),8,_fp) != 8)
                return false;

            /*
             * Dimensions array subelement:
             */
            dword[0] = miINT32;
            dword[1] = 8;
            dword[2] = 1;
            dword[3] = 0;

            if ( std::fwrite( dword, sizeof(int), 4, _fp ) != 4)
                return false;

            /*
             * Array name subelement:
             */
             dword[0] = miINT8;
             dword[1] = _name.size();

            if ( std::fwrite( dword, sizeof(int), 2, _fp ) != 2)
                return false;

            if ( std::fwrite(_name.c_str(), sizeof(char),
                             _name.size() , _fp) !=_name.size())
                return false;


            const int rem = _name.size() % 8;
            if (rem)
            {
                // Add padding to the name to make sure it aligns
                // on an 8-byte boundary:
                char pad[] = { 0, 0, 0, 0, 0, 0, 0, 0 };

                if (std::fwrite(pad, sizeof(char), 8-rem, _fp) !=
                    (size_t)(8-rem))
                        return false ;
            }

            /*
             * Real part subelement:
             */
             dword[0] = miType;
             dword[1] = 0;

             if ( std::fwrite( dword, sizeof(int), 2, _fp ) != 2)
                return false;

            /*
             * Save the location of the number of bytes in the real
             * part subelement. We'll update this value as we write
             * data samples to the file
             */
            _re_tag_offset = std::ftell(_fp );
            if (_re_tag_offset == -1)
                return false;
            else
                _re_tag_offset -= sizeof(int);

            return true;
        }

    private:

        bool update_counters()
        {
            long curr = std::ftell(_fp);
            if (curr == -1)
                return false;

            size_t bytes = _mat_tag_size + _count * sizeof(T);
            size_t rem = bytes % 8;
            if (rem)
                bytes += (8 - rem);

            if (std::fseek(_fp,_mat_tag_offset,SEEK_SET))
                return false;
            if (std::fwrite(&bytes, sizeof(size_t), 1, _fp)
                != 1)
                return false;

            if (std::fseek(_fp,_dim_tag_offset,SEEK_SET))
                return false;
            if (std::fwrite(&_count,sizeof(size_t), 1, _fp)
                != 1)
                return false;

            bytes = _count * sizeof(T);

            if (std::fseek(_fp,_re_tag_offset, SEEK_SET))
                return false;
            if (std::fwrite(&bytes, sizeof(size_t), 1, _fp)
                != 1)
                return false;

            if (std::fseek(_fp, curr, SEEK_SET))
                return false;

            if (rem)
            {
                /*
                 * Add padding to make sure the data aligns on
                 * an 8-byte boundary in case this is the last
                 * write:
                 */
                const char zeros[8] = {0};
                if (std::fwrite(&zeros,sizeof(char),8-rem,_fp)
                    != 8-rem)
                    return false;
                if ( std::fseek(  _fp, curr, SEEK_SET ) != 0 )
                    return false;
            }

            return true;
        }

        size_t      _count;
        const int   _dim_tag_offset;
        FILE*       _fp;
        const int   _mat_tag_offset;
        size_t      _mat_tag_size;
        std::string _name;
        int          _re_tag_offset;
    };

    typedef std::vector<variable_base*>
        var_v;
    typedef std::map<std::string , int>
        str_int_map;

public:

    /**
     * Running mode of this instance
     */
    typedef enum
    {
        RealTime /**< Set up for real-time data collection */
    } mode_t;

    /**
     * Constructor
     *
     * @param[in] running_mode Mode to run in. Currently only
     *            RealTime is supported
     * @param[in] dir The output directory
     */
    MatFile(mode_t running_mode, const std::string& dir)
        : _dir(dir),
          _name2id(),
          _running_mode(running_mode),
          _variables()
    {
        struct stat info;
        _is_ready =
            !stat(dir.c_str(), &info);
    }

    /**
     * Destructor
     */
    ~MatFile()
    {
        for (size_t i = 0; i < _variables.size(); i++)
            delete _variables[i];
    }

    /**
     * Create a new output variable. This will create a new MAT file
     * that contains data for this variable only
     *
     * @tparam T The type of this variable. This must be a basic C++
     *           type (e.g. float), or anything typedef'd to one
     *
     * @param [in] name The name of this variable. This is also what
     *                  the MAT file will be called
     *
     * @return A unique ID by which to reference the variable, or if
     *         it already exists, its ID
     */
    template <typename T>
    int create(const std::string& name)
    {
        if (!_is_ready)
            return -1;

        const int id = static_cast<int>(_variables.size());

        if (_name2id.find(name) != _name2id.end())
            return _name2id[name];
        else
                _name2id[name] = id;

        _variables.push_back( new Variable<T>(_dir,name) );
        return id;
    }

    /**
     * Get the flag indicating if this MatFile object was properly
     * initialized
     *
     * @return True if object construction succeeded
     */
    bool is_ready() const
    {
        return _is_ready;
    }

    /**
     * Write the next data sample to the MAT file for the specified
     * variable
     *
     * @tparam The type of this variable
     *
     * @param[in] name  The name of the variable to write to
     * @param[in] value The value to write
     *
     * @return True on success
     */
    template <typename T>
    bool write(const std::string& name, const T& value) const
    {
        if (!_is_ready)
            return false;

        str_int_map::const_iterator iter =
            _name2id.find(name);
        if (iter == _name2id.end())
            return false;

        return
            write(iter->second, value);
    }

    /**
     * Write the next data sample to the MAT file for the specified
     * variable
     *
     * @tparam The type of this variable
     *
     * @param[in] id   The ID of the variable to write to, obtained
     *                 from create()
     * @param[in] value The value to write
     *
     * @return True on success
     */
    template <typename T>
    bool write(int id, const T& value) const
    {
        if (!_is_ready)
            return false;

        const size_t _id = id;

        if (_variables.size() <= _id)
            return false;

        Variable<T>* var =
            dynamic_cast<Variable<T>*>(_variables[id]);

        return
            var->write(value);
    }

private:

    std::string _dir;
    bool        _is_ready;
    str_int_map _name2id;
    mode_t      _running_mode;
    var_v       _variables;
};

#endif // __MATFILE_H__

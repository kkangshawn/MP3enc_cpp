/**
 * @file        main.cpp
 * @version     1.0
 * @brief       MP3enc_cpp main application source
 * @date        Mar 4, 2020
 * @author      Siwon Kang (kkangshawn@gmail.com)
 */

#include "main.h"

#include <vector>
#include <time.h>
#if defined __linux
#include <dirent.h>
#elif defined _WIN32
#include <Windows.h>
#endif
using namespace std;

MP3enc* MP3enc::m_instance = nullptr;
size_t MP3enc::refCnt = 0;

void
MP3enc::showUsage()
{
    cout << "Usage:" << endl;
    cout << "   MP3enc_cpp <input_directory | input_filename [-o <output_filename>]> [OPTIONS]" << endl;
    cout << endl << "Options:" << endl;
    cout << "     -h            Show help" << endl;
    cout << "     -r            Search subdirectories recursively" << endl;
    cout << "     -q <mode>     Set quality level" << endl;
    cout << "         fast         fast encoding with small file size" << endl;
    cout << "         standard     standard quality - default" << endl;
    cout << "         best         best quality" << endl;
    cout << "     -v            Verbose detail" << endl;
    cout << endl << "Example:" << endl;
    cout << "   MP3enc_cpp input.wav -o output.mp3" << endl;
    cout << "   MP3enc_cpp wav_dir";
    cout << DELIMITER;
    cout << " -r -q fast -v" << endl;
}

void
MP3enc::checkPath(string path, vector<AudioData*>& v)
{
    if (path.size() < 1) {
        cerr << "ERROR: Input file is null" << endl;
        return;
    }
    if (path.back() == DELIMITER) {
        path.pop_back();
    }

#if defined __linux
    DIR* dir;
    struct dirent* dir_ent;

    dir = opendir(path.c_str());
    if (!dir) {
        /* process single file */
        ifstream infile(path);
        if (!infile.is_open()) {
            cerr << "ERROR: Failed to find " << path << endl;
            return;
        }
        AudioData adata(path, m_opt.outPath);
        adata.start();
        adata.join();
        return;
    }

    while ((dir_ent = readdir(dir)) != NULL) {
        string fullPath = path + DELIMITER + dir_ent->d_name;
        switch (dir_ent->d_type) {
        case DT_DIR:
            if (m_opt.recursive && scmp(dir_ent->d_name, ".") && scmp(dir_ent->d_name, "..")) {
                checkPath(fullPath, v);
            }
            break;
        case DT_REG:
            if (is_wav(dir_ent->d_name)) {
                if (!m_opt.outPath.empty()) {
                    m_opt.outPath.clear();
                    DEBUG::WARN("Output filename(-o) option is ignored in case of decoding directory");
                }
                AudioData* adata = new AudioData(fullPath, m_opt.outPath);
                adata->start();
                v.push_back(adata);
            }
            break;
        case DT_LNK:
            break;
        default:
            break;
        }
    }

    closedir(dir);
#elif defined _WIN32
    HANDLE hFind;
    WIN32_FIND_DATAA data;

    hFind = FindFirstFileA(path.c_str(), &data);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        cerr << "ERROR: Failed to find " << path << endl;
        return;
    }
    if (data.dwFileAttributes == FILE_ATTRIBUTE_ARCHIVE ||
            data.dwFileAttributes == FILE_ATTRIBUTE_NORMAL)
    {
        AudioData* adata = new AudioData(path, m_opt.outPath);
        adata->start();
        v.push_back(adata);
        return;
    }
    else if (data.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
    {
        string findPath = path + DELIMITER + "*";
        hFind = FindFirstFileA(findPath.c_str(), &data);

        do
        {
            string fullPath = path + DELIMITER + data.cFileName;
            if ((data.dwFileAttributes == FILE_ATTRIBUTE_ARCHIVE ||
                        data.dwFileAttributes == FILE_ATTRIBUTE_NORMAL) && is_wav(data.cFileName))
            {
                if (!m_opt.outPath.empty())
                {
                    m_opt.outPath.clear();
                    DEBUG::WARN("Output filename(-o) option is ignored in case of decoding directory");
                }
                AudioData* adata = new AudioData(fullPath, m_opt.outPath);
                adata->start();
                v.push_back(adata);
            }
            else if (data.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY &&
                m_opt.recursive &&
                scmp(data.cFileName, ".") && scmp(data.cFileName, ".."))
            {
                checkPath(fullPath, v);
            }
        } while (FindNextFileA(hFind, &data));
    }
    FindClose(hFind);
#endif
}

bool
MP3enc::parseOption(int argc, char** argv)
{
    if (argc < 2) {
        cerr << "ERROR: Argument missing. Please see below usage:" << endl;
        m_instance->showUsage();
        return false;
    }

    for (int i = 1; i < argc; i++) {
        if (!scmp(argv[i], "-h")) {
            showUsage();
            DEBUG::CLEAR();
            return false;
        } else if (!scmp(argv[i], "-o")) {
            if (i + 1 >= argc) {
                cerr << "ERROR: -o needs output filename" << endl;
                return false;
            } else if (argv[i + 1][0] == '-') {
                cerr << "ERROR: output filename shall not start from '-'" << endl;
                return false;
            }
            m_opt.outPath = argv[i + 1];
            i++;
        } else if (!scmp(argv[i], "-r")) {
            m_opt.recursive = true;
        } else if (!scmp(argv[i], "-q")) {
            i++;
            if (i >= argc) {
                cerr << "ERROR: -q needs to specify mode" << endl;
                return false;
            }
            if (!scmp(argv[i], "fast")) {
                AudioData::set_quality(AudioData::QL_FAST);
                DEBUG::INFO("Quality level: FAST");
            } else if (!scmp(argv[i], "standard")) {
                AudioData::set_quality(AudioData::QL_STANDARD);
                DEBUG::INFO("Quality level: STANDARD");
            } else if (!scmp(argv[i], "best")) {
                AudioData::set_quality(AudioData::QL_BEST);
                DEBUG::INFO("Quality level: BEST");
            } else {
                cerr << "ERROR: Wrong mode for quality level. Please see below usage:" << endl;
                m_instance->showUsage();
                return false;
            }
        } else if (!scmp(argv[i], "-v")) {
            m_opt.verbose = true;
            DEBUG::SET();
        } else {
            m_opt.inPath = argv[i];
        }
    }
    vector<AudioData*> filelist = {};
    checkPath(m_opt.inPath, filelist);
    for (AudioData* d : filelist) {
        d->join();
        delete d;
    }

    return true;
}

void
MP3enc::freeInstance()
{
    if (!m_instance) {
        DEBUG::ERR("No instance to free");
        return;
    }

    if (refCnt > 0) {
        refCnt--;
    }
    if (refCnt == 0) {
        delete m_instance;
        m_instance = nullptr;
    }
}

MP3enc*
MP3enc::getInstance(int argc, char** argv)
{
    if (!m_instance) {
        m_instance = new MP3enc();
        if (!m_instance) {
            DEBUG::ERR("Failed to allocate memory");
            return nullptr;
        }
    }

    if (m_instance->parseOption(argc, argv) == false) {
        DEBUG::ERR("Failed to parse options");
        m_instance->freeInstance();
        return nullptr;
    }

    refCnt++;

    return m_instance;
}

int
MP3enc::main(int argc, char** argv)
{
    cout << "MP3enc_cpp v" << VERSION << endl;

    clock_t t = clock();

    MP3enc* mp3enc = MP3enc::getInstance(argc, argv);

    if (!mp3enc) {
        DEBUG::ERR("Failed to get instance");
        return 1;
    }

    mp3enc->freeInstance();

    t = clock() - t;
    double elapsed = (double)t / CLOCKS_PER_SEC;
    cout << "elapsed " << fixed << elapsed << "s" << endl;

    return 0;
}

int main(int argc, char** argv)
{
    return MP3enc::main(argc, argv);
}

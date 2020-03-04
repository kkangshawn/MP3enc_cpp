#include "common.h"
#include "thread.h"
#include "utils.h"
#include "audio.h"

#include <dirent.h>

using namespace std;

class MP3enc : public Utils, DEBUG {
public:
    static MP3enc* getInstance(int argc, char** argv);
    static int main(int argc, char** argv);
    void freeInstance();
    void showUsage();

private:
    struct Options {
        string          inPath;
        string          outPath;
        bool            recursive;
        bool            verbose;
    };

    MP3enc() : m_opt{ {}, {}, false, false } {}
    virtual ~MP3enc() {}

    bool parseOption(int argc, char** argv);
    void checkPath(string path);

    Options m_opt;
    static MP3enc* m_instance;
    static size_t refCnt;
};
MP3enc* MP3enc::m_instance = nullptr;
size_t MP3enc::refCnt = 0;

MP3enc* MP3enc::getInstance(int argc, char** argv)
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

void MP3enc::freeInstance()
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

void MP3enc::showUsage()
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
    checkPath(m_opt.inPath);

    return true;
}

void
MP3enc::checkPath(string path)
{
    if (path.size() < 1) {
        cerr << "ERROR: Input file is null" << endl;
        return;
    }
    if (path.back() == DELIMITER) {
        path.pop_back();
    }

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

    vector<AudioData*> v;
    while ((dir_ent = readdir(dir)) != NULL) {
        string fullPath = path + DELIMITER + dir_ent->d_name;
        switch (dir_ent->d_type) {
        case DT_DIR:
            if (m_opt.recursive && scmp(dir_ent->d_name, ".") && scmp(dir_ent->d_name, "..")) {
                checkPath(fullPath);
            }
            break;
        case DT_REG:
            if (isWAV(dir_ent->d_name)) {
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
    for (AudioData* d : v) {
        d->join();
        delete d;
    }
    closedir(dir);
}

int MP3enc::main(int argc, char** argv)
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

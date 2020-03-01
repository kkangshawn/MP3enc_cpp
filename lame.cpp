#include "common.h"
#include "utils.h"

#include "lib/lame.h"

using namespace std;

const int SAMPLE_SIZE = 1152;
const int MAX_U_32_NUM = 0xFFFFFFFF;

class AudioData : public Utils, DEBUG {
public:
    enum SOUNDFORMAT {
        sf_unknown,
        sf_raw,
        sf_wave,
        sf_aiff,
        sf_mp1,
        sf_mp2,
        sf_mp3,
        sf_mp123,
        sf_ogg
    };

    /** Structures */
    struct ReaderConfig {
        SOUNDFORMAT input_format;
        int         swap_bytes;
        int         swap_channel;
        int         input_samplerate;
    };

    /** Constructor/Destructor */
    AudioData() : m_ifstream(nullptr), m_ofstream(nullptr), m_infile{}, m_outfile{},
                m_pcmswapbytes(0), m_count_samples_carefully(0),
                m_pcm_is_unsigned_8bit(0), m_rconfig{sf_unknown, 0, 0, 0} {}
    virtual ~AudioData() {
        close_file();
        free_pcm_buffer(m_pcm32);
        free_pcm_buffer(m_pcm16);
    }

    /** Public functions */
    static int      init(int argc, char** argv);
    void*           lame_encoder_loop(lame_t gf, void* data);

private:
    /** Structures */
    struct PcmBuffer {
        void*       ch[2];          /* buffer for each channel */
        int         w;              /* sample width */
        int         n;              /* number of samples allocated */
        int         u;              /* number of samples used */
        int         skip_start;     /* number of samples to ignore at the beginning */
        int         skip_end;       /* number of samples to ignore at the end */
    };

    /** Private functions */
    bool            init_infile(lame_t gfp, const string infile);
    bool            init_outfile(const string infile);
    ifstream*       open_wave_file(lame_t gfp, char const* infile);
    SOUNDFORMAT     parse_file_header(lame_t gfp);
    int             parse_wave_header(lame_t gfp);
    void            close_file();
    void            init_pcm_buffer(PcmBuffer& b, int w);
    void            free_pcm_buffer(PcmBuffer& b);
    void            set_skip_start_and_end();

    ifstream*       m_ifstream;
    ofstream*       m_ofstream;
    string          m_infile;
    string          m_outfile;
    int             m_pcmswapbytes;
    int             m_count_samples_carefully;
    int             m_pcm_is_unsigned_8bit;
    int             m_pcm_is_ieee_float;
    int             m_pcmbitwidth;
    PcmBuffer       m_pcm32;
    PcmBuffer       m_pcm16;
    ReaderConfig    m_rconfig;

    /**
     * @brief   Constant values for parsing wave header
     * @see     parse_wave_header()
     */
    int const WAV_ID_RIFF = 0x52494646;  /* "RIFF" */
    int const WAV_ID_WAVE = 0x57415645;  /* "WAVE" */
    int const WAV_ID_FMT  = 0x666d7420;   /* "fmt " */
    int const WAV_ID_DATA = 0x64617461;  /* "data" */
#ifndef WAVE_FORMAT_PCM
    short const WAVE_FORMAT_PCM = 0x0001;
#endif
#ifndef WAVE_FORMAT_IEEE_FLOAT
    short const WAVE_FORMAT_IEEE_FLOAT = 0x0003;
#endif
#ifndef WAVE_FORMAT_EXTENSIBLE
    short const WAVE_FORMAT_EXTENSIBLE = 0xFFFE;
#endif
};

void*
AudioData::lame_encoder_loop(lame_t gf, void* data)
{
    size_t id3v2_size;
    int iread;
    int buf[2][SAMPLE_SIZE];

    id3v2_size = lame_get_id3v2_tag(gf, 0, 0);
    if (id3v2_size > 0) {
        cout << "size > 0" << endl;
    } else {
        cout << "size < 0" << endl;
    }

    DEBUG::SET();
    string msg = "Encoding ";
    msg += m_infile;
    msg += " -> ";
    msg += m_outfile;
    msg += " as ";
    DEBUG::INFO(msg.c_str());
    cout << 1.e-3 * lame_get_out_samplerate(gf) << " KHz " << endl;
    static const char *mode_names[2][4] = {
        {"stereo", "j-stereo", "dual-ch", "single-ch"},
        {"stereo", "force-ms", "dual-ch", "single-ch"}};
    switch (lame_get_VBR(gf)) {
    case vbr_rh:
        break;
    default:
        break;
    }
    cout << mode_names[lame_get_force_ms(gf)][lame_get_mode(gf)] << " MPEG-" << 2 - lame_get_version(gf) << endl;

    do {
    } while (iread > 0);

    return NULL;

    unsigned char mp3buf[LAME_MAXMP3BUFFER];
    size_t owrite;
}

void
AudioData::init_pcm_buffer(PcmBuffer& b, int w)
{
    b.ch[0] = nullptr;
    b.ch[1] = nullptr;
    b.w = w;
    b.n = 0;
    b.u = 0;
    b.skip_start = 0;
    b.skip_end = 0;
}

void
AudioData::free_pcm_buffer(PcmBuffer& b)
{
    b.ch[0] = nullptr;
    b.ch[1] = nullptr;
    b.n = 0;
    b.u = 0;
}

AudioData::add_pcm_buffer(PcmBuffer& b, 

void
AudioData::set_skip_start_and_end()
{
    int skip_start = 0;
    int skip_end = 0;

    switch (m_rconfig.input_format) {
    case sf_mp123:
        break;
    case sf_mp3:
        skip_start = 528 + 1;
        skip_end = 0 - (528 + 1);
        break;
    case sf_mp2:
    case sf_mp1:
        skip_start += 240 + 1;
        break;
    case sf_raw:
    case sf_wave:
    case sf_aiff:
    default:
        break;
    }
    if (skip_start < 0) skip_start = 0;
    if (skip_end < 0) skip_end = 0;

    m_pcm16.skip_start = m_pcm32.skip_start = skip_start;
    m_pcm16.skip_end = m_pcm32.skip_end = skip_end;
}

void
AudioData::close_file()
{
    if (this->m_ifstream) {
        this->m_ifstream->close();
    }
    if (this->m_ofstream) {
        this->m_ofstream->close();
    }
    this->m_ifstream = nullptr;
    this->m_ofstream = nullptr;
}

AudioData::SOUNDFORMAT
AudioData::parse_file_header(lame_t gfp)
{
    int type = read_32_bits_high_low(m_ifstream);

    this->m_count_samples_carefully = 0;
    this->m_pcm_is_unsigned_8bit = 1;

    if (type == WAV_ID_RIFF) {
        int const ret = parse_wave_header(gfp);
        if (ret > 0) {
            this->m_count_samples_carefully = 1;
            return sf_wave;
        } else if (ret < 0) {
            cout << "WARNING: corrupt or unsupported WAVE format" << endl;
        }
    } else {
        cout << "WARNING: unsupported audio format" << endl;
    }

    return sf_unknown;
}

int
AudioData::parse_wave_header(lame_t gfp)
{
    long    file_length = 0;        /* not used */
    int     type = 0;
    int     format_tag = 0;
    int     channels = 0;
    int     samples_per_sec = 0;
    int     avg_bytes_per_sec = 0;    /* not used */
    int     block_align = 0;        /* not used */
    int     bits_per_sample = 0;
    bool    is_wav = false;
    int     data_length = 0;
    int     sub_size = 0;
    int     loop_count = 20;

    file_length = read_32_bits_high_low(m_ifstream);    /* file length */
    if (read_32_bits_high_low(m_ifstream) != WAV_ID_WAVE) {
        return -1;
    }

    for (loop_count; loop_count > 0; --loop_count) {
        int type = read_32_bits_high_low(m_ifstream);

        if (type == WAV_ID_FMT) {
            sub_size = read_32_bits_low_high(m_ifstream);
            sub_size = make_even_number_of_bytes_in_length(sub_size);
            if (sub_size < 16) {
                /* chunk too short */
                return -1;
            }

            format_tag = read_16_bits_low_high(m_ifstream);
            sub_size -= 2;
            channels = read_16_bits_low_high(m_ifstream);
            sub_size -= 2;
            samples_per_sec = read_32_bits_low_high(m_ifstream);
            sub_size -= 4;
            avg_bytes_per_sec = read_32_bits_low_high(m_ifstream);
            sub_size -= 4;
            block_align = read_16_bits_low_high(m_ifstream);
            sub_size -= 2;
            bits_per_sample = read_16_bits_low_high(m_ifstream);
            sub_size -= 2;

            if ((sub_size > 9) && (format_tag == WAVE_FORMAT_EXTENSIBLE)) {
                read_16_bits_low_high(m_ifstream);    /* cbSize */
                read_16_bits_low_high(m_ifstream);    /* ValidBitsPerSample */
                read_32_bits_low_high(m_ifstream);    /* ChannelMask */
                format_tag = read_16_bits_low_high(m_ifstream);
                sub_size -= 10;
            }

            if (sub_size > 0) {
                if (!m_ifstream->seekg(sub_size, std::ios::cur).good()) {
                    DEBUG::ERR("WAV_ID_FMT seekg() failed");
                    return -1;
                }
            }
        } else if (type == WAV_ID_DATA) {
            sub_size = read_32_bits_low_high(m_ifstream);
            data_length = sub_size;
            is_wav = true;
            break;
        } else {
            sub_size = read_32_bits_low_high(m_ifstream);
            sub_size = make_even_number_of_bytes_in_length(sub_size);
            if (!m_ifstream->seekg(sub_size, std::ios::cur).good()) {
                DEBUG::ERR("seekg() failed");
                return -1;
            }
        }
    }
    if (is_wav) {
        if (format_tag != WAVE_FORMAT_PCM && format_tag != WAVE_FORMAT_IEEE_FLOAT) {
            /* verbose check */
            cerr << "ERROR: Unsupported data format: " << std::hex << format_tag << endl;
            return 0;
        }

        if (lame_set_num_channels(gfp, channels) == -1) {
            /* verbose check */
            cerr << "ERROR: Unsupported number of channels: " << channels << endl;
            return 0;
        }

        if (m_rconfig.input_samplerate != 0) {
            samples_per_sec = m_rconfig.input_samplerate;
        }
        lame_set_in_samplerate(gfp, samples_per_sec);

        m_pcmbitwidth = bits_per_sample;
        m_pcm_is_unsigned_8bit = 1;
        m_pcm_is_ieee_float = (format_tag == WAVE_FORMAT_IEEE_FLOAT) ? 1 : 0;
        lame_set_num_samples(gfp, data_length / (channels * ((bits_per_sample + 7) / 8)));

        return 1;
    }

    return -1;
}

ifstream*
AudioData::open_wave_file(lame_t gfp, char const* infile)
{
    lame_set_num_samples(gfp, MAX_U_32_NUM);

    if (this->m_ifstream) {
        DEBUG::WARN("file is already opened. try reopen.");
        close_file();
    }
    this->m_ifstream = new ifstream(infile, std::ios::binary);

    if (!this->m_ifstream->is_open()) {
        string msg = "could not find \"";
        msg += infile;
        msg += "\"";
        DEBUG::ERR(msg.c_str());
        return nullptr;
    }

    if (this->m_rconfig.input_format == sf_raw) {
        DEBUG::WARN("Assuming raw pcm input file");
        this->m_pcmswapbytes = this->m_rconfig.swap_bytes;
    } else {
        this->m_rconfig.input_format = parse_file_header(gfp);
    }

    if (this->m_rconfig.input_format == sf_unknown) {
        DEBUG::ERR("unknown format");
        close_file();
        return nullptr;
    }

    if (lame_get_num_samples(gfp) == MAX_U_32_NUM) {
        int const tmp_num_channels = lame_get_num_channels(gfp);
        double const filelen = get_file_size(infile);
        if (filelen >= 0 && tmp_num_channels > 0) {
            unsigned long filesize = (unsigned long)(filelen / (2 * tmp_num_channels));
            lame_set_num_samples(gfp, filesize);
            m_count_samples_carefully = 0;
        }
    }

    return this->m_ifstream;
}

bool
AudioData::init_outfile(const string infile)
{
    if (!m_ifstream) {
        DEBUG::ERR("Can't set output file, input file is not set");
        return false;
    }

    /* generate output file */
    m_outfile = infile;
    string::reverse_iterator it = m_outfile.rbegin();

    *(it + 2) = 'm';
    *(it + 1) = 'p';
    *(it)     = '3';

    m_ofstream = new ofstream(m_outfile, std::ios::binary);
    return (this->m_ofstream != NULL);
}

bool
AudioData::init_infile(lame_t gfp, const string infile)
{
    m_count_samples_carefully = 0;
    m_pcmbitwidth = 16;
    m_pcmswapbytes = m_rconfig.swap_bytes;
    m_pcm_is_unsigned_8bit = 1;
    m_pcm_is_ieee_float = 0;
    m_ifstream = open_wave_file(gfp, infile.c_str());
    if (!m_ifstream)
        return false;
    m_infile = infile;

    init_pcm_buffer(m_pcm32, sizeof(int));
    init_pcm_buffer(m_pcm16, sizeof(short));
    set_skip_start_and_end();

    unsigned long n = lame_get_num_samples(gfp);
    if (n != MAX_U_32_NUM) {
        unsigned long const discard = m_pcm32.skip_start + m_pcm32.skip_end;
        lame_set_num_samples(gfp, n > discard ? n - discard : 0);
    }

    return true;
}

int
AudioData::init(int argc, char** argv)
{
    if (argc != 2) {
        cerr << "ERROR: wrong argument" << endl;
        return N_OK;
    }

    lame_t gf = lame_init();
    if (!gf) {
        cerr << "ERROR: fatal error during initialization" << endl;
        return N_OK;
    }

    string infile(argv[1]);
    AudioData adata;

    if (!adata.isWAV(infile)) {
        cerr << "ERROR: " << infile << " is not WAV file" << endl;
        lame_close(gf);
        return N_OK;
    }

    if (!adata.init_infile(gf, infile)) {
        cerr << "ERROR: failed to initialize input file" << endl;
        lame_close(gf);
        return N_OK;
    }

    if (!adata.init_outfile(infile)) {
        cerr << "ERROR: failed to initialize output file" << endl;
        lame_close(gf);
        return N_OK;
    }

    lame_set_write_id3tag_automatic(gf, 0);
    if (lame_init_params(gf) < 0) {
        cerr << "ERROR: lame_init_params() error" << endl;
        lame_close(gf);
        return N_OK;
    }

    cout << "Init succeeded" << endl;

    adata.lame_encoder_loop(gf, NULL);

    lame_init_bitstream(gf);

    lame_close(gf);
    return OK;
}

int main(int argc, char** argv)
{
    int ret = 0;
    ret = AudioData::init(argc, argv);
    return ret;
}

#include "common.h"
#include "utils.h"

#include <vector>
#include <cstring>
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
        int         input_samplerate;
    };

    /** Constructor/Destructor */
    AudioData() : m_ifstream(nullptr), m_ofstream(nullptr), m_infile{}, m_outfile{},
                m_count_samples_carefully(0), m_num_samples_read(0),
                m_pcm_is_unsigned_8bit(0), m_pcm_is_ieee_float(0), m_pcmbitwidth(0),
                m_rconfig{sf_unknown, 0} {DEBUG::SET();}

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
        vector<char> ch[2];         /* buffer for each channel */
        int         w;              /* sample width */
        int         n;              /* number of samples allocated */
        int         u;              /* number of samples used */
        int         skip_start;     /* number of samples to ignore at the beginning */
        int         skip_end;       /* number of samples to ignore at the end */
    };

    /** Private functions */
    bool            init_infile(lame_t& gfp, const string infile);
    bool            init_outfile(const string infile);
    ifstream*       open_wave_file(lame_t& gfp, char const* infile);
    SOUNDFORMAT     parse_file_header(lame_t& gfp);
    int             parse_wave_header(lame_t& gfp);
    void            close_file();
    void            init_pcm_buffer(PcmBuffer& b, int w);
    void            free_pcm_buffer(PcmBuffer& b);
    int             add_pcm_buffer(PcmBuffer& b, void* a0, void* a1, int read);
    int             take_pcm_buffer(PcmBuffer& b, void* a0, void* a1, int a_n, int mm);
    void            set_skip_start_and_end();
    int             get_audio(lame_t gf, int buffer[2][SAMPLE_SIZE]);
    int             get_audio_common(lame_t gf, int buffer[2][SAMPLE_SIZE]);
    int             read_samples_pcm(ifstream* ifs, int sample_buffer[2 * SAMPLE_SIZE],
                                    int samples_to_read);
    int             unpack_read_samples(const int samples_to_read, const int bytes_per_sample,
                                    const int swap_order, int* sample_buffer, ifstream* ifs);

    ifstream*       m_ifstream;
    ofstream*       m_ofstream;
    string          m_infile;
    string          m_outfile;
    int             m_count_samples_carefully;
    int             m_pcm_is_unsigned_8bit;
    int             m_pcm_is_ieee_float;
    int             m_pcmbitwidth;
    PcmBuffer       m_pcm32;
    PcmBuffer       m_pcm16;
    int             m_num_samples_read;
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

int
AudioData::unpack_read_samples(const int samples_to_read, const int bytes_per_sample,
                                const int swap_order, int* sample_buffer, ifstream* ifs)
{
    size_t samples_read;
    int* op;
    unsigned char* ip = (unsigned char*)sample_buffer;
    const int b = sizeof(int) * 8;
    int i;
    char *buf = new char[bytes_per_sample * samples_to_read];
    //ifs->read(buf, 100000);
    /*
    for (int i = 0; i < 100000; i++) {
        cout << "[" << i << "]" << (int)buf[i] << endl;
    }
    */
    samples_read = ifs->read((char*)sample_buffer, bytes_per_sample * samples_to_read).gcount() / bytes_per_sample;
//    cout << "TO READ " << samples_to_read << " actual " << samples_read << endl;
//    sample_buffer = (int*)buf;
/*
    for (int j = 0; j < samples_to_read / 2; j++) {
        cout << "[" << j << "]" << hex << sample_buffer[j] << endl;
    }
    cout << endl;
    */
    op = sample_buffer + samples_read;

//    cout << "bps " << bytes_per_sample << " samples_to_read " << samples_to_read << " so " << swap_order << " samples_read " << samples_read << endl;

#define GA_URS_IFLOOP( ga_urs_bps ) \
    if (bytes_per_sample == ga_urs_bps) \
        for (i = samples_read * bytes_per_sample; (i -= bytes_per_sample) >= 0;)

    if (swap_order == 0) {
        GA_URS_IFLOOP(1)
            *--op = ip[i] << (b - 8);
        GA_URS_IFLOOP(2)
            *--op = ip[i] << (b - 16) | ip[i + 1] << (b - 8);
        GA_URS_IFLOOP(3)
            *--op = ip[i] << (b - 24) | ip[i + 1] << (b - 16) | ip[i + 2] << (b - 8);
        GA_URS_IFLOOP(4)
            *--op = ip[i] << (b - 32) | ip[i + 1] << (b - 24) | ip[i + 2] << (b - 16) | ip[i + 3] << (b - 8);
    } else {
        GA_URS_IFLOOP(1)
            *--op = (ip[i] ^ 0x80) << (b - 8) | 0x7f << (b - 16);
        GA_URS_IFLOOP(2)
            *--op = ip[i] << (b - 8) | ip[i + 1] << (b - 16);
        GA_URS_IFLOOP(3)
            *--op = ip[i] << (b - 8) | ip[i + 1] << (b - 16) | ip[i + 2] << (b - 24);
        GA_URS_IFLOOP(4)
            *--op = ip[i] << (b - 8) | ip[i + 1] << (b - 16) | ip[i + 2] << (b - 24) | ip[i + 3] << (b - 32);
    }
#undef GA_URS_IFLOOP

    if (m_pcm_is_ieee_float) {
        float const m_max = INT32_MAX;
        float const m_min = -(float) INT32_MIN;
        float* x = (float*)sample_buffer;

        if (sizeof(float) != sizeof(int)) {
            cerr << "ERROR: float size mismatch!!!" << endl;
            exit(1);
        }
        for (i = 0; i < samples_to_read; ++i) {
            float const u = x[i];
            int v;
            if (u >= 1) {
                v = INT32_MAX;
            } else if (u <= -1) {
                v = INT32_MIN;
            } else if (u >= 0) {
                v = (int)(u * m_max + 0.5f);
            } else {
                v = (int)(u * m_min - 0.5f);
            }
            sample_buffer[i] = v;
        }
    }

    return samples_read;
}

int
AudioData::read_samples_pcm(ifstream* ifs, int sample_buffer[2 * SAMPLE_SIZE], int samples_to_read)
{
    int samples_read;
    int swap_byte_order;
    int bytes_per_sample = m_pcmbitwidth / 8;

    switch (m_pcmbitwidth) {
    case 32:
    case 24:
    case 16:
        swap_byte_order = 0;
        break;
    case 8:
        swap_byte_order = m_pcm_is_unsigned_8bit;
        break;
    default:
        cerr << "Only 8, 16, 24 and 32 bit input files supported" << endl;
        return -1;
    }
    samples_read = unpack_read_samples(ifs, sample_buffer, samples_to_read, bytes_per_sample, swap_byte_order);
    return samples_read;
}

int
AudioData::get_audio_common(lame_t gf, int buffer[2][SAMPLE_SIZE])
{
    int num_channels = lame_get_num_channels(gf);
    int frame_size = lame_get_framesize(gf);
    unsigned int tmp_num_samples = lame_get_num_samples(gf);
    unsigned int remaining = 0;
    int samples_read;
    int insample[2 * SAMPLE_SIZE];
    int *pidx;

    if (num_channels < 1 || 2 < num_channels ||
        frame_size < 1 || SAMPLE_SIZE < frame_size) {
        cerr << "ERROR: wrong channel or frame size, channel: " << num_channels <<
            " frame_size: " << frame_size << " exceeding " << SAMPLE_SIZE << endl;
        exit(1);
    }

    if (m_count_samples_carefully) {
        if (m_num_samples_read < tmp_num_samples) {
            remaining = tmp_num_samples - m_num_samples_read;
        }

        if (remaining < (unsigned int)frame_size && tmp_num_samples != 0) {
            frame_size = remaining;
        }
    }

    samples_read = read_samples_pcm(m_ifstream, insample, num_channels * frame_size);
    if (samples_read < 0) {
        return samples_read;
    }

    pidx = insample + samples_read;
    samples_read /= num_channels;
    if (buffer != NULL) {
        if (num_channels == 2) {
            for (int i = samples_read; --i >= 0;) {
                buffer[1][i] = *--pidx;
                buffer[0][i] = *--pidx;
            }
        } else if (num_channels == 1) {
            memset(buffer[1], 0, samples_read * sizeof(int));
            for (int i = samples_read; --i >= 0;) {
                buffer[0][i] = *--pidx;
            }
        } else {
            cerr << "ERROR: wrong number of channels (" << num_channels << ")" << endl;
            exit(1);
        }
    }

    if (tmp_num_samples != MAX_U_32_NUM) {
        m_num_samples_read += samples_read;
    }

    return samples_read;
}

int
AudioData::get_audio(lame_t gf, int buffer[2][SAMPLE_SIZE])
{
    int read = 0;
    int used = 0;
    int ret = 0;

    do {
        read = get_audio_common(gf, buffer);
//        for (int i = 0; i < SAMPLE_SIZE; i++) {
//            cout << std::hex << buffer[0][i] << " " << buffer[1][i] << endl;
//        }
        used = add_pcm_buffer(m_pcm32, buffer[0], buffer[1], read);
    } while (read > 0 && used <= 0);

    if (read < 0) {
        return read;
    }

    return take_pcm_buffer(m_pcm32, buffer[0], buffer[1], used, SAMPLE_SIZE);
}

void*
AudioData::lame_encoder_loop(lame_t gf, void* data)
{
    int iread;
    int imp3;
    int buf[2][SAMPLE_SIZE];
    unsigned char mp3buf[LAME_MAXMP3BUFFER];

    /*
    size_t id3v2_size;
    id3v2_size = lame_get_id3v2_tag(gf, 0, 0);
    if (id3v2_size > 0) {
        cout << "size > 0, ";
    } else {
        cout << "size < 0, ";
    }
    cout << "id3v2_size: " << id3v2_size << endl;
*/
    string msg = "Encoding ";
    msg += m_infile;
    msg += " -> ";
    msg += m_outfile;
    msg += " as ";
    DEBUG::INFO(msg.c_str());
    cout << 1.e-3 * lame_get_out_samplerate(gf) << " KHz ";
    static const char *mode_names[2][4] = {
        {"stereo", "j-stereo", "dual-ch", "single-ch"},
        {"stereo", "force-ms", "dual-ch", "single-ch"}};
    cout << mode_names[lame_get_force_ms(gf)][lame_get_mode(gf)] << " MPEG-" << 2 - lame_get_version(gf) << (lame_get_out_samplerate(gf) > 16000 ? ".5" : "") << " LAYER III ";
    switch (lame_get_VBR(gf)) {
    case vbr_rh:
        cout << "quality: " << lame_get_quality(gf);
    case vbr_mt:
    case vbr_mtrh:
        cout << "VBR(q=" << lame_get_VBR_quality(gf) <<  ")" << endl;
        break;
    case vbr_abr:
        cout << "average " << lame_get_VBR_mean_bitrate_kbps(gf) << " kbps quality: " << lame_get_quality(gf) << endl;
        break;
    default:
        cout << lame_get_brate(gf) << " kbps quality: " << lame_get_quality(gf) << endl;
        break;
    }

    do {
        iread = get_audio(gf, buf);
//        cout << "iread: " << iread << endl;
//        cout << "sizeof: " << sizeof(mp3buf) << endl;
        if (iread >= 0) {
            imp3 = lame_encode_buffer_int(gf, buf[0], buf[1], iread, mp3buf, sizeof(mp3buf));
            if (imp3 < 0) {
                if (imp3 == -1) {
                    cerr << "ERROR: mp3 buffer is not big enough..." << endl;
                } else {
                    cout << "ERROR: mp3 internal error: code " << imp3 << endl;
                }
                return (void*)1;
            }

            if (m_ofstream->write((char*)mp3buf, imp3).fail()) {
                cerr << "ERROR: failed to write mp3 output" << endl;
                return (void*)1;
            }
//            cout << "imp3: " << imp3 << endl;
//            for (int i = 0; i < imp3; i++) {
//                cout << hex << (int)mp3buf[i];
//            }
//            cout << endl;
//            return (void*)1;
        }
    } while (iread > 0);

    imp3 = lame_encode_flush(gf, mp3buf, sizeof(mp3buf));
    if (imp3 < 0) {
        if (imp3 == -1) {
            cerr << "ERROR: mp3 buffer is not big enough..." << endl;
        } else {
            cout << "ERROR: mp3 internal error: code " << imp3 << endl;
        }
        return (void*)1;
    }
    if (m_ofstream->write((char*)mp3buf, imp3).fail()) {
        cerr << "ERROR: failed to write mp3 output" << endl;
        return (void*)1;
    }

    /* write id3v1 tag */
    /*
    imp3 = lame_get_id3v1_tag(gf, mp3buf, sizeof(mp3buf));
    if (imp3 <= 0) {
        DEBUG::INFO("no ID3v1 tag exists");
    } else if (imp3 > sizeof(mp3buf)) {
        DEBUG::INFO("ID3v1 tag exceeds buffer size");
    } else {
        if (m_ofstream->write((char*)mp3buf, imp3).fail()) {
            cerr << "ERROR: failed to write ID3v1 tag" << endl;
            return (void*)1;
        }
    }
*/
    /* write xing frame */
    imp3 = lame_get_lametag_frame(gf, mp3buf, sizeof(mp3buf));
    if (imp3 <= 0) {
        DEBUG::INFO("no LAME-tag exists");
    } else if (imp3 > sizeof(mp3buf)) {
        DEBUG::INFO("LAME-tag frame exceeds buffer size");
    } else if (m_ofstream->seekp(id3v2_size, std::ios::beg).fail()) {
        DEBUG::WARN("fatal error: can't update LAME-tag frame!");
    } else {
        if (m_ofstream->write((char*)mp3buf, imp3).fail()) {
            cerr << "ERROR: failed to write LAME-tag" << endl;
        } else {
            cout << "Encoding done" << endl;
        }
    }

    return NULL;
}

void
AudioData::init_pcm_buffer(PcmBuffer& b, int w)
{
    b.ch[0] = {};
    b.ch[1] = {};
    b.w = w;
    b.n = 0;
    b.u = 0;
    b.skip_start = 0;
    b.skip_end = 0;
}

void
AudioData::free_pcm_buffer(PcmBuffer& b)
{
    b.ch[0].clear();
    b.ch[1].clear();
    b.n = 0;
    b.u = 0;
}

int
AudioData::add_pcm_buffer(PcmBuffer& b, void* a0, void* a1, int read)
{
    int a_n;

    if (read < 0) {
        return b.u - b.skip_end;
    }
    if (b.skip_start >= read) {
        b.skip_start -= read;
        return b.u - b.skip_end;
    }
    a_n = read - b.skip_start;

    if (a_n > 0) {
        int const a_skip = b.w * b.skip_start;
        int const a_want = b.w * a_n;
        int const b_used = b.w * b.u;
        int const b_have = b.w * b.n;
        int const b_need = b.w * (b.u + a_n);
        if (b_have < b_need) {
            b.n = b.u + a_n;
            b.ch[0].resize(b_need);
            b.ch[1].resize(b_need);
        }
        b.u += a_n;
        if (a0) {
            char* src = (char*)a0;
            char* dst = b.ch[0].data();
            memcpy(dst + b_used, src + a_skip, a_want);
        }
        if (a1) {
            char* src = (char*)a1;
            char* dst = b.ch[1].data();
            memcpy(dst + b_used, src + a_skip, a_want);
        }
    }
    b.skip_start = 0;

    return b.u - b.skip_end;
}

int AudioData::take_pcm_buffer(PcmBuffer& b, void* a0, void* a1, int a_n, int mm)
{
    if (a_n > mm) {
        a_n = mm;
    }
    if (a_n > 0) {
        int const a_take = b.w * a_n;
        if (a0) {
            memcpy(a0, b.ch[0].data(), a_take);
        }
        if (a1) {
            memcpy(a1, b.ch[1].data(), a_take);
        }
        b.u -= a_n;
        if (b.u < 0) {
            b.u = 0;
            return a_n;
        }
        memmove(b.ch[0].data(), b.ch[0].data() + a_take, b.w * b.u);
        memmove(b.ch[1].data(), b.ch[1].data() + a_take, b.w * b.u);
    }

    return a_n;
}

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
AudioData::parse_file_header(lame_t& gfp)
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
AudioData::parse_wave_header(lame_t& gfp)
{
    long    file_length = 0;        /* not used */
    int     type = 0;
    int     format_tag = 0;
    int     channels = 0;
    int     samples_per_sec = 0;
    int     avg_bytes_per_sec = 0;  /* not used */
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
                if (m_ifstream->seekg(sub_size, std::ios::cur).fail()) {
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
            if (m_ifstream->seekg(sub_size, std::ios::cur).fail()) {
                DEBUG::ERR("seekg() failed");
                return -1;
            }
        }
    }
    /*
    char buf[100000];
    m_ifstream->read(buf, 100000);
    for (int i = 0; i < 100000; i++) {
        cout << "[" << i << "]" << (int)buf[i] << endl;
    }
    */
    if (is_wav) {
        if (format_tag != WAVE_FORMAT_PCM && format_tag != WAVE_FORMAT_IEEE_FLOAT) {
            cerr << "ERROR: Unsupported data format: " << std::hex << format_tag << endl;
            return 0;
        }

        if (lame_set_num_channels(gfp, channels) == -1) {
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
AudioData::open_wave_file(lame_t& gfp, char const* infile)
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
AudioData::init_infile(lame_t& gfp, const string infile)
{
    m_count_samples_carefully = 0;
    m_pcmbitwidth = 16;
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
    lame_set_VBR_q(gf, 2);
    lame_set_VBR(gf, vbr_default);

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

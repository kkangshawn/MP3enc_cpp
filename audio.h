#ifndef _AUDIO_H
#define _AUDIO_H

#include "common.h"
#include "utils.h"
#include "thread.h"

#include <vector>
#include "lib/lame.h"

using namespace std;

class AudioData : public Thread, Utils, DEBUG {
public:
    enum QUALITY_LEVEL { QL_FAST, QL_STANDARD, QL_BEST };
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
    AudioData(string infile, string outfile) : m_ifstream(nullptr), m_ofstream(nullptr),
                m_infile{}, m_outfile{}, m_init(false), m_count_samples_carefully(0),
                m_pcm_is_unsigned_8bit(0), m_pcm_is_ieee_float(0), m_pcmbitwidth(0),
                m_num_samples_read(0), m_rconfig{sf_unknown, 0} { m_init = init(infile, outfile); }

    virtual ~AudioData() {
        close_file();
        free_pcm_buffer(m_pcm32);
        free_pcm_buffer(m_pcm16);
        if (m_gf) {
            lame_close(m_gf);
        }
    }

    /** Public functions */
    static void     set_quality(QUALITY_LEVEL quality);
    void*           lame_encoder_loop(void* data);
    void            run();
    static const int SAMPLE_SIZE = 1152;

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
    bool            init(string infile, string outfile);
    bool            init_infile(lame_t& gfp, const string infile);
    bool            init_outfile(const string infile, const string outfile);
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
    int             unpack_read_samples(ifstream* ifs, int* sample_buffer,
                        const int samples_to_read, const int bytes_per_sample, const int swap_order);

    lame_t          m_gf;
    ifstream*       m_ifstream;
    ofstream*       m_ofstream;
    string          m_infile;
    string          m_outfile;
    bool            m_init;
    int             m_count_samples_carefully;
    int             m_pcm_is_unsigned_8bit;
    int             m_pcm_is_ieee_float;
    int             m_pcmbitwidth;
    PcmBuffer       m_pcm32;
    PcmBuffer       m_pcm16;
    int             m_num_samples_read;
    ReaderConfig    m_rconfig;
    static QUALITY_LEVEL encoding_quality;

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
#endif

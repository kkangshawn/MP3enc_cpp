/**
 * @file        audio.h
 * @version     0.9
 * @brief       MP3enc_cpp audio data module header
 * @date        Mar 4, 2020
 * @author      Siwon Kang (kkangshawn@gmail.com)
 */

#ifndef _AUDIO_H
#define _AUDIO_H

#include "common.h"
#include "utils.h"
#include "thread.h"

#include <vector>
#include "lib/lame.h"

/**
 * @class   AudioData audio.h "audio.h"
 * @brief   class for processing audio data which is strongly involved with LAME library.
 *          Many are derived from LAME library's frontend application code but mostly
 *          refactored to be adjusted to object-oriented design and got slim.
 */
class AudioData : public Thread, Utils, DEBUG {
public:
    enum QUALITY_LEVEL { QL_FAST, QL_STANDARD, QL_BEST };

    /* Constructor/Destructor */
    AudioData(std::string infile, std::string outfile) : m_ifstream(nullptr), m_ofstream(nullptr),
                m_infile{}, m_outfile{}, m_init(false), m_count_samples_carefully(0),
                m_pcm_is_unsigned_8bit(0), m_pcm_is_ieee_float(0), m_pcmbitwidth(0),
                m_pcm32{ {}, 0, 0, 0, 0, 0 }, m_pcm16{ {}, 0, 0, 0, 0, 0 },
                m_num_samples_read(0), m_rconfig{SOUNDFORMAT::sf_unknown, 0}
    {
        m_init = init(infile, outfile);
    }

    virtual ~AudioData() {
        close_file();
        free_pcm_buffer(m_pcm32);
        free_pcm_buffer(m_pcm16);
        if (m_gf) {
            lame_close(m_gf);
        }
    }

    /**
     * @fn      static void set_quality(QUALITY_LEVEL quality)
     * @brief   set encoding quality.
     * @param [in]  quality     encoding quality preset specified as QL_STANDARD, QL_FAST, QL_BEST
     */
    static void     set_quality(QUALITY_LEVEL quality);
    /**
     * @fn      void* lame_encoder_loop(void* data)
     * @brief   An encoding subroutine to be run as thread.
     * @param [in]  data    void formed input argument, not used yet
     * @return  void formed return value, not used yet
     */
    void*           lame_encoder_loop(void* data);
    /**
     * @fn      void run()
     * @brief   A function to be binded to thread. lame_encoder_loop() takes place.
     */
    void            run();

private:
    static const int SAMPLE_SIZE = 1152;
    enum class SOUNDFORMAT {
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

    /**
     * @struct  ReaderConfig audio.h "audio.h"
     * @brief   Information of input sound.
     */
    struct ReaderConfig {
        SOUNDFORMAT input_format;
        int         input_samplerate;
    };

    /**
     * @struct  PcmBuffer audio.h "audio.h"
     * @brief   Buffer format to read and write pcm sound.
     */
    struct PcmBuffer {
        std::vector<char> ch[2];    /**< buffer for each channel */
        int         w;              /**< sample width */
        int         n;              /**< number of samples allocated */
        int         u;              /**< number of samples used */
        int         skip_start;     /**< number of samples to ignore at the beginning */
        int         skip_end;       /**< number of samples to ignore at the end */
    };

    /* Private functions */
    bool            init(std::string infile, std::string outfile);
    bool            init_infile(lame_t& gfp, const std::string infile);
    bool            init_outfile(const std::string infile, const std::string outfile);
    std::ifstream*  open_wave_file(lame_t& gfp, char const* infile);
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
    int             read_samples_pcm(std::ifstream* ifs, int sample_buffer[2 * SAMPLE_SIZE],
                                    int samples_to_read);
    int             unpack_read_samples(std::ifstream* ifs, int* sample_buffer,
                        const int samples_to_read, const int bytes_per_sample, const int swap_order);

    lame_t          m_gf;
    std::ifstream*  m_ifstream;
    std::ofstream*  m_ofstream;
    std::string     m_infile;
    std::string     m_outfile;
    bool            m_init;
    int             m_count_samples_carefully;
    int             m_pcm_is_unsigned_8bit;
    int             m_pcm_is_ieee_float;
    int             m_pcmbitwidth;
    PcmBuffer       m_pcm32;
    PcmBuffer       m_pcm16;
    unsigned int    m_num_samples_read;
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

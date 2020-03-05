/**
 * @file        audio.cpp
 * @version     1.0
 * @brief       MP3enc_cpp audio data module source
 * @date        Mar 4, 2020
 * @author      Siwon Kang (kkangshawn@gmail.com)
 */

#include "audio.h"

#include <sstream>
#include <cstring>

using namespace std;

const unsigned int MAX_U_32_NUM = 0xFFFFFFFF;
AudioData::QUALITY_LEVEL AudioData::encoding_quality = QL_STANDARD;

int
AudioData::unpack_read_samples(ifstream* ifs, int* sample_buffer,
        int samples_to_read, const int bytes_per_sample, const int swap_order)
{
    size_t          samples_read;
    int*            op;
    unsigned char*  ip = (unsigned char*)sample_buffer;
    const int       b = sizeof(int) * 8;
    int             i;

    samples_read = ifs->read((char*)sample_buffer, bytes_per_sample * samples_to_read).gcount();
    samples_read /= bytes_per_sample;
    op = sample_buffer + samples_read;

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
        float const m_min = -(float)INT32_MIN;
        float*      x = (float*)sample_buffer;

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
    samples_read = unpack_read_samples(ifs, sample_buffer,
                            samples_to_read, bytes_per_sample, swap_byte_order);
    return samples_read;
}

int
AudioData::get_audio_common(lame_t gf, int buffer[2][SAMPLE_SIZE])
{
    int             num_channels = lame_get_num_channels(gf);
    int             frame_size = lame_get_framesize(gf);
    unsigned int    tmp_num_samples = lame_get_num_samples(gf);
    unsigned int    remaining = 0;
    int             samples_read;
    int             insample[2 * SAMPLE_SIZE];
    int*            pidx;

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

    do {
        read = get_audio_common(gf, buffer);
        used = add_pcm_buffer(m_pcm32, buffer[0], buffer[1], read);
    } while (read > 0 && used <= 0);

    if (read < 0) {
        return read;
    }

    return take_pcm_buffer(m_pcm32, buffer[0], buffer[1], used, SAMPLE_SIZE);
}

void
AudioData::set_quality(QUALITY_LEVEL quality)
{
    AudioData::encoding_quality = quality;
}

void
AudioData::run()
{
    if (m_init) {
        lame_encoder_loop(NULL);
    } else {
        DEBUG::ERR("can't start thread because not initialized");
    }
    lame_init_bitstream(m_gf);
}

void*
AudioData::lame_encoder_loop(void* data)
{
    int             iread;
    int             imp3;
    size_t          tagsize;
    int             buf[2][SAMPLE_SIZE];
    unsigned char   mp3buf[LAME_MAXMP3BUFFER];
    ostringstream   msg;

    for (int i = 0; i < LAME_MAXMP3BUFFER; i++) {
        mp3buf[i] = 0;
    }

    size_t id3v2_size;
    id3v2_size = lame_get_id3v2_tag(m_gf, 0, 0);

    msg << "Start encoding [" << m_infile << " -> " << m_outfile << "]";
    if (DEBUG::IS_SET()) {
        msg << " as " << (1.e-3 * lame_get_out_samplerate(m_gf)) << "KHz ";
        static const char *mode_names[2][4] = {
            {"stereo", "j-stereo", "dual-ch", "single-ch"},
            {"stereo", "force-ms", "dual-ch", "single-ch"}};
        msg << mode_names[lame_get_force_ms(m_gf)][lame_get_mode(m_gf)] << " MPEG-" <<
            (2 - lame_get_version(m_gf)) <<
            (lame_get_out_samplerate(m_gf) < 16000 ? ".5" : "") << " LAYER III ";
        switch (lame_get_VBR(m_gf)) {
        case vbr_rh:
            msg << "quality: " << lame_get_quality(m_gf);
        case vbr_mt:
        case vbr_mtrh:
            msg << "VBR(q=" << lame_get_VBR_quality(m_gf) << ")";
            break;
        case vbr_abr:
            msg << "average " << lame_get_VBR_mean_bitrate_kbps(m_gf) <<
                " kbps quality: " << lame_get_quality(m_gf);
            break;
        default:
            msg << lame_get_brate(m_gf) << " kbps quality: " << lame_get_quality(m_gf);
            break;
        }
    }
    cout << msg.str() << endl;

    do {
        iread = get_audio(m_gf, buf);
        if (iread >= 0) {
            imp3 = lame_encode_buffer_int(m_gf, buf[0], buf[1], iread, mp3buf, sizeof(mp3buf));
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
        }
    } while (iread > 0);

    imp3 = lame_encode_flush(m_gf, mp3buf, sizeof(mp3buf));
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

    /* write xing frame */
    tagsize = lame_get_lametag_frame(m_gf, mp3buf, sizeof(mp3buf));
    if (tagsize <= 0) {
        DEBUG::INFO("no LAME-tag exists");
    } else if (tagsize > sizeof(mp3buf)) {
        DEBUG::INFO("LAME-tag frame exceeds buffer size");
    } else if (m_ofstream->seekp(id3v2_size, std::ios::beg).fail()) {
        DEBUG::WARN("fatal error: can't update LAME-tag frame!");
    } else {
        if (m_ofstream->write((char*)mp3buf, tagsize).fail()) {
            cerr << "ERROR: failed to write LAME-tag" << endl;
        } else {
            cout << "Encoding " << m_outfile << " done" << endl;
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
    case SOUNDFORMAT::sf_mp123:
        break;
    case SOUNDFORMAT::sf_mp3:
        skip_start = 528 + 1;
        skip_end = 0 - (528 + 1);
        break;
    case SOUNDFORMAT::sf_mp2:
    case SOUNDFORMAT::sf_mp1:
        skip_start += 240 + 1;
        break;
    case SOUNDFORMAT::sf_raw:
    case SOUNDFORMAT::sf_wave:
    case SOUNDFORMAT::sf_aiff:
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
            return SOUNDFORMAT::sf_wave;
        } else if (ret < 0) {
            cout << "WARNING: corrupt or unsupported WAVE format" << endl;
        }
    } else {
        cout << "WARNING: unsupported audio format" << endl;
    }

    return SOUNDFORMAT::sf_unknown;
}

int
AudioData::parse_wave_header(lame_t& gfp)
{
/*  long    file_length = 0;        // not used */
    int     type = 0;
    int     format_tag = 0;
    int     channels = 0;
    int     samples_per_sec = 0;
/*  int     avg_bytes_per_sec = 0;  // not used */
/*  int     block_align = 0;        // not used */
    int     bits_per_sample = 0;
    bool    is_wav = false;
    int     data_length = 0;
    int     sub_size = 0;

    /*file_length = */read_32_bits_high_low(m_ifstream);
    if (read_32_bits_high_low(m_ifstream) != WAV_ID_WAVE) {
        return -1;
    }

    for (int loop_count = 20; loop_count > 0; --loop_count) {
        type = read_32_bits_high_low(m_ifstream);

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
            /*avg_bytes_per_sec = */read_32_bits_low_high(m_ifstream);
            sub_size -= 4;
            /*block_align = */read_16_bits_low_high(m_ifstream);
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

    if (this->m_rconfig.input_format == SOUNDFORMAT::sf_raw) {
        DEBUG::WARN("Assuming raw pcm input file");
    } else {
        this->m_rconfig.input_format = parse_file_header(gfp);
    }

    if (this->m_rconfig.input_format == SOUNDFORMAT::sf_unknown) {
        DEBUG::ERR("unknown format");
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
AudioData::init_outfile(const string infile, const string outfile)
{
    m_outfile = outfile;
    if (m_outfile.empty()) {
        m_outfile = infile;
    }
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

bool
AudioData::init(string infile, string outfile)
{
    m_gf = lame_init();
    if (!m_gf) {
        cerr << "ERROR: fatal error during initialization" << endl;
        return false;
    }

    switch (AudioData::encoding_quality) {
    case QL_BEST:
        lame_set_preset(m_gf, INSANE);
        lame_set_quality(m_gf, 0);
        break;
    case QL_FAST:
        lame_set_force_ms(m_gf, 1);
        lame_set_mode(m_gf, JOINT_STEREO);
        lame_set_quality(m_gf, 7);
        break;
    case QL_STANDARD:
    default:
        lame_set_VBR_q(m_gf, 2);
        lame_set_VBR(m_gf, vbr_default);;
        break;
    }

    if (!init_infile(m_gf, infile)) {
        cerr << "ERROR: failed to initialize input file: " << infile << endl;
        return false;
    }

    if (!init_outfile(infile, outfile)) {
        cerr << "ERROR: failed to initialize output file" << endl;
        return false;
    }

    lame_set_write_id3tag_automatic(m_gf, 0);
    if (lame_init_params(m_gf) < 0) {
        cerr << "ERROR: lame_init_params() error" << endl;
        return false;
    }

    DEBUG::INFO("LAME library initialization succeeded");
    return true;
}

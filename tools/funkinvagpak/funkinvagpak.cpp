/*
 * this is just cuckys mus converter hacked up to make streamed vags LOL
 * funkinmuspak by Regan "CuckyDev" Green
 * Converts audio files to MUS files for the Friday Night Funkin' PSX port
*/

/*
  Uses ADPCM conversion by spicyjpeg
  (C) 2021 spicyjpeg
*/

#define NOMINMAX

#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <string>
#include <iomanip>
#include <cstring>

// https://miniaud.io/docs/manual/index.html#Decoding
#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_ENCODING
#define MA_NO_DEVICE_IO
#define MA_HAS_VORBIS
#define MA_API static
#include "miniaudio.h"

#undef STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

#include "adpcm.h"

//Audio constants
#define DEFAULT_SAMPLE_RATE 44100
#define DEFAULT_BUFFER_SIZE 2048

struct InputAudio
{
    //Audio data
    std::vector<int16_t> data;
};

struct VagChannel
{
    //Descriptor
    std::string path;
    float use_l = 0.0f, use_r = 0.0f;
    
    //Audio data
    InputAudio *audio = nullptr;
    std::vector<uint8_t> adpcm;
};

void write_int16_little(uint8_t *ptr, int16_t value) {
    ptr[0] = (uint8_t) ((value >> 0) & 0xff);
    ptr[1] = (uint8_t) ((value >> 8) & 0xff);
}

void write_int16_big(uint8_t *ptr, int16_t value) {
    ptr[0] = (uint8_t) ((value >> 8) & 0xff);
    ptr[1] = (uint8_t) ((value >> 0) & 0xff);
}

void write_int32_little(uint8_t *ptr, int32_t value) {
    ptr[0] = (uint8_t) ((value >>  0) & 0xff);
    ptr[1] = (uint8_t) ((value >>  8) & 0xff);
    ptr[2] = (uint8_t) ((value >> 16) & 0xff);
    ptr[3] = (uint8_t) ((value >> 24) & 0xff);
}

void write_int32_big(uint8_t *ptr, int32_t value) {
    ptr[0] = (uint8_t) ((value >> 24) & 0xff);
    ptr[1] = (uint8_t) ((value >> 16) & 0xff);
    ptr[2] = (uint8_t) ((value >>  8) & 0xff);
    ptr[3] = (uint8_t) ((value >>  0) & 0xff);
}

//Entry point
int main(int argc, char *argv[])
{
    int sample_rate = DEFAULT_SAMPLE_RATE;
    size_t buffer_size = DEFAULT_BUFFER_SIZE;

    //Check arguments
    if (argc < 3) {
        std::cout << "usage: funkinvagpak out_vag in_txt [sample_rate] [interleave]" << std::endl;
        std::cout << "default values: sample_rate=" << DEFAULT_SAMPLE_RATE << ", interleave=" << DEFAULT_BUFFER_SIZE << std::endl;
        return 1;
    }
    if (argc >= 4)
        sample_rate = strtoul(argv[3], nullptr, 0);
    if (argc >= 5)
        buffer_size = strtoul(argv[4], nullptr, 0);

    std::string path_vag = std::string(argv[1]);
    std::string path_txt = std::string(argv[2]);
    
    std::string path_base;
    auto path_base_cut = path_txt.find_last_of("/\\");
    if (path_base_cut != std::string::npos)
        path_base = path_txt.substr(0, path_base_cut + 1);
    
    //Read txt file
    std::unordered_map<std::string, InputAudio> vag_audio;
    std::vector<VagChannel> vag_channels;
    
    std::ifstream stream_txt(path_txt);
    if (!stream_txt.is_open())
    {
        std::cout << "Failed to open txt " << path_txt << std::endl;
        return 1;
    }

    size_t samples_per_buffer = buffer_size / spu::BLOCK_LENGTH * spu::BLOCK_NUM_SAMPLES;
    int16_t *pcm_read_buffer = new int16[samples_per_buffer * 2];

    //Read channels
    size_t max_length = 0;

    while (!stream_txt.eof())
    {
        //Read descriptor
        VagChannel channel;
        stream_txt >> std::quoted(channel.path) >> channel.use_l >> channel.use_r;
        if (!channel.path.size())
            continue;

        std::cout << "Encoding " << channel.path << " [L=" << channel.use_l << ", R=" << channel.use_r << "]" << std::endl;

        //Read audio
        auto audio_find = vag_audio.find(channel.path);
        if (audio_find != vag_audio.end())
        {
            //Use already loaded audio
            channel.audio = &audio_find->second;
        }
        else
        {
            //Load audio
            std::string path = path_base + channel.path;
            InputAudio audio;

            //Create miniaudio decoder
            ma_decoder decoder;
            ma_decoder_config decoder_config = ma_decoder_config_init(ma_format_s16, 2, sample_rate);

            if (ma_decoder_init_file(path.c_str(), &decoder_config, &decoder) != MA_SUCCESS)
            {
                std::cout << "Failed to open audio " << path << std::endl;
                return 1;
            }

            std::cout << "  Decoding " << channel.path << std::endl;

            //Read PCM data and buffer it in memory
            while (1) {
                size_t length = ma_decoder_read_pcm_frames(&decoder, pcm_read_buffer, samples_per_buffer);
                if (!length)
                    break;

                audio.data.insert(audio.data.end(), pcm_read_buffer, &pcm_read_buffer[length * 2]);
            }

            //Delete miniaudio decoder
            ma_decoder_uninit(&decoder);

            //Push audio to audio map
            vag_audio[channel.path] = audio;
            channel.audio = &vag_audio[channel.path];
        }

        //Mix audio
        std::vector<int16_t> mix_buffer(channel.audio->data.size() / 2);
        for (size_t i = 0; i < mix_buffer.size(); i++) {
            auto data = &(channel.audio->data[i*2]);
            mix_buffer[i] = (int16_t)((float)data[0] * channel.use_l + (float)data[1] * channel.use_r);
        }

        //Encode audio to ADPCM
        size_t length = spu::getNumBlocks(mix_buffer.size()) * spu::BLOCK_LENGTH;
        channel.adpcm.resize(length);
        spu::encodeSound(mix_buffer.data(), channel.adpcm.data(), mix_buffer.size(), mix_buffer.size());
        max_length = std::max(max_length, length);

        //Push channel to channel list
        vag_channels.push_back(channel);
    }

    //Close txt file
    stream_txt.close();
    delete[] pcm_read_buffer;

    //Write vag file
    std::ofstream stream_vag(path_vag, std::ios::binary);
    if (!stream_vag.is_open())
    {
        std::cout << "Failed to open vag " << path_vag << std::endl;
        return 1;
    }

    std::cout << "Writing " << vag_channels.size() << " channels to " << path_vag << std::endl;

    //Write meta header
    uint8_t header[2048] = {};

    memcpy(&header[0x0], "VAGi", 4);
    write_int32_big(&header[0x4], 0x20); // version
    write_int32_little(&header[0x8], buffer_size); // buffer size
    write_int32_big(&header[0xc], max_length); // size of audio data  for each channel
    write_int32_big(&header[0x10], sample_rate); // sample rate
    memset(&header[0x14], 0, 10);
    write_int16_little(&header[0x1e], vag_channels.size()); // channels
    strncpy((char*)&header[0x20], path_vag.c_str(), 16); // file name

    stream_vag.write((const char*)header, 2048);

    size_t num_chunks = (max_length + (buffer_size - 1)) / buffer_size;
    for (size_t i = 0; i < num_chunks; i++)
    {
        size_t offset = i * buffer_size;

        //Write chunk buffers
        for (auto &channel : vag_channels)
        {
            int length = (int)channel.adpcm.size() - (int)offset;
            length = std::min(std::max(length, 0), (int)buffer_size);

            if (length) {
                channel.adpcm[offset + length - (spu::BLOCK_LENGTH - 1)] = 0x03; // add loop flag
                stream_vag.write((const char*) &(channel.adpcm[offset]), length);
            }

            for (int k = buffer_size - length; k; k--) // pad chunk
                stream_vag.put(0);
        }
    }
    
    //Close vag file
    stream_vag.close();
    
    return 0;
}

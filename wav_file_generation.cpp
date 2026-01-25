#include <cmath>    // calcualte sine wave
#include <cstring>  // strcmp, strcpy and strlen
#include <fstream>  // for files
#include <iostream>



// Students are expected to program the actual packing directly from numeric values into the character array/arrays.  The python example uses an external library for that, but we will not.

// Note that you don’t have to write everything to the file at once.  You can reuse a static array and write a large file in chunks.  Also note that in this program your arrays may actually be enormous compared to the data in assignments you have done previously in programming courses.


void writeUIntLE(unsigned char* out, unsigned int value, int byteSize) {
    /* write unsigned value of size byteSize as little endinan
        return that value in out */
    for (int i = 0; i < byteSize; ++i) {
        out[i] = (unsigned char)(value & 0xFF);
        value >>= 8;
    }
}

static void makeWaveHeader(
    unsigned char header[44],
    unsigned int sampleRate,
    unsigned short numChannels,
    unsigned short bitsPerSample,
    unsigned int numSamples)
{
    // derived values
    unsigned int byteRate = sampleRate * numChannels * (bitsPerSample / 8);
    unsigned short blockAlign = (unsigned short) (numChannels * (bitsPerSample / 8));
    
    unsigned int subchunk2Size = numSamples * numChannels * (bitsPerSample / 8);
    unsigned int chunkSize = 36 + subchunk2Size; // or 4 + (8 + SubChunk1Size) + (8 + SubChunk2Size)

    
    // RIFF HEADER

    // 0–3 "RIFF"
    header[0] = 'R';
    header[1] = 'I';
    header[2] = 'F';
    header[3] = 'F';

    // 4–7 ChunkSize
    writeUIntLE(header + 4, chunkSize, 4);

    // 8–11 "WAVE"
    header[8]  = 'W';
    header[9]  = 'A';
    header[10] = 'V';
    header[11] = 'E';


    // SUBCHUNKS - Subchunk 1

    // 12–15 "fmt "
    header[12] = 'f';
    header[13] = 'm';
    header[14] = 't';
    header[15] = ' ';

    // 16–19 Subchunk1Size (16 for PCM)
    writeUIntLE(header + 16, 16, 4);

    // 20–21 AudioFormat (1 = PCM)
    writeUIntLE(header + 20, 1, 2);

    // 22–23 NumChannels (Mono = 1, Stereo = 2, etc)
    writeUIntLE(header + 22, numChannels, 2);

    // 24–27 SampleRate (8000, 44100, etc)
    writeUIntLE(header + 24, sampleRate, 4);

    // 28–31 ByteRate
    writeUIntLE(header + 28, byteRate, 4);

    // 32–33 BlockAlign
    writeUIntLE(header + 32, blockAlign, 2);

    // 34–35 BitsPerSample
    writeUIntLE(header + 34, bitsPerSample, 2);


    // SUBCHUNKS - Subchunk 1

    // 36–39 "data"
    header[36] = 'd';
    header[37] = 'a';
    header[38] = 't';
    header[39] = 'a';

    // 40–43 Subchunk2Size
    writeUIntLE(header + 40, subchunk2Size, 4);
}



void addSample(int wave, int sample) {
   int sample_16 = (int) (sample * 32767);
//    data = struct.pack('<h', sample_16);
//    wave.extend(data)
}





int main() {
    unsigned int sampleRate = 44100;    // Sample rate in Hz. (CD quality)
    unsigned short channels = 1;        // // Mono
    unsigned short bits = 16;           // bits per sample
    double durationSeconds = 0.5;       // Length of tone
    unsigned int numSamples = (unsigned int) (durationSeconds * sampleRate);

    unsigned char header[44];

    makeWaveHeader(header, sampleRate, channels, bits, numSamples);

    std::ofstream outFile("example.wav", std::ios::binary);
    outFile.write((const char*) header, 44);


    

    // Add frequency samples
    // for (int i = 0; i < noSamples; i++) {
    //     float sample = std::cos(freq * i * 3.142 / sampleRate); 
    //     addSample(wave, sample);
    // }




    // then write sample data (2 bytes each for 16-bit mono)
    // ...

    
    outFile.close();
}
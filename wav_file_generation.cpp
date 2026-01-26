#include <cmath>    // calcualte sine wave
#include <cstring>  // strcmp, strcpy and strlen
#include <fstream>  // for files
#include <iostream>
#include <cctype>   // isdigit


static void buildWavFilename(char outName[37], const char baseName[33]) {
    /* 
        outName stores up to 32 letters + 4 (.wave) + 1 ('\0') = 37
    */
    std::strcpy(outName, baseName);
    std::strcat(outName, ".wav"); // TODO - don't know if i can use strcat()
}


static void writeUIntLE(unsigned char* out, unsigned int value, int byteSize) {
    /*
        Converts a sequence of bytes to little-endian format.
    */
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



static void addSample16LE(std::ofstream& outFile, int sample) {
    /*
        Writes a single 16-bit PCM audio sample to a file in little-endian format

        The input sample is expected to be a signed integer in the range [-32768, 32767].
        The value is clamped to this range, converted to its raw 16-bit representation,
        and written as two bytes (16 bits = short) (LSB first).
    */

    // clamp
    if (sample > 32767) sample = 32767;
    if (sample < -32768) sample = -32768;

    // interpret the signed value as raw 16-bit data
    unsigned short raw16 = (unsigned short) sample;

    // write sample as little-endian bytes
    unsigned char sampleBytes[2];
    writeUIntLE(sampleBytes, (unsigned int) raw16, 2);
    outFile.write((const char*) sampleBytes, 2);
}




bool isInt(const char* s) {
    // only digits

    if (*s == '\0') {
        return true;
    }
    for (int i = 0; s[i]; i++) {
        if (!isdigit(s[i])) {
            return false;
        }
    }
    return true;
}

bool isDouble(const char* s) {
    // contains a dot

    bool dotFound = false;

    for (int i = 0; s[i]; i++) {
        if (s[i] == '.') {

            if (dotFound) {
                // 2 dots
                return false;
            }
            dotFound = true;
        } else if (!isdigit(s[i])) {
            return false;
        }
    }
    return dotFound;
}

bool validateTerminalArgs(int argc, char *argv[]) {
    /* 
        Running the program on the terminal has to have three arguments,
        a string, an integer and a real number
    */
    if (argc != 4) {
        return false;
    }

    int hasStr = false;
    int hasInt = false;
    int hasReal = false;

    for (int i = 1; i < argc; i++) {
        if (isInt(argv[i])) {
            hasInt = true;
        } else if (isDouble(argv[i])) {
            hasReal = true;
        } else {
            hasStr = true;
        }
    }

    if (hasStr && hasInt && hasReal) {
        return true;
    }
    return false;
}


int main(int argc, char *argv[]) {
    // bool useTerminalArgs = validateTerminalArgs(argc, argv);
    // if (!useTerminalArgs) { get_usr_input(); }

    unsigned int sampleRate = 44100;    // Sample rate in Hz. (CD quality)
    unsigned short channels = 1;        // Mono
    unsigned short bits = 16;           // bits per sample

    double durationSeconds = 0.5;       // Length of tone
    unsigned int numSamples = (unsigned int) (durationSeconds * sampleRate);

    const int freq = 440;
    const double PI = 3.14159265358979323846;


    // build header
    unsigned char header[44];
    makeWaveHeader(header, sampleRate, channels, bits, numSamples);

    // write file
    std::ofstream outFile("cpp-example.wav", std::ios::binary);
    if (!outFile) {
        std::cout << "Could not open/create output file.\n";
        return 1;
    }
    outFile.write((const char*) header, 44);
    

    // write samples (frequency)
    for (unsigned int i = 0; i < numSamples; ++i) {
        double s = std::cos(2 * PI * freq * (double) i / (double) sampleRate); // [-1,1]
        int sample = (int)(s * 32767.0); // -32768 to 32767.
        addSample16LE(outFile, sample);
    }

    
    outFile.close();
    std::cout << "WAVE file written to cpp-example.wav\n";
    return 0;
}
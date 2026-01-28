#include <cmath>    // calcualte sine wave
#include <cstring>  // strcmp, strcpy and strlen
#include <fstream>  // for files
#include <iostream> // cout, cin
#include <cstdlib>  // atoi, atof


static void buildWavFilename(char outName[37], const char baseName[33]) {
    /* 
        outName stores up to 32 chars + 4 (.wave) + 1 ('\0') = 37
        baseName stores max 32 chars + '\0'
    */
    std::strcpy(outName, baseName);
    std::strcat(outName, ".wav");
}


static void writeUIntLE(unsigned char* out, unsigned int value, int byteSize) {
    /*
        Writes the lowest byteSize bytes of value into out in little-endian order
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




static bool readSongHeader(const char* textFilename, char baseName[33], int& bpm) {
    /*
        Reads the song text file and gets wav-filename and bpm

        format:
            output-wave-filename
            tempo-in-BPM
    */
    std::ifstream musicFile(textFilename);
    if (!musicFile) {
        std::cout << "Unable to open file: " << textFilename << "\n";
        return false;
    }

    musicFile >> baseName;   // first token
    musicFile >> bpm;        // second token

    if (bpm <= 0) {
        std::cout << "Incorrect file header (missing name or BPM <= 0)\n";
        return false;
    }

    return true;
}

static int computeTotalSamples(const char* textFilename, int& bpm, unsigned int sampleRate) {
    /*
        Reads the song text file, skip first 2 tokens
        and returns total number of audio samples.

        File format:
            remaining lines:
                note octave num den
            or silence:
                s num den
    */
    std::ifstream musicFile(textFilename);
    if (!musicFile) {
        std::cout << "Unable to open file: " << textFilename << "\n";
        return false;
    }
    // skip tokens
    char ignoreName[33];
    int fileBpm = 0;
    musicFile >> ignoreName;
    musicFile >> bpm;


    unsigned int totalSamples = 0;

    // read until EOF
    while (true) {
        char noteChar;
        if (!(musicFile >> noteChar)) break; // no more notes

        int num = 0;
        int den = 0;

        if (noteChar == 's') {

            // silence line: s <num> <den>
            if (!(musicFile >> num >> den)) {
                std::cout << "Incorrect silence line in file" << "\n";
                return 0;
            }

        } else {

            // note line: note octave num den
            int octave = 0;
            if (!(musicFile >> octave >> num >> den)) {
                std::cout << "Incorrect note line in file" << "\n";
                return 0;
            }
            (void) octave; // not needed for sample count

        }

        if (den == 0) {
            std::cout << "Invalid note length: den is 0" << "\n";
            return 0;
        }

        // beats = 4 * num/den (since 1/4 note = 1 beat)
        double beats = 4.0 * (double) num / (double) den;

        // seconds for this event (sec / bpm = sec per beat)
        double seconds = beats * (60.0 / (double) bpm);

        if (seconds < 0.0) seconds = 0.0;

        // convert to samples (rounded)
        unsigned int samples = (unsigned int) (seconds * (double) sampleRate + 0.5);

        totalSamples += samples;
    }

    return totalSamples;
}


static void addSample16LE(std::ofstream& waveFile, int sample) {
    /*
        Writes a single 16-bit PCM audio sample to a file in little-endian order

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
    waveFile.write((const char*) sampleBytes, 2);
}

static void writeToneSamples(std::ofstream& waveFile, double freq, unsigned int numSamples, unsigned int sampleRate) {
    // generate samples of a cosine wave at frequency
    const double PI = 3.14159265358979323846;

    for (unsigned int i = 0; i < numSamples; ++i) {
        double s = std::cos(2.0 * PI * freq * (double) i / (double) sampleRate); // [-1,1]
        int sample = (int)(s * 32767.0); // -32768 to 32767.
        addSample16LE(waveFile, sample);
    }
}

static double noteFrequency(char note, int octave) {
}

static void writeSilenceSamples(std::ofstream& waveFile, unsigned int count) {
    // silence in 16-bit PCM is sample value 0
    for (unsigned int i = 0; i < count; ++i) {
        addSample16LE(waveFile, 0);
    }
}


static double freqTableOctaveOne(char note) {
    // lowercase = base note
    // uppercase = sharp
    switch (note) {
        case 'a': return 440.0;
        case 'A': return 466.0;
        case 'b': return 494.0;
        case 'c': return 523.0;  // also B
        case 'C': return 554.0;
        case 'd': return 587.0;
        case 'D': return 622.0;
        case 'e': return 659.0;
        case 'f': return 698.0;  // also E
        case 'F': return 740.0;
        case 'g': return 784.0;
        case 'G': return 831.0;
        default:  return 0.0;
    }
}

static unsigned int durationToSamples(int num, int den, int bpm, unsigned int sampleRate) {
}

static int writeSongSamples(const char* textFilename, int& bpm, unsigned int sampleRate, std::ofstream& waveFile) {
    /*
        Reads the song text file, skip first 2 tokens and write song sample for each note
    */
    std::ifstream musicFile(textFilename);
    if (!musicFile) {
        std::cout << "Unable to open file: " << textFilename << "\n";
        return false;
    }
    // skip token
    char ignoreName[33];
    musicFile >> ignoreName;

    musicFile >> bpm;


    // read until EOF
    while (true) {
        char noteChar;
        if (!(musicFile >> noteChar)) break; // no more notes

        int num = 0;
        int den = 0;

        if (noteChar == 's') {

            // silence line: s <num> <den>
            if (!(musicFile >> num >> den)) {
                std::cout << "Incorrect silence line in file" << "\n";
                return 0;
            }
            unsigned int numSamples = durationToSamples(num, den, bpm, sampleRate);
            writeSilenceSamples(waveFile, numSamples);

        } else {

            // note: <note> <octave> <num> <den>
            int octave = 0;
            if (!(musicFile >> octave >> num >> den)) {
                std:: cout << "Incorrect note line in file" << "\n";
                return 0;
            }

            double freq = noteFrequency(noteChar, octave);
            if (freq <= 0.0) {
                std::cout << "Unknown note character: " << noteChar << "\n";
                return 0;
            }

            unsigned int count = durationToSamples(num, den, bpm, sampleRate);
            writeToneSamples(waveFile, freq, count, sampleRate);
        }
    }

    return 1; // success
}



int main(int argc, char *argv[]) {
    const unsigned int sampleRate = 44100;    // Sample rate in Hz. (CD quality)
    const unsigned short channels = 1;        // Mono
    const unsigned short bits = 16;           // bits per sample


    // read txt song file
    if (argc != 2) {
        std::cout << "Please write a text filename to read" << "\n";;
        std::cout << "Usage: " << argv[0] << " <songfile.txt>" << "\n";;
        return 1;
    }

    char baseName[33];                        // wave file name
    int bpm = 0;

    if (!readSongHeader(argv[1], baseName, bpm)) return 1;


    // build output wave filename
    char outName[37];
    buildWavFilename(outName, baseName);

    // build header
    unsigned int totalSamples = computeTotalSamples(argv[1], bpm, sampleRate);
    unsigned char header[44];
    makeWaveHeader(header, sampleRate, channels, bits, totalSamples);


    // write header
    std::ofstream waveFile(outName, std::ios::binary);
    if (!waveFile) {
        std::cout << "Could not open/create output file" << "\n";
        return 1;
    }
    waveFile.write((const char*) header, 44);



    writeSongSamples(argv[1], bpm, sampleRate, waveFile);


    
    waveFile.close();
    std::cout << "WAVE file written to " << outName << '\n';
    return 0;
}
    
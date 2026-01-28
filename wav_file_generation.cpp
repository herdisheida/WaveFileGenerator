#include <cmath>    // calcualte sine wave
#include <cstring>  // strcmp, strcpy and strlen
#include <fstream>  // for files
#include <iostream> // cout, cin


static void buildWavFilename(char outName[37], const char baseName[33]) {
    /* outName = baseName + ".wav" (max 32 chars + 4 + '\0') */
    std::strcpy(outName, baseName);
    std::strcat(outName, ".wav");
}

static void writeUIntLE(unsigned char* out, unsigned int value, int byteSize) {
    /* writes an integer into out in little-endian, using byteSize bytes */
    for (int i = 0; i < byteSize; i++) {
        out[i] = (unsigned char) (value & 0xFF);
        value >>= 8;
    }
}

static void makeWaveHeader( unsigned char header[44], unsigned int sampleRate, unsigned short numChannels, unsigned short bitsPerSample, unsigned int numSamples) {
    unsigned int byteRate = sampleRate * numChannels * (bitsPerSample / 8);
    unsigned short blockAlign = (unsigned short) (numChannels * (bitsPerSample / 8));
    unsigned int subchunk2Size = numSamples * numChannels * (bitsPerSample / 8);
    unsigned int chunkSize = 36 + subchunk2Size; // or 4 + (8 + SubChunk1Size) + (8 + SubChunk2Size)

    header[0] = 'R'; header[1] = 'I'; header[2] = 'F'; header[3] = 'F';
    writeUIntLE(header + 4, chunkSize, 4);
    header[8]  = 'W'; header[9]  = 'A'; header[10] = 'V'; header[11] = 'E';
    header[12] = 'f'; header[13] = 'm'; header[14] = 't'; header[15] = ' ';
    writeUIntLE(header + 16, 16, 4);
    writeUIntLE(header + 20, 1, 2);
    writeUIntLE(header + 22, numChannels, 2);
    writeUIntLE(header + 24, sampleRate, 4);
    writeUIntLE(header + 28, byteRate, 4);
    writeUIntLE(header + 32, blockAlign, 2);
    writeUIntLE(header + 34, bitsPerSample, 2);
    header[36] = 'd'; header[37] = 'a'; header[38] = 't'; header[39] = 'a';
    writeUIntLE(header + 40, subchunk2Size, 4);
}

static bool readSongHeader(const char* textFilename, char baseName[33], int& bpm) {
    /* read a wav-filename (without .wav) and bpm from a .txt file */
    std::ifstream musicFile(textFilename);
    if (!musicFile) {
        std::cout << "Unable to open file: " << textFilename << "\n";
        return false;
    }

    musicFile >> baseName;
    musicFile >> bpm;

    if (!musicFile || bpm <= 0) {
        std::cout << "Incorrect file header (missing name or BPM <= 0)" << '\n';
        return false;
    }
    return true;
}

static unsigned int durationToSamples(int num, int den, int bpm, unsigned int sampleRate) {
    /*  Convert a note length (num/den of a whole note) into sample count.
        Whole note = 4 beats. One beat lasts 60/bpm seconds. */
    if (den == 0 || bpm <= 0) return 0;

    double beats = 4.0 * (double) num / (double) den;  // 1/4 note = 1 beat
    double seconds = beats * (60.0 / (double) bpm);  // sec / bpm = sec per beat
    if (seconds < 0.0) seconds = 0.0;
    return (unsigned int) (seconds * (double) sampleRate); // convert to samples
}


static unsigned computeTotalSamples(const char* textFilename, int& bpm, unsigned int sampleRate) {
    /* read the whole song file and sum how many samples it will need. */
    std::ifstream musicFile(textFilename);
    if (!musicFile) {
        std::cout << "Unable to open file: " << textFilename << "\n";
        return 0;
    }

    // skip name + bpm
    char ignoreName[33];
    musicFile >> ignoreName;
    musicFile >> bpm;
    if (!musicFile || bpm <= 0) {
        std::cout << "Incorrect file header (missing name or BPM <= 0)" << '\n';
        return 0;
    }

    unsigned int totalSamples = 0;
    char note;
    while (musicFile >> note) { // read until EOF
        int num = 0, den = 0;

        if (note == 's') {
            // silence: s <num> <den>
            if (!(musicFile >> num >> den)) {
                std::cout << "Incorrect silence line in file" << "\n";
                return 0;
            }
        } else {
            // note line: <note> <octave> <num> <den>
            int octave = 0;
            if (!(musicFile >> octave >> num >> den)) {
                std::cout << "Incorrect note line in file" << "\n";
                return 0;
            }
            (void) octave; // not needed for sample count
        }
        totalSamples += durationToSamples(num, den, bpm, sampleRate);
    }
    return totalSamples;
}

static void addSampleLE(std::ofstream& waveFile, int sample) {
    /* write one 16-bit audio sample in little-endian format */
    if (sample > 32767) sample = 32767;
    if (sample < -32768) sample = -32768;

    unsigned short raw16 = (unsigned short) sample; // WAV file wants raw 16-bit data
    // write sample as little-endian bytes
    unsigned char sampleBytes[2];
    writeUIntLE(sampleBytes, (unsigned int) raw16, 2);
    waveFile.write((const char*) sampleBytes, 2);
}

static void writeSilenceSamples(std::ofstream& waveFile, unsigned int count) {
    /* write silent samples (value 0). */
    for (unsigned int i = 0; i < count; i++) {
        addSampleLE(waveFile, 0);
    }
}


static double freqTableOctaveOne(char note) {
    /* Frequency table for octave 1 */
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

static double noteFrequency(char note, int octave) {
    /* convert note and octave to Hz */
    double f = freqTableOctaveOne(note);
    if (f <= 0.0) { return 0.0; }

    int diff = octave - 1; // how far from octave 1

    // each octave up doubles frequency and each octave down halves it
    while (diff > 0) { f *= 2.0; --diff; }
    while (diff < 0) { f /= 2.0; ++diff; }
    return f;
}

static void writeToneSamples(std::ofstream& waveFile, double freq, unsigned int numSamples, unsigned int sampleRate) {
    /* write samples of a cos wave at frequency freq */
    const double PI = 3.14159265358979323846;

    for (unsigned int i = 0; i < numSamples; i++) {
        double s = std::cos(2 * PI * freq * (double) i / (double) sampleRate); // [-1,1]
        int sample = (int) (s * 32767.0); // WAV only allows -32768 to 32767.
        addSampleLE(waveFile, sample);
    }
}

static bool writeSongSamples(const char* textFilename, int& bpm, unsigned int sampleRate, std::ofstream& waveFile) {
    /* Reads the song file and write all audio samples to the WAV file */
    std::ifstream musicFile(textFilename);
    if (!musicFile) {
        std::cout << "Unable to open file: " << textFilename << "\n";
        return false;
    }
    // skip name + read bpm
    char ignoreName[33];
    musicFile >> ignoreName;
    musicFile >> bpm;
    if (!musicFile || bpm <= 0) {
        std::cout << "Bad file header (missing name or bpm)" << '\n';
        return false;
    }

    char note;
    while (musicFile >> note) {
        int num = 0, den = 0;

        if (note == 's') {
            // silence: s <num> <den>
            if (!(musicFile >> num >> den)) {
                std::cout << "Incorrect silence line in file" << "\n";
                return false;
            }
            unsigned int numSamples = durationToSamples(num, den, bpm, sampleRate);
            writeSilenceSamples(waveFile, numSamples);

        } else {
            // note: <note> <octave> <num> <den>
            int octave = 0;
            if (!(musicFile >> octave >> num >> den)) {
                std:: cout << "Incorrect note line in file" << "\n";
                return false;
            }

            double freq = noteFrequency(note, octave);
            if (freq <= 0.0) {
                std::cout << "Unknown note character: " << note << "\n";
                return false;
            }

            unsigned int count = durationToSamples(num, den, bpm, sampleRate);
            writeToneSamples(waveFile, freq, count, sampleRate);
        }
    }
    return true; // success
}


int main(int argc, char *argv[]) {
    const unsigned int sampleRate = 44100;    // Sample rate in Hz. (CD quality)
    const unsigned short channels = 1;        // Mono
    const unsigned short bits = 16;           // bits per sample

    // read txt song file
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <songfile.txt>" << "\n";;
        return 1;
    }
    char baseName[33];  // wave file name
    int bpm = 0;
    if (!readSongHeader(argv[1], baseName, bpm)) return 1;

    // build output wave filename
    char outName[37];
    buildWavFilename(outName, baseName);

    // build header
    unsigned int totalSamples = computeTotalSamples(argv[1], bpm, sampleRate);
    if (totalSamples <= 0) {
        std::cout << "Could not compute samples (file error)" << '\n';
        return 1;
    }
    unsigned char header[44];
    makeWaveHeader(header, sampleRate, channels, bits, totalSamples);

    // write header
    std::ofstream waveFile(outName, std::ios::binary);
    if (!waveFile) {
        std::cout << "Could not create output file" << "\n";
        return 1;
    }

    waveFile.write((const char*) header, 44);

    if (!writeSongSamples(argv[1], bpm, sampleRate, waveFile)) {
        std::cout << "Error while writing samples" << '\n';
        return 1;
    }
        
    waveFile.close();
    std::cout << "WAV file created: " << outName << "\n";
    return 0;
}
    
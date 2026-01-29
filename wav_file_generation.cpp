#include <cmath>    // calcualte cos wave
#include <cstring>  // strcmp, strcpy and strlen
#include <fstream>  // for files
#include <iostream> // cout, cin


void writeUnsignedInt(std::ofstream& out, unsigned int x) {
    /* write unsigned int in little-endian form */
    out.put((unsigned char)( x        & 0xFF));
    out.put((unsigned char)((x >> 8 ) & 0xFF));
    out.put((unsigned char)((x >> 16) & 0xFF));
    out.put((unsigned char)((x >> 24) & 0xFF));
}
void writeUnsignedShort(std::ofstream& out, unsigned short x) {
    /* write unsigned short in little-endian form */
    out.put((unsigned char)( x       & 0xFF));
    out.put((unsigned char)((x >> 8) & 0xFF));
}
void writeShort(std::ofstream&  out, short x) {
    /* write signed short (16 bit) as raw bytes in  little-endian form */
    unsigned short raw = (unsigned short) x;
    out.put((unsigned char)( raw       & 0xFF));
    out.put((unsigned char)((raw >> 8) & 0xFF));
}
void writeFourChars(std::ofstream& out, const char x[]) {
    out.write(x, 4);
}

void writeWaveHeader(std::ofstream& out, unsigned int totalSamples, unsigned int sampleRate) {
    const unsigned int bitsSample = 16;       // bits per sample
    const unsigned short noChannels = 1;      // Mono
    unsigned int subchunk1Size = 16;

    const unsigned short blockAlign = noChannels * bitsSample / 8;
    const unsigned int byteRate = sampleRate * noChannels * bitsSample / 8;
    const unsigned int subchunk2Size = totalSamples * noChannels * bitsSample / 8;

    const char RIFF[4] = {'R', 'I', 'F', 'F'};
    const char WAVE[4] = {'W', 'A', 'V', 'E'};
    const char FMT[4]  = {'f', 'm', 't', ' '};
    const char DATA[4] = {'d', 'a', 't', 'a'};

    writeFourChars(out, RIFF);                     // ChunkId
    writeUnsignedInt(out, 36 + subchunk2Size);     // ChunkSize
    writeFourChars(out, WAVE);                     // Format
    writeFourChars(out, FMT);                      // Subchunk1ID
    writeUnsignedInt(out, subchunk1Size);          // Subchunk1Size
    writeUnsignedShort(out, 1);                    // AudioFormat
    writeUnsignedShort(out, noChannels);           // NumChannels
    writeUnsignedInt(out, sampleRate);             // SampleRate
    writeUnsignedInt(out, byteRate);               // ByteRate
    writeUnsignedShort(out, blockAlign);           // BLockAlign
    writeUnsignedShort(out, bitsSample);           // BitsPerSimple
    writeFourChars(out, DATA);                     // Subchunk2ID
    writeUnsignedInt(out, subchunk2Size);          // Subchunk2Size
}

static double getBaseFrequency(char note) {
    /* Frequency table for octave 1 */
    switch (note) {
        case 'a': return 440.0;
        case 'A': return 466.0;
        case 'b': return 494.0;
        case 'B': return 523.0;
        case 'c': return 523.0;
        case 'C': return 554.0;
        case 'd': return 587.0;
        case 'D': return 622.0;
        case 'e': return 659.0;
        case 'E': return 698.0;
        case 'f': return 698.0;
        case 'F': return 740.0;
        case 'g': return 784.0;
        case 'G': return 831.0;
        default:  return 0.0;
    }
}

static double getFrequency(char note, int octave) {
    /* convert note and octave to Hz */
    if (note == 's') return 0.0;
    double base_freq = getBaseFrequency(note);

    // each octave up doubles frequency and each octave down halves it
    if (octave == 0) return base_freq / 2;
    return base_freq * pow(2, octave - 1);
}

int getSampleCount(int num, int den, int& bpm, unsigned int sampleRate) {
    /*  Convert a note length (num/den of a whole note) into sample count */
    const double beats = 4.0 * (double) num / (double) den;  // whole note = 4 beats
    double seconds = beats * (60.0 / (double) bpm);    // seconds per beat = 60 bpm
    if (seconds < 0.0) seconds = 0.0;
    return (unsigned int) (seconds * (double) sampleRate + 0.5); // convert to samples
}

void writeData(std::ofstream& outs, unsigned int nunSamples, double freq, unsigned int sampleRate) {
    const double PI = 3.14159265358979323846;

    for (unsigned int i = 0; i < nunSamples; i++) {
        double sample = 0;
        if (freq != 0.0) {
            sample = std::cos(2.0 * PI * freq * (double) i / (double) sampleRate); // [-1,1]
        }
        int s = (int) (sample * 32767.0);  // WAV only allows -32768 to 32767.
        if (s > 32767) s = 32767;
        if (s < -32768) s = -32768;
        writeShort(outs, (short) s);
    }
}


int main(int argc, char *argv[]) {
    const unsigned int sampleRate = 44100;    // Sample rate in Hz. (CD quality)
    char wavFilename[512];
    int bpm = 0;

    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <songfile.txt>" << '\n';
        return 1;
    }

    std::ifstream musicFile(argv[1]);
    if (!musicFile) {
        std::cout << "Unable to open file: " << argv[1] << '\n';
        return 1;
    }

    // get song header
    musicFile >> wavFilename;
    musicFile >> bpm;


    int nameLength = strlen(wavFilename);
    if (
        nameLength > 32 || (
        wavFilename[nameLength - 1] == 'v' &&
        wavFilename[nameLength - 2] == 'a' &&
        wavFilename[nameLength - 3] == 'w' &&
        wavFilename[nameLength - 4] == '.'
        )
    ) {
        std::cout << "Incorrect WAV filename format:" << '\n';
        std::cout << "  32 char max, don't write the file extension (.wav)" << '\n';
        return 1;   
    }

    std::strcat(wavFilename, ".wav");  // add .wav

    // get song data
    char note;
    int octave;
    int numerator;
    int denominator;

    double frequencies[1024];
    int samples[1024];
    int numSound = 0;
    int totalSamples = 0;

    while (musicFile >> note) {
        if (note != 's') {
            musicFile >> octave;
        }

        if (!(musicFile >> numerator >> denominator)) {
            std::cout << "Incorrect note format in txt song file" << '\n';
            return 1;
        }

        frequencies[numSound] = getFrequency(note, octave);
        samples[numSound] = getSampleCount(numerator, denominator, bpm, sampleRate);
        totalSamples += samples[numSound];
        numSound++;
    }
    musicFile.close();


    // create wav file
    std::ofstream waveFile(wavFilename, std::ios::binary);
    writeWaveHeader(waveFile, totalSamples, sampleRate); // header

    for (int i = 0; i < numSound; i++) {
        writeData(waveFile, samples[i], frequencies[i], sampleRate); // data (Hz frequencies)
    }

    waveFile.close();
    std::cout << "WAV file created: " << wavFilename << '\n';
    return 0;
}

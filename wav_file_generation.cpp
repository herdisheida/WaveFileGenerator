#include <cmath>    // calcualte sine wave
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
    // out.put(x & 0x00FF);
    // out.put((x & 0xFF00) >> 8);
    unsigned short raw = (unsigned short) x;
    out.put((unsigned char)( raw       & 0xFF));
    out.put((unsigned char)((raw >> 8) & 0xFF));
}

void writeFourChars(std::ofstream& out, const char x[]) {
    out.write(x, 4);
}

void writeWaveHeader(std::ofstream& out, unsigned int totalSamples, unsigned int sampleRate) {
    unsigned int bitsSample = 16;       // bits per sample
    unsigned short noChannels = 1;      // Mono
    unsigned int subchunk1Size = 16;

    unsigned short blockAlign = noChannels * bitsSample / 8;
    unsigned int byteRate = sampleRate * noChannels * bitsSample / 8;
    unsigned int subchunk2Size = totalSamples * noChannels * bitsSample / 8;

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
    /*  Convert a note length (num/den of a whole note) into sample count.
        Whole note = 4 beats. One beat lasts 60/bpm seconds. */
    double beats = 4.0 * (double) num / (double) den;  // whole note = 4 beats
    double seconds = beats * (60.0 / (double) bpm);    // seconds per beat = 60 bpm
    if (seconds < 0.0) seconds = 0.0;
    return (unsigned int) (seconds * (double) sampleRate + 0.5); // convert to samples
}


void writeData(std::ofstream& outs, unsigned int nunSamples, double freq, unsigned int sampleRate, double freq2) {
    const double PI = 3.14159265358979323846;


    for (unsigned int i = 0; i < nunSamples; i++) {

        double sample = 0;
        double sample1 = 0;
        double sample2 = 0;

        if (freq != 0.0) {

            // TODO add 2.0 * because im testing í canvas þarf þá að sleppa því þar
            sample1 = std::cos(PI * freq * (double) i / (double) sampleRate); // [-1,1]

            sample2 = std::cos(PI * freq2 * (double) i / (double) sampleRate);

            sample = (sample1 + sample2) / 2;
        }

        int s = (int) (sample * 32767.0);  // WAV only allows -32768 to 32767.
        if (s > 32767) s = 32767;
        if (s < -32768) s = -32768;

        writeShort(outs, (short) s);
    }
}



int main(int argc, char *argv[]) {
    const unsigned int sampleRate = 44100;    // Sample rate in Hz. (CD quality)

    // song header
    char wavFilename[37]; // 32 + "".wav" + '\n'
    int bpm = 0;

    std::ifstream musicFile2;  // declare here
    std::ifstream musicFile;
    bool harmonize = false;

    // -h mode
    if (argc == 4 && std::strcmp(argv[1], "-h") == 0) {
        musicFile2.open(argv[3]);
        if (!musicFile2) {
            std::cout << "Unable to open file: " << argv[3] << "\n";
            return 1;
        }
        musicFile.open(argv[2]);
        if (!musicFile) {
            std::cout << "Unable to open file: " << argv[2] << "\n";
            return 1;
        }
        harmonize = true;

    }
    
    if (argc != 2 && std::strcmp(argv[1], "-h") != 0) {
        std::cout << "Usage:\n";
        std::cout << "  " << argv[0] << " <songA.txt>\n";
        std::cout << "  " << argv[0] << " -h <songB.txt> <songC.txt>\n";
        return 1;
    }


    if (!harmonize) {
        musicFile.open(argv[1]);
        if (!musicFile) {
            std::cout << "Unable to open file: " << argv[1] << "\n";
            return 1;
        }
    }


    if (harmonize) { musicFile2 >> wavFilename >> bpm; }  // skip

    musicFile >> wavFilename;
    std::strcat(wavFilename, ".wav");  // add .wav

    musicFile >> bpm;

    // get song data 1
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
        musicFile >> numerator;
        musicFile >> denominator;

        frequencies[numSound] = getFrequency(note, octave);
        samples[numSound] = getSampleCount(numerator, denominator, bpm, sampleRate);
        totalSamples += samples[numSound];
        numSound++;
    }
    musicFile.close();


    // song data 2
    char note2;
    int octave2;
    int numerator2;
    int denominator2;

    double frequencies2[1024];
    int samples2[1024];
    int numSound2 = 0;
    int totalSamples2 = 0;

    while (musicFile2 >> note2) {
        if (note2 != 's') {
            musicFile2 >> octave2;
        }
        musicFile2 >> numerator2;
        musicFile2 >> denominator2;

        frequencies[numSound2] = getFrequency(note, octave2);
        samples[numSound2] = getSampleCount(numerator2, denominator2, bpm, sampleRate);
        totalSamples2 += samples2[numSound2];
        numSound2++;
    }
    musicFile2.close();


    // create wav file
    std::ofstream waveFile(wavFilename, std::ios::binary);

    // write header
    if (harmonize && totalSamples2 > totalSamples) {

        // harmonize
        writeWaveHeader(waveFile, totalSamples2, sampleRate);

        // write data
        for (int i = 0; i < numSound2; i++) {
            writeData(waveFile, samples2[i], frequencies[i], sampleRate, frequencies2[i]);
        }
    
    } else {
        // normal mode
        writeWaveHeader(waveFile, totalSamples, sampleRate);
    
        // write data
        for (int i = 0; i < numSound; i++) {
            writeData(waveFile, samples[i], frequencies[i], sampleRate, frequencies2[i]);
        }
    }


    // close
    waveFile.close();
    std::cout << "WAV file created: " << wavFilename << "\n";
    return 0;
}
    
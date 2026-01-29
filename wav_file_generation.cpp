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


void writeData(std::ofstream& outs, unsigned int sampleCount, double freq1, unsigned int sampleRate, double freq2) {
    const double PI = 3.14159265358979323846;

    for (unsigned int i = 0; i < sampleCount; ++i) {
        double s1 = 0.0;
        double s2 = 0.0;

        if (freq1 != 0.0) {
            s1 = std::cos(2.0 * PI * freq1 * (double) i / (double) sampleRate);
        }
        if (freq2 != 0.0) {
            s2 = std::cos(2.0 * PI * freq2 * (double) i / (double) sampleRate);
        }

        // Mix: if both active, average; if only one active, keep it.
        double mixed;
        if (freq1 != 0.0 && freq2 != 0.0) {
            mixed = (s1 + s2) / 2;
        } else {
            mixed = (s1 + s2); // one of them is 0
        }

        int s = (int) (mixed * 32767.0);
        if (s > 32767) s = 32767.0;
        if (s < -32768) s = -32768.0;

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


    // decide mode + open files
    if (argc == 4 && std::strcmp(argv[1], "-h") == 0) {
        harmonize = true;

        musicFile.open(argv[2]);
        if (!musicFile) {
            std::cout << "Unable to open file: " << argv[2] << "\n";
            return 1;
        }
        musicFile2.open(argv[3]);
        if (!musicFile2) {
            std::cout << "Unable to open file: " << argv[3] << "\n";
            return 1;
        }
    } else if (argc == 2) {
        musicFile.open(argv[1]);
        if (!musicFile) {
            std::cout << "Unable to open file: " << argv[1] << "\n";
            return 1;
        }
    } else {
        std::cout << "Usage:\n";
        std::cout << "  " << argv[0] << " <songA.txt>\n";
        std::cout << "  " << argv[0] << " -h <songB.txt> <songC.txt>\n";
        return 1;
    }



    // get wav filename + bpm
    musicFile >> wavFilename >> bpm;
    if (!musicFile || bpm <= 0) {
        std::cout << "Bad header in first file\n";
        return 1;
    }
    std::strcat(wavFilename, ".wav");  // add .wav


    // if harmony - read annd validate header from file2
    int bpm2 = 0;
    if (harmonize) {
        char ignoreName2[33];
        musicFile2 >> ignoreName2 >> bpm2;
        if (!musicFile2 || bpm2 <= 0) {
            std::cout << "Bad header in second file" << '\n';
            return 1;
        }
        if (bpm2 != bpm) {
            std::cout << "BPM mismatch between files (" << bpm << " vs " << bpm2 << ")" << '\n';
            return 1;
        }
    }

    // get song data 1
    double frequencies[1024];
    int samples[1024];
    int numSound = 0;
    unsigned int totalSamples = 0;

    char note;
    while (musicFile >> note) {
        int octave = 1; // default so silence doesn't use garbage
        int numerator = 0, denominator = 0;

        if (note != 's') {
            if (!(musicFile >> octave >> numerator >> denominator)) break;
        } else {
            if (!(musicFile >> numerator >> denominator)) break;
        }

        if (numSound >= 1024) {
            std::cout << "Too many events (max 1024)\n";
            return 1;
        }

        frequencies[numSound] = getFrequency(note, octave);
        samples[numSound] = getSampleCount(numerator, denominator, bpm, sampleRate);
        totalSamples += (unsigned int)samples[numSound];
        ++numSound;
    }
    musicFile.close();

    // song data 2
    double frequencies2[1024];
    int samples2[1024];
    int numSound2 = 0;
    unsigned int totalSamples2 = 0;

    if (harmonize) {
        char noteB;
        while (musicFile2 >> noteB) {
            int octaveB = 1;
            int numeratorB = 0, denominatorB = 0;

            if (noteB != 's') {
                if (!(musicFile2 >> octaveB >> numeratorB >> denominatorB)) break;
            } else {
                if (!(musicFile2 >> numeratorB >> denominatorB)) break;
            }

            if (numSound2 >= 1024) {
                std::cout << "Too many events in file2 (max 1024)\n";
                return 1;
            }

            frequencies2[numSound2] = getFrequency(noteB, octaveB);
            samples2[numSound2] = getSampleCount(numeratorB, denominatorB, bpm, sampleRate);
            totalSamples2 += (unsigned int)samples2[numSound2];
            ++numSound2;
        }
        musicFile2.close();
    }


    // decide output length
    unsigned int outTotal = totalSamples;
    if (harmonize && totalSamples2 > outTotal) outTotal = totalSamples2;

    // create wav file
    std::ofstream waveFile(wavFilename, std::ios::binary);
    if (!waveFile) {
        std::cout << "Could not create output file" << "\n";
        return 1;
    }

    // write data
    writeWaveHeader(waveFile, outTotal, sampleRate);

    // write data
    if (!harmonize) {
        for (int i = 0; i < numSound; ++i) {
            writeData(waveFile, (unsigned int) samples[i], frequencies[i], sampleRate, 0.0);
        }
    } else {
        // if files differ in number of samples, the extra samples will be played alone
        int maxEvents = (numSound > numSound2) ? numSound : numSound2;

        for (int i = 0; i < maxEvents; ++i) {
            unsigned int n1 = (i < numSound)  ? (unsigned int) samples[i]  : 0;
            unsigned int n2 = (i < numSound2) ? (unsigned int) samples2[i] : 0;

            // use max duration for this sample slot
            unsigned int sampleCount = (n1 > n2) ? n1 : n2;

            double f1 = (i < numSound)  ? frequencies[i]  : 0.0;
            double f2 = (i < numSound2) ? frequencies2[i] : 0.0;

            writeData(waveFile, sampleCount, f1, sampleRate, f2);
        }
    }

    // close
    waveFile.close();
    std::cout << "WAV file created: " << wavFilename << "\n";
    return 0;
}
    
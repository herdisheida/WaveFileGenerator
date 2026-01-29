#include <cmath>    // calcualte sine wave
#include <cstring>  // strcmp, strcpy and strlen
#include <fstream>  // for files
#include <iostream> // cout, cin


char RIFF[4] = {'R', 'I', 'F', 'F'};
char WAVE[4] = {'W', 'A', 'V', 'E'};
char FMT[4]  = {'f', 'm', 't', ' '};
char DATA[4] = {'d', 'a', 't', 'a'};


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
    out.put(x & 0x00FF);
    out.put((x & 0xFF00) >> 8);
}

void writeFourChars(std::ofstream& out, char x[]) {
    for (int i = 0; i < 4; i++) out.put(x[i]);
}

void writeWaveHeader(std::ofstream& out, unsigned int totalSamples, unsigned int sampleRate) {
    unsigned int bitsSample = 16;       // bits per sample
    unsigned short noChannels = 1;      // Mono
    unsigned int subchunk1Size = 16;

    unsigned short blockAlign = noChannels * bitsSample / 8;
    unsigned int byteRate = sampleRate * noChannels * bitsSample / 8;
    unsigned int subchunk2Size = totalSamples * noChannels * bitsSample / 8;

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


void writeData(std::ofstream& outs, unsigned int nunSamples, double freq, unsigned int sampleRate) {
    const double PI = 3.14159265358979323846;


    for (unsigned int i = 0; i < nunSamples; i++) {
        double sample = 0;
        if (freq != 0.0) {
            // TODO add 2.0 * because im testing í canvas þarf þá að sleppa því þar
            sample = std::cos(PI * freq * (double) i / (double) sampleRate); // [-1,1]
        }

        // TODO short? -- not int?
        short shortSample = (short) (32767.0 * sample);  // WAV only allows -32768 to 32767.
        writeShort(outs, shortSample);
    }
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



int getSampleCount(int num, int den, int bpm, unsigned int sampleRate) {
    /*  Convert a note length (num/den of a whole note) into sample count.
        Whole note = 4 beats. One beat lasts 60/bpm seconds. */

    // TODO delete?
    // return 44100 * numerator * 60 * 4 / ( bpm  * denominator) ;

    double beats = 4.0 * (double) num / (double) den;  // 1/4 note = 1 beat
    double seconds = beats * (60.0 / (double) bpm);  // sec / bpm = sec per beat
    if (seconds < 0.0) seconds = 0.0;
    return (unsigned int) (seconds * (double) sampleRate); // convert to samples
}


int main(int argc, char *argv[]) {
    const unsigned int sampleRate = 44100;    // Sample rate in Hz. (CD quality)


    // read terminal
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <songfile.txt>" << "\n";;
        return 1;
    }

    std::ifstream musicFile(argv[1]);
    if (!musicFile) {
        std::cout << "Unable to open file: " << argv[1] << "\n";
        return 1;
    }

    // get song header
    char wavFilename[37]; // 32 + "".wav" + '\n'
    int bpm = 0;

    musicFile >> wavFilename;
    std::strcat(wavFilename, ".wav");  // add .wav

    musicFile >> bpm;

    std::cout << "WAV file to write: " << wavFilename << '\n';
    std::cout << "Beats per minute: " << bpm << '\n';


    // get song data
    char note;
    int octave;
    int numerator;
    int denominator;

    double frequencies[1024];
    int samples[1024];
    int numSound = 0;
    int totalSamples = 0;

    while (!musicFile.eof()) {
        musicFile >> note;
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


    // create wav file
    std::ofstream waveFile(wavFilename, std::ios::binary);

    // write header
    writeWaveHeader(waveFile, totalSamples, sampleRate);

    // write data
    for (int i = 0; i < numSound; i++) {
        writeData(waveFile, samples[i], frequencies[i], sampleRate);
    }

    // close
    waveFile.close();
    std::cout << "WAV file created: " << wavFilename << "\n";
    return 0;
}
    
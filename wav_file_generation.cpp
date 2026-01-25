#include <cmath>    // calcualte sine wave
#include <cstring>  // strcmp, strcpy and strlen
#include <fstream>  // for files




// Students are expected to program the actual packing directly from numeric values into the character array/arrays.  The python example uses an external library for that, but we will not.

// Note that you don’t have to write everything to the file at once.  You can reuse a static array and write a large file in chunks.  Also note that in this program your arrays may actually be enormous compared to the data in assignments you have done previously in programming courses.

int makeWaveHeader(int sampleRate, int oChannels, int bitsSample) {

    int wave;
    return wave;
}


void addSample(int wave, int sample) {
   int sample_16 = (int) (sample * 32767);
//    data = struct.pack('<h', sample_16);
//    wave.extend(data)
}


int *writeLittleEndian(int byteSize, int* ptr) {
    // TODO
}

int main() {
   int sampleRate = 44100;      // Sample rate in Hz. (CD quality)
   int freq = 440;              // A above middle C
   float duration = 0.5f;       // Length of tone - seconds
   int noChannels = 1;          // Mono

   int noSamples  = (int) (duration * sampleRate);   // Total number of samples for file

   // Create the header for the wave file
   int wave = makeWaveHeader(sampleRate, noChannels, 16);

   // Add frequency samples
   for (int i = 0; i < noSamples; i++) {
       float sample = std::cos(freq * i * 3.142 / sampleRate); 
       addSample(wave, sample);
   }


   // Update chunk and subchunk sizes
//    int dataBytes   = (int) (duration * sampleRate * noChannels * 16 / 8);
//    wave[40:44] = dataBytes.to_bytes(4,'little');          // Sub Chunk size
//    dataBytes += 4 + 8 + 16 + 8;                           // Header size
//    wave[4:8] = dataBytes.to_bytes(4, 'little');

//    // write out to file
//    fptr = open("example.wav", "w+b");
//    fptr.write(wave);
//    fptr.close();
}
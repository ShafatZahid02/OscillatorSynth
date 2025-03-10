// oscillator.cpp
#include <cmath>
#include <iostream>
#include <vector>
#include <array>
#include <portaudio.h>

// Oscillator Class
class Oscillator {
public:
    enum WaveType { SINE, SQUARE, SAWTOOTH };

    Oscillator(WaveType type, double frequency, double amplitude, double attack, double decay, double sustain, double release)
        : type(type), frequency(frequency), amplitude(amplitude), attack(attack), decay(decay), sustain(sustain), release(release), time(0.0) {}

    double getSample() {
        double value = 0.0;
        double envelope = getEnvelope();

        switch (type) {
            case SINE:
                value = std::sin(2 * M_PI * frequency * time);
                break;
            case SQUARE:
                value = (std::sin(2 * M_PI * frequency * time) > 0) ? 1.0 : -1.0;
                break;
            case SAWTOOTH:
                value = 2 * (time * frequency - std::floor(time * frequency + 0.5));
                break;
        }

        time += 1.0 / 44100.0; // Increment time based on sample rate (44.1 kHz)
        return value * amplitude * envelope;
    }

private:
    WaveType type;
    double frequency;
    double amplitude;
    double attack, decay, sustain, release;
    double time;

    double getEnvelope() {
        if (time < attack) {
            return time / attack;
        } else if (time < attack + decay) {
            return 1.0 - (1.0 - sustain) * ((time - attack) / decay);
        } else {
            return sustain * std::exp(-release * (time - attack - decay));
        }
    }
};

// Delay Effect Class
class DelayEffect {
public:
    DelayEffect(double delayTime, double feedback, double mix)
        : delayTime(delayTime), feedback(feedback), mix(mix), bufferSize(static_cast<size_t>(delayTime * 44100)), bufferIndex(0) {
        buffer.resize(bufferSize, 0.0);
    }

    double process(double input) {
        double delayedSample = buffer[bufferIndex];
        buffer[bufferIndex] = input + delayedSample * feedback;
        bufferIndex = (bufferIndex + 1) % bufferSize;
        return input * (1.0 - mix) + delayedSample * mix;
    }

private:
    double delayTime; // Delay time in seconds
    double feedback;  // Feedback amount (0.0 to 1.0)
    double mix;       // Dry/wet mix (0.0 = dry, 1.0 = wet)
    std::vector<double> buffer;
    size_t bufferSize;
    size_t bufferIndex;
};

// Reverb Effect Class
class ReverbEffect {
public:
    ReverbEffect(double decay, double mix)
        : decay(decay), mix(mix), combFilters{
            CombFilter(0.0297, decay),
            CombFilter(0.0371, decay),
            CombFilter(0.0411, decay),
            CombFilter(0.0437, decay)
        }, allPassFilters{
            AllPassFilter(0.005, 0.7),
            AllPassFilter(0.0017, 0.7)
        } {}

    double process(double input) {
        double output = 0.0;
        for (auto& comb : combFilters) {
            output += comb.process(input);
        }
        for (auto& allPass : allPassFilters) {
            output = allPass.process(output);
        }
        return input * (1.0 - mix) + output * mix;
    }

private:
    class CombFilter {
    public:
        CombFilter(double delayTime, double decay)
            : bufferSize(static_cast<size_t>(delayTime * 44100)), bufferIndex(0), decay(decay) {
            buffer.resize(bufferSize, 0.0);
        }

        double process(double input) {
            double output = buffer[bufferIndex];
            buffer[bufferIndex] = input + output * decay;
            bufferIndex = (bufferIndex + 1) % bufferSize;
            return output;
        }

    private:
        std::vector<double> buffer;
        size_t bufferSize;
        size_t bufferIndex;
        double decay;
    };

    class AllPassFilter {
    public:
        AllPassFilter(double delayTime, double decay)
            : bufferSize(static_cast<size_t>(delayTime * 44100)), bufferIndex(0), decay(decay) {
            buffer.resize(bufferSize, 0.0);
        }

        double process(double input) {
            double bufferOutput = buffer[bufferIndex];
            double output = -input + bufferOutput;
            buffer[bufferIndex] = input + bufferOutput * decay;
            bufferIndex = (bufferIndex + 1) % bufferSize;
            return output;
        }

    private:
        std::vector<double> buffer;
        size_t bufferSize;
        size_t bufferIndex;
        double decay;
    };

    double decay; // Reverb decay time
    double mix;   // Dry/wet mix (0.0 = dry, 1.0 = wet)
    std::array<CombFilter, 4> combFilters;
    std::array<AllPassFilter, 2> allPassFilters;
};

// Global oscillators and effects
std::vector<Oscillator> oscillators;
DelayEffect delay(0.5, 0.5, 0.5); // Default delay settings
ReverbEffect reverb(0.7, 0.5);    // Default reverb settings

// PortAudio callback function
static int audioCallback(const void* inputBuffer, void* outputBuffer,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void* userData) {
    float* out = (float*)outputBuffer;
    for (unsigned long i = 0; i < framesPerBuffer; i++) {
        double sample = 0.0;
        for (auto& osc : oscillators) {
            sample += osc.getSample();
        }

        // Apply effects
        sample = delay.process(sample);
        sample = reverb.process(sample);

        *out++ = static_cast<float>(sample); // Left channel
        *out++ = static_cast<float>(sample); // Right channel
    }
    return paContinue;
}

// Initialize PortAudio
void initializeAudio() {
    Pa_Initialize();
    PaStream* stream;
    Pa_OpenDefaultStream(&stream, 0, 2, paFloat32, 44100, 256, audioCallback, nullptr);
    Pa_StartStream(stream);
}

// Cleanup PortAudio
void cleanupAudio() {
    Pa_Terminate();
}

// JNI functions
extern "C" {
    void addOscillator(int type, double frequency, double amplitude, double attack, double decay, double sustain, double release) {
        oscillators.push_back(Oscillator(static_cast<Oscillator::WaveType>(type), frequency, amplitude, attack, decay, sustain, release));
    }

    void setDelayParameters(double delayTime, double feedback, double mix) {
        delay = DelayEffect(delayTime, feedback, mix);
    }

    void setReverbParameters(double decay, double mix) {
        reverb = ReverbEffect(decay, mix);
    }

    void initializeAudioBackend() {
        initializeAudio();
    }

    void cleanupAudioBackend() {
        cleanupAudio();
    }
}

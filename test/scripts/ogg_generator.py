import numpy as np
import argparse
import sys
import soundfile as sf

if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog='OggGenerator')
    parser.add_argument('program_name', type=str)
    parser.add_argument('output_filename', type=str)
    args = parser.parse_args(sys.argv)

    frequency_left = 440  # Frequency for left channel (A4 pitch)
    frequency_right = 554  # Frequency for right channel (C#5 pitch)
    duration = 0.2
    sample_rate = 44100

    t = np.linspace(0, duration, int(sample_rate * duration), endpoint=False)

    samples_left = np.sin(2 * np.pi * frequency_left * t)
    samples_right = np.sin(2 * np.pi * frequency_right * t)
    stereo_samples = np.column_stack((samples_left, samples_right))

    stereo_samples = np.int16(stereo_samples * 32767)

    sf.write(args.output_filename, stereo_samples, sample_rate,
             format='OGG', subtype='VORBIS')

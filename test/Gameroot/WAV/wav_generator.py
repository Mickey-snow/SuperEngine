import wave
import argparse
import numpy as np
import struct
import sys


def generate_sine_wave(frequency, duration, sample_rate):
    """Generate a sine wave signal."""
    t = np.linspace(0, duration, int(sample_rate * duration), endpoint=False)
    wave = np.sin(2 * np.pi * frequency * t)
    return wave


def convert_to_pcm(data, sample_width):
    """Convert numpy array data to PCM format based on sample width."""
    if sample_width == 1:
        # 8-bit audio
        data = np.clip(data * 127.5 + 127.5, 0, 255).astype(np.uint8)
        pcm_data = data.tobytes()
    elif sample_width == 2:
        # 16-bit audio
        data = np.clip(data * 32767.0, -32768, 32767).astype(np.int16)
        pcm_data = data.tobytes()
    elif sample_width == 3:
        # 24-bit audio
        data = np.clip(data * 8388607.0, -8388608, 8388607).astype(np.int32)
        pcm_data = b''.join(struct.pack('<i', sample)[0:3] for sample in data)
    elif sample_width == 4:
        # 32-bit audio
        data = np.clip(data * 2147483647.0, -2147483648, 2147483647).astype(np.int32)
        pcm_data = data.tobytes()
    else:
        raise ValueError("Unsupported sample width")
    return pcm_data


def write_wave(file_name, data, sample_rate, channels, sample_width):
    """Write the provided data to a WAV file with specified parameters."""
    with wave.open(file_name, 'wb') as wf:
        wf.setnchannels(channels)
        wf.setsampwidth(sample_width)
        wf.setframerate(sample_rate)
        wf.writeframes(data)


def write_wav_param(file_name, sample_rate, channels, sample_width, frequency, duration):
    """Write audio signal parameters."""
    with open(f"{file_name}.param", "w") as f:
        f.write(f"{sample_rate} {channels} {sample_width} {frequency} {duration}\n")


def generate_test_wav_files(output_file, sample_rate, frequency,
                            channels, sample_width, duration):
    """Generate test WAV files with different parameters."""
    # Generate sine wave
    sine_wave = generate_sine_wave(frequency, duration, sample_rate)
    if channels == 2:
        sine_wave = np.tile(sine_wave, (2, 1)).T.flatten()  # Duplicate for stereo
    # Convert to PCM data
    pcm_data = convert_to_pcm(sine_wave, sample_width)

    # Write to WAV file
    write_wave(output_file, pcm_data, sample_rate, channels,
               sample_width)
    write_wav_param(output_file, sample_rate, channels,
                    sample_width, frequency, duration)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog='WavGenerator')
    parser.add_argument('program_name', type=str)
    parser.add_argument('output_filename', type=str)
    parser.add_argument('--frequency', type=int, required=True)
    parser.add_argument('--sample_rate', type=int, required=True)
    parser.add_argument('--channel', choices=[1, 2], type=int, required=True)
    parser.add_argument('--sample_format', choices=[8, 16, 24, 32],
                        type=int, required=True)
    parser.add_argument('--duration', type=float, required=True)

    args = parser.parse_args(sys.argv)

    generate_test_wav_files(args.output_filename, args.sample_rate,
                            args.frequency, args.channel, args.sample_format//8,
                            args.duration)

# 播放p3Format的音频Files
import opuslib
import struct
import numpy as np
import sounddevice as sd
import argparse

def play_p3_file(input_file):
    """
    播放p3Format的音频Files
    p3Format: [1字节Class型, 1字节保留, 2字节Length, Opus数据]
    """
    # InitializeOpus解码器
    sample_rate = 16000  # 采样率固定为16000Hz
    channels = 1  # 单声道
    decoder = opuslib.Decoder(sample_rate, channels)
    
    # 帧Size (60ms)
    frame_size = int(sample_rate * 60 / 1000)
    
    # Open音频流
    stream = sd.OutputStream(
        samplerate=sample_rate,
        channels=channels,
        dtype='int16'
    )
    stream.start()
    
    try:
        with open(input_file, 'rb') as f:
            print(f"Currently播放: {input_file}")
            
            while True:
                # Read头部 (4字节)
                header = f.read(4)
                if not header or len(header) < 4:
                    break
                
                # 解析头部
                packet_type, reserved, data_len = struct.unpack('>BBH', header)
                
                # ReadOpus数据
                opus_data = f.read(data_len)
                if not opus_data or len(opus_data) < data_len:
                    break
                
                # 解码Opus数据
                pcm_data = decoder.decode(opus_data, frame_size)
                
                # 将字节转换为numpy数组
                audio_array = np.frombuffer(pcm_data, dtype=np.int16)
                
                # 播放音频
                stream.write(audio_array)
                
    except KeyboardInterrupt:
        print("\n播放AlreadyStop")
    finally:
        stream.stop()
        stream.close()
        print("播放Done")

def main():
    parser = argparse.ArgumentParser(description='播放p3Format的音频Files')
    parser.add_argument('input_file', help='输入的p3Files路径')
    args = parser.parse_args()
    
    play_p3_file(args.input_file)

if __name__ == "__main__":
    main() 

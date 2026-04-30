"""
实时AFSK解调器 - 基于Goertzel算法
"""

import numpy as np
from collections import deque


class TraceGoertzel:
    """实时Goertzel算法Implement"""
    
    def __init__(self, freq: float, n: int):
        """
        InitializeGoertzel算法
        
        Args:
            freq: 归一化Frequency (目标Frequency/采样Frequency)
            n: 窗口Size
        """
        self.freq = freq
        self.n = n
        
        # 预计算系数 - 与参考Code一致
        self.k = int(freq * n)
        self.w = 2.0 * np.pi * freq
        self.cw = np.cos(self.w)
        self.sw = np.sin(self.w)
        self.c = 2.0 * self.cw
        
        # InitializeStatusVariables - 使用deque存储最近两个值
        self.zs = deque([0.0, 0.0], maxlen=2)
    
    def reset(self):
        """重置算法Status"""
        self.zs.clear()
        self.zs.extend([0.0, 0.0])
    
    def __call__(self, xs):
        """
        Processing一组采样点 - 与参考Code一致的接口
        
        Args:
            xs: 采样点序列
            
        Returns:
            计算出的振幅
        """
        self.reset()
        for x in xs:
            z1, z2 = self.zs[-1], self.zs[-2]  # Z[-1], Z[-2]
            z0 = x + self.c * z1 - z2  # S[n] = x[n] + C * S[n-1] - S[n-2]
            self.zs.append(float(z0))  # Update序列
        return self.amp
    
    @property
    def amp(self) -> float:
        """计算当前振幅 - 与参考Code一致"""
        z1, z2 = self.zs[-1], self.zs[-2]
        ip = self.cw * z1 - z2
        qp = self.sw * z1
        return np.sqrt(ip**2 + qp**2) / (self.n / 2.0)


class PairGoertzel:
    """双频Goertzel解调器"""
    
    def __init__(self, f_sample: int, f_space: int, f_mark: int, 
                 bit_rate: int, win_size: int):
        """
        Initialize双频解调器
        
        Args:
            f_sample: 采样Frequency
            f_space: SpaceFrequency (通常对应0)
            f_mark: MarkFrequency (通常对应1)
            bit_rate: 比特率
            win_size: Goertzel窗口Size
        """
        assert f_sample % bit_rate == 0, "采样Frequency必须是比特率的整数倍"
        
        self.Fs = f_sample
        self.F0 = f_space
        self.F1 = f_mark
        self.bit_rate = bit_rate
        self.n_per_bit = int(f_sample // bit_rate)  # 每个比特的采样点数
        
        # 计算归一化Frequency
        f1 = f_mark / f_sample
        f0 = f_space / f_sample
        
        # InitializeGoertzel算法
        self.g0 = TraceGoertzel(freq=f0, n=win_size)
        self.g1 = TraceGoertzel(freq=f1, n=win_size)
        
        # 输入缓冲区
        self.in_buffer = deque(maxlen=win_size)
        self.out_count = 0
        
        print(f"PairGoertzel initialized: f0={f0:.6f}, f1={f1:.6f}, win_size={win_size}, n_per_bit={self.n_per_bit}")
    
    def __call__(self, s: float):
        """
        Processing单个采样点 - 与参考Code一致的接口
        
        Args:
            s: 采样点值
            
        Returns:
            (amp0, amp1, p1_prob) - 空间Frequency振幅，标记Frequency振幅，标记概率
        """
        self.in_buffer.append(s)
        self.out_count += 1
        
        amp0, amp1, p1_prob = 0, 0, None
        
        # 每个比特Period输出一次结果
        if self.out_count >= self.n_per_bit:
            amp0 = self.g0(self.in_buffer)  # 计算spaceFrequency振幅
            amp1 = self.g1(self.in_buffer)  # 计算markFrequency振幅
            p1_prob = amp1 / (amp0 + amp1 + 1e-8)  # 计算mark概率
            self.out_count = 0
            
        return amp0, amp1, p1_prob


class RealTimeAFSKDecoder:
    """实时AFSK解码器 - 基于起始帧触发"""
    
    def __init__(self, f_sample: int = 16000, mark_freq: int = 1800, 
                 space_freq: int = 1500, bitrate: int = 100, 
                 s_goertzel: int = 9, threshold: float = 0.5):
        """
        Initialize实时AFSK解码器
        
        Args:
            f_sample: 采样Frequency
            mark_freq: MarkFrequency
            space_freq: SpaceFrequency 
            bitrate: 比特率
            s_goertzel: Goertzel窗口Size系数 (win_size = f_sample // mark_freq * s_goertzel)
            threshold: 判决门限
        """
        self.f_sample = f_sample
        self.mark_freq = mark_freq
        self.space_freq = space_freq
        self.bitrate = bitrate
        self.threshold = threshold
        
        # 计算窗口Size - 与参考Code一致
        win_size = int(f_sample / mark_freq * s_goertzel)
        
        # Initialize解调器
        self.demodulator = PairGoertzel(f_sample, space_freq, mark_freq, 
                                       bitrate, win_size)
        
        # 帧Define - 与参考Code一致
        self.start_bytes = b'\x01\x02'
        self.end_bytes = b'\x03\x04'
        self.start_bits = "".join(format(int(x), '08b') for x in self.start_bytes)
        self.end_bits = "".join(format(int(x), '08b') for x in self.end_bytes)

        # Status机
        self.state = "idle" # idle / entering
        
        # 存储解调结果
        self.buffer_prelude:deque = deque(maxlen=len(self.start_bits)) # 判断是否Start
        self.indicators = []  # 存储概率序列
        self.signal_bits = ""  # 存储比特序列
        self.text_cache = ""
        
        # 解码结果
        self.decoded_messages = []
        self.total_bits_received = 0
        
        print(f"Decoder initialized: win_size={win_size}")
        print(f"Start frame: {self.start_bits} (from {self.start_bytes.hex()})")
        print(f"End frame: {self.end_bits} (from {self.end_bytes.hex()})")
    
    def process_audio(self, samples: np.array) -> str:
        """
        Processing音频数据并Return解码文本
        
        Args:
            audio_data: 音频字节数据 (16-bit PCM)
            
        Returns:
            新解码的文本
        """       
        new_text = "" 
        # 逐个Processing采样点
        for sample in samples:
            amp0, amp1, p1_prob = self.demodulator(sample)
            # 如果有概率输出，记录并判决
            if p1_prob is not None:
                bit = '1' if p1_prob > self.threshold else '0'
                match self.state:
                    case "idle":
                        self.buffer_prelude.append(bit)
                        pass
                    case "entering":
                        self.buffer_prelude.append(bit)
                        self.signal_bits += bit
                        self.total_bits_received += 1
                    case _:
                        pass
                self.indicators.append(p1_prob)

                # CheckStatus机
                if self.state == "idle" and "".join(self.buffer_prelude) == self.start_bits:
                    self.state = "entering"
                    self.text_cache = ""
                    self.signal_bits = ""  # Clear比特序列
                    self.buffer_prelude.clear()
                elif self.state == "entering" and ("".join(self.buffer_prelude) == self.end_bits or len(self.signal_bits) >= 256):
                    self.state = "idle"
                    self.buffer_prelude.clear()

                # 每收集一定数量的比特后尝试解码
                if len(self.signal_bits) >= 8:
                    text = self._decode_bits_to_text(self.signal_bits)
                    if len(text) > len(self.text_cache):
                        new_text = text[len(self.text_cache) - len(text):]
                        self.text_cache = text
        return new_text
    
    def _decode_bits_to_text(self, bits: str) -> str:
        """
        将比特串解码为文本
        
        Args:
            bits: 比特串
            
        Returns:
            解码出的文本
        """
        if len(bits) < 8:
            return ""
        
        decoded_text = ""
        byte_count = len(bits) // 8
        
        for i in range(byte_count):
            # 提取8位
            byte_bits = bits[i*8:(i+1)*8]
            
            # 位转字节
            byte_val = int(byte_bits, 2)
            
            # 尝试解码为ASCII字符
            if 32 <= byte_val <= 126:  # 可PrintASCII字符
                decoded_text += chr(byte_val)
            elif byte_val == 0:  # NULL字符，忽略
                continue
            else:
                # 非可Print字符pass，以十六进制显示
                pass
                # decoded_text += f"\\x{byte_val:02X}"
        
        return decoded_text
    
    def clear(self):
        """Clear解码Status"""
        self.indicators = []
        self.signal_bits = ""
        self.decoded_messages = []
        self.total_bits_received = 0
        print("解码器StatusAlreadyClear")
    
    def get_stats(self) -> dict:
        """Get解码统计Info"""
        return {
            'prelude_bits': "".join(self.buffer_prelude),
            "state": self.state,
            'total_chars': sum(len(msg) for msg in self.text_cache),
            'buffer_bits': len(self.signal_bits),
            'mark_freq': self.mark_freq,
            'space_freq': self.space_freq,
            'bitrate': self.bitrate,
            'threshold': self.threshold,
        }

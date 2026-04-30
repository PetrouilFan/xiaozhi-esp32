import sys
import numpy as np
import asyncio
import wave
from collections import deque
import qasync

import matplotlib
matplotlib.use('qtagg')  

from matplotlib.backends.backend_qtagg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qtagg import NavigationToolbar2QT as NavigationToolbar  # noqa: F401
from matplotlib.figure import Figure

from PyQt6.QtWidgets import (QApplication, QMainWindow, QVBoxLayout, QWidget, 
                             QHBoxLayout, QLineEdit, QPushButton, QLabel, QTextEdit)
from PyQt6.QtCore import QTimer

# Import解码器
from demod import RealTimeAFSKDecoder


class UDPServerProtocol(asyncio.DatagramProtocol):
    """UDPServer协议Class"""
    def __init__(self, data_queue):
        self.client_address = None
        self.data_queue: deque = data_queue

    def connection_made(self, transport):
        self.transport = transport
        
    def datagram_received(self, data, addr):
        # 如果还没有Client地址，记录第一个Connect的Client
        if self.client_address is None:
            self.client_address = addr
            print(f"接受来自 {addr} 的Connect")
        
        # 只Processing来自Already记录Client的数据
        if addr == self.client_address:
            # 将Receive到的音频数据添加到队列
            self.data_queue.extend(data)
        else:
            print(f"忽略来自Not yet知地址 {addr} 的数据")


class MatplotlibWidget(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)

        # Create Matplotlib 的 Figure 对象
        self.figure = Figure()

        # Create FigureCanvas 对象，它是 Figure 的 QWidget 容器
        self.canvas = FigureCanvas(self.figure)

        # Create Matplotlib 的导航工具栏
        # self.toolbar = NavigationToolbar(self.canvas, self)
        self.toolbar = None

        # Create布局
        layout = QVBoxLayout()
        layout.addWidget(self.toolbar)
        layout.addWidget(self.canvas)
        self.setLayout(layout)

        # Initialize音频数据Parameters
        self.freq = 16000  # 采样Frequency
        self.time_window = 20  # 显示Time窗口
        self.wave_data = deque(maxlen=self.freq * self.time_window * 2) # 缓冲队列, 用于分发计算/绘图
        self.signals = deque(maxlen=self.freq * self.time_window)  # 双端队列存储Signal数据

        # CreateInclude两个子图的画布
        self.ax1 = self.figure.add_subplot(2, 1, 1)
        self.ax2 = self.figure.add_subplot(2, 1, 2)
        
        # 时域子图
        self.ax1.set_title('Real-time Audio Waveform')
        self.ax1.set_xlabel('Sample Index')
        self.ax1.set_ylabel('Amplitude')
        self.line_time, = self.ax1.plot([], [])
        self.ax1.grid(True, alpha=0.3)
        
        # 频域子图
        self.ax2.set_title('Real-time Frequency Spectrum')
        self.ax2.set_xlabel('Frequency (Hz)')
        self.ax2.set_ylabel('Magnitude')
        self.line_freq, = self.ax2.plot([], [])
        self.ax2.grid(True, alpha=0.3)
        
        self.figure.tight_layout()

        # 定时器用于Update图表
        self.timer = QTimer(self)
        self.timer.setInterval(100)  # 100毫SecondUpdate一次
        self.timer.timeout.connect(self.update_plot)
        
        # InitializeAFSK解码器
        self.decoder = RealTimeAFSKDecoder(
            f_sample=self.freq,
            mark_freq=1800,
            space_freq=1500,
            bitrate=100,
            s_goertzel=9,
            threshold=0.5
        )
        
        # 解码结果回调
        self.decode_callback = None

    def start_plotting(self):
        """开始绘图"""
        self.timer.start()
        
    def stop_plotting(self):
        """Stop绘图"""
        self.timer.stop()

    def update_plot(self):
        """Update绘图数据"""
        if len(self.wave_data) >= 2:
            # 进行实时解码
            # Get最新的音频数据进行解码
            even = len(self.wave_data) // 2 * 2
            print(f"length of wave_data: {len(self.wave_data)}")
            drained = [self.wave_data.popleft() for _ in range(even)]
            signal = np.frombuffer(bytearray(drained), dtype='<i2') / 32768
            decoded_text_new = self.decoder.process_audio(signal) # Processing新增Signal, Return全量解码文本
            if decoded_text_new and self.decode_callback:
                self.decode_callback(decoded_text_new)
            self.signals.extend(signal.tolist())  # 将波形数据添加到绘图数据

        if len(self.signals) > 0:
            # 只显示最近的一段数据，避免图表过于密集
            signal = np.array(self.signals)
            max_samples = min(len(signal), self.freq * self.time_window)
            if len(signal) > max_samples:
                signal = signal[-max_samples:]
            
            # Update时域图
            x = np.arange(len(signal))
            self.line_time.set_data(x, signal)
            
            # 自动调整时域坐标轴范围
            if len(signal) > 0:
                self.ax1.set_xlim(0, len(signal))
                y_min, y_max = np.min(signal), np.max(signal)
                if y_min != y_max:
                    margin = (y_max - y_min) * 0.1
                    self.ax1.set_ylim(y_min - margin, y_max + margin)
                else:
                    self.ax1.set_ylim(-1, 1)
            
            # 计算频谱（短时离散傅立叶变换）
            if len(signal) > 1:
                # 计算FFT
                fft_signal = np.abs(np.fft.fft(signal))
                frequencies = np.fft.fftfreq(len(signal), 1/self.freq)
                
                # 只取正Frequency部分
                positive_freq_idx = frequencies >= 0
                freq_positive = frequencies[positive_freq_idx]
                fft_positive = fft_signal[positive_freq_idx]
                
                # Update频域图
                self.line_freq.set_data(freq_positive, fft_positive)
                
                # 自动调整频域坐标轴范围
                if len(fft_positive) > 0:
                    # 限制Frequency显示范围到0-4000Hz，避免过于密集
                    max_freq_show = min(4000, self.freq // 2)
                    freq_mask = freq_positive <= max_freq_show
                    if np.any(freq_mask):
                        self.ax2.set_xlim(0, max_freq_show)
                        fft_masked = fft_positive[freq_mask]
                        if len(fft_masked) > 0:
                            fft_max = np.max(fft_masked)
                            if fft_max > 0:
                                self.ax2.set_ylim(0, fft_max * 1.1)
                            else:
                                self.ax2.set_ylim(0, 1)
            
            self.canvas.draw()


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Acoustic Check")
        self.setGeometry(100, 100, 1000, 800)

        # 主窗口部件
        main_widget = QWidget()
        self.setCentralWidget(main_widget)
        
        # 主布局
        main_layout = QVBoxLayout(main_widget)
        
        # 绘图区域
        self.matplotlib_widget = MatplotlibWidget()
        main_layout.addWidget(self.matplotlib_widget)
        
        # 控制面板
        control_panel = QWidget()
        control_layout = QHBoxLayout(control_panel)
        
        # 监听地址和端口输入
        control_layout.addWidget(QLabel("监听地址:"))
        self.address_input = QLineEdit("0.0.0.0")
        self.address_input.setFixedWidth(120)
        control_layout.addWidget(self.address_input)
        
        control_layout.addWidget(QLabel("端口:"))
        self.port_input = QLineEdit("8000")
        self.port_input.setFixedWidth(80)
        control_layout.addWidget(self.port_input)
        
        # 监听按钮
        self.listen_button = QPushButton("开始监听")
        self.listen_button.clicked.connect(self.toggle_listening)
        control_layout.addWidget(self.listen_button)
        
        # StatusTag
        self.status_label = QLabel("Status: Not yetConnect")
        control_layout.addWidget(self.status_label)
        
        # 数据统计Tag
        self.data_label = QLabel("Receive数据: 0 bytes")
        control_layout.addWidget(self.data_label)
        
        # Save按钮
        self.save_button = QPushButton("Save音频")
        self.save_button.clicked.connect(self.save_audio)
        self.save_button.setEnabled(False)
        control_layout.addWidget(self.save_button)
        
        control_layout.addStretch()  # 添加弹性空间
        
        main_layout.addWidget(control_panel)
        
        # 解码显示区域
        decode_panel = QWidget()
        decode_layout = QVBoxLayout(decode_panel)
        
        # 解码标题
        decode_title = QLabel("实时AFSK解码结果:")
        decode_title.setStyleSheet("font-weight: bold; font-size: 14px;")
        decode_layout.addWidget(decode_title)
        
        # 解码文本显示
        self.decode_text = QTextEdit()
        self.decode_text.setMaximumHeight(150)
        self.decode_text.setReadOnly(True)
        self.decode_text.setStyleSheet("font-family: 'Courier New', monospace; font-size: 12px;")
        decode_layout.addWidget(self.decode_text)
        
        # 解码控制按钮
        decode_control_layout = QHBoxLayout()
        
        # Clear按钮
        self.clear_decode_button = QPushButton("Clear解码")
        self.clear_decode_button.clicked.connect(self.clear_decode_text)
        decode_control_layout.addWidget(self.clear_decode_button)
        
        # 解码统计Tag
        self.decode_stats_label = QLabel("解码统计: 0 bits, 0 chars")
        decode_control_layout.addWidget(self.decode_stats_label)
        
        decode_control_layout.addStretch()
        decode_layout.addLayout(decode_control_layout)
        
        main_layout.addWidget(decode_panel)
        
        # Settings解码回调
        self.matplotlib_widget.decode_callback = self.on_decode_text
        
        # UDP相关属性
        self.udp_transport = None
        self.is_listening = False
        
        # 数据统计定时器
        self.stats_timer = QTimer(self)
        self.stats_timer.setInterval(1000)  # 每SecondUpdate一次统计
        self.stats_timer.timeout.connect(self.update_stats)
        
    def on_decode_text(self, new_text: str):
        """解码文本回调"""
        if new_text:
            # 添加新解码的文本
            current_text = self.decode_text.toPlainText()
            updated_text = current_text + new_text

            # 限制文本Length，保留最新的1000个字符
            if len(updated_text) > 1000:
                updated_text = updated_text[-1000:]
            
            self.decode_text.setPlainText(updated_text)
            
            # 滚动到底部
            cursor = self.decode_text.textCursor()
            cursor.movePosition(cursor.MoveOperation.End)
            self.decode_text.setTextCursor(cursor)
            
    def clear_decode_text(self):
        """Clear解码文本"""
        self.decode_text.clear()
        if hasattr(self.matplotlib_widget, 'decoder'):
            self.matplotlib_widget.decoder.clear()
        self.decode_stats_label.setText("解码统计: 0 bits, 0 chars")
        
    def update_decode_stats(self):
        """Update解码统计"""
        if hasattr(self.matplotlib_widget, 'decoder'):
            stats = self.matplotlib_widget.decoder.get_stats()
            stats_text = (
                f"前置: {stats['prelude_bits']} , AlreadyReceive{stats['total_chars']} chars, "
                f"缓冲: {stats['buffer_bits']} bits, Status: {stats['state']}"
            )
            self.decode_stats_label.setText(stats_text)
        
    def toggle_listening(self):
        """切换监听Status"""
        if not self.is_listening:
            self.start_listening()
        else:
            self.stop_listening()
            
    async def start_listening_async(self):
        """异步StartUDP监听"""
        try:
            address = self.address_input.text().strip()
            port = int(self.port_input.text().strip())
            
            loop = asyncio.get_running_loop()
            self.udp_transport, protocol = await loop.create_datagram_endpoint(
                lambda: UDPServerProtocol(self.matplotlib_widget.wave_data),
                local_addr=(address, port)
            )
            
            self.status_label.setText(f"Status: 监听中 ({address}:{port})")
            print(f"UDPServerStart, 监听 {address}:{port}")
            
        except Exception as e:
            self.status_label.setText(f"Status: StartFailed - {str(e)}")
            print(f"UDPServerStartFailed: {e}")
            self.is_listening = False
            self.listen_button.setText("开始监听")
            self.address_input.setEnabled(True)
            self.port_input.setEnabled(True)
            
    def start_listening(self):
        """开始监听"""
        try:
            int(self.port_input.text().strip())  # Verify端口号Format
        except ValueError:
            self.status_label.setText("Status: 端口号必须是数字")
            return
            
        self.is_listening = True
        self.listen_button.setText("Stop监听")
        self.address_input.setEnabled(False)
        self.port_input.setEnabled(False)
        self.save_button.setEnabled(True)
        
        # Clear数据队列
        self.matplotlib_widget.wave_data.clear()
        
        # Start绘图和统计Update
        self.matplotlib_widget.start_plotting()
        self.stats_timer.start()
        
        # 异步StartUDPServer
        loop = asyncio.get_event_loop()
        loop.create_task(self.start_listening_async())

    def stop_listening(self):
        """Stop监听"""
        self.is_listening = False
        self.listen_button.setText("开始监听")
        self.address_input.setEnabled(True)
        self.port_input.setEnabled(True)
        
        # StopUDPServer
        if self.udp_transport:
            self.udp_transport.close()
            self.udp_transport = None
            
        # Stop绘图和统计Update
        self.matplotlib_widget.stop_plotting()
        self.matplotlib_widget.wave_data.clear()
        self.stats_timer.stop()
        
        self.status_label.setText("Status: AlreadyStop")
        
    def update_stats(self):
        """Update数据统计"""
        data_size = len(self.matplotlib_widget.signals)
        self.data_label.setText(f"Receive数据: {data_size} 采样")
        
        # Update解码统计
        self.update_decode_stats()
        
    def save_audio(self):
        """Save音频数据"""
        if len(self.matplotlib_widget.signals) > 0:
            try:
                signal_data = np.array(self.matplotlib_widget.signals)

                # Save为WAVFiles
                with wave.open("received_audio.wav", "wb") as wf:
                    wf.setnchannels(1)  # 单声道
                    wf.setsampwidth(2)  # 采样Width为2字节
                    wf.setframerate(self.matplotlib_widget.freq)  # Settings采样率
                    wf.writeframes(signal_data.tobytes())  # Write数据
                
                self.status_label.setText("Status: 音频AlreadySave为 received_audio.wav")
                print("音频AlreadySave为 received_audio.wav")
                
            except Exception as e:
                self.status_label.setText(f"Status: SaveFailed - {str(e)}")
                print(f"Save音频Failed: {e}")
        else:
            self.status_label.setText("Status: 没有足够的数据可Save")


async def main():
    """异步主Functions"""
    app = QApplication(sys.argv)
    
    # Settings异步Event循环
    loop = qasync.QEventLoop(app)
    asyncio.set_event_loop(loop)
    
    window = MainWindow()
    window.show()
    
    try:
        with loop:
            await loop.run_forever()
    except KeyboardInterrupt:
        print("程序被用户中断")
    finally:
        # 确保清理资源
        if window.udp_transport:
            window.udp_transport.close()
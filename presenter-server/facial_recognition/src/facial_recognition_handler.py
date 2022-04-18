"""人脸识别通道处理模块"""

import time
import logging
from common.channel_handler import ChannelHandler

HEARTBEAT_TIMEOUT = 100


class FacialRecognitionHandler(ChannelHandler):
    """人脸识别通道处理"""

    def __init__(self, channel_name, media_type):
        """初始化函数"""
        self.face_list = None
        self.sleep_time = 0.01
        super(FacialRecognitionHandler, self).__init__(channel_name, media_type)

    # video线程
    def _video_thread(self):
        """处理视频的后台线程"""
        logging.info('创建%s视频处理线程', self.thread_name)
        for frame in self.frames():
            if frame:
                # 向浏览器发送信号
                self.frame_data = frame
                self.web_event.set()

            # 退出线程
            if self.close_thread_switch:
                self.channel_manager.clean_channel_resource_by_name(self.channel_name)
                logging.info('停止线程：%s。', self.thread_name)
                break

    # 生成器生成帧
    def frames(self):
        """生成器生成帧图像"""
        while True:
            self.image_event.wait()
            self.image_event.clear()
            if self.img_data:
                yield self.img_data
                self.img_data = None

            # 如果设置_close_thread_switch，立即返回
            if self.close_thread_switch:
                yield None

            # 如果100秒内没有帧或心跳，停止线程并关闭套接字
            if time.time() - self.heartbeat > HEARTBEAT_TIMEOUT:
                self.set_thread_switch()
                self.img_data = None
                yield None

    # 保存帧
    def save_frame(self, image, face_list):
        """
        Description: 保存帧信息
        Input:
            image: 原始图像数据
            face_list: 人脸信息，包括用户ID、用户名、人脸特征、人脸坐标、温度
        """
        while self.img_data:
            time.sleep(self.sleep_time)

        # 计算fps
        self.time_list.append(self.heartbeat)
        self.image_number += 1
        while self.time_list[0] + 1 < time.time():
            self.time_list.pop(0)
            self.image_number -= 1
            if self.image_number == 0:
                break

        self.fps = len(self.time_list)
        self.img_data = image
        self.face_list = face_list
        self.image_event.set()
        self.set_heartbeat()

    # 获取帧
    def get_frame(self):
        """
        Description: 获取帧信息
        """
        # 等到收到帧数据，然后将其推送到浏览器。
        ret = self.web_event.wait()
        self.web_event.clear()

        if ret:
            # 如果web_event.set()，否则timeout退出
            return {
                "image": self.frame_data,
                "fps": self.fps,
                "face_list": self.face_list
            }

        return {}
